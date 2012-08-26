#include <lv2.h>
#include <malloc.h>
#include <fluidsynth.h>
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"


#include "fluidsynth.h"
#include "uris.h"



static LV2_Descriptor *fluidDescriptor = NULL;

typedef struct {
    LV2_Atom_Sequence* control;
    LV2_Atom_Sequence* notify_port;
    LV2_Atom_Forge_Frame notify_frame;

    float* left;
    float* right;

    LV2_URID_Map*        map;
    LV2_Worker_Schedule* schedule;
    LV2_Atom_Forge forge;
    uint32_t frame_offset;

    FluidSynthURIs uris;
    SoundFontData soundfont_data;
    
    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_sequencer_t* sequencer;
    short synthSeqId;
    unsigned int samples_per_tick;
} FluidSynth;

static void cleanupFluidSynth(LV2_Handle instance) {
    FluidSynth *plugin_data = (FluidSynth*)instance;
    delete_fluid_synth(plugin_data->synth);
    delete_fluid_settings(plugin_data->settings);
    delete_fluid_sequencer(plugin_data->sequencer);
    free(instance);
}

static void connectPortFluidSynth(LV2_Handle instance,
                                  uint32_t port, void *data) {

    FluidSynth* plugin = (FluidSynth *)instance;

    switch (port) {
    case CONTROL:
        plugin->control = (LV2_Atom_Sequence*)data;
        break;
    case NOTIFY:
        plugin->notify_port = (LV2_Atom_Sequence*)data;
        break;
    case LEFT:
        plugin->left = (float*)data;
        break;
    case RIGHT:
        plugin->right = (float*)data;
        break;
    }
}

static LV2_Handle instantiateFluidSynth( const LV2_Descriptor *desc, double sample_rate,
                                         const char *bundle_path, const LV2_Feature * const * features) {

    FluidSynth* plugin_data = (FluidSynth*)malloc(sizeof(FluidSynth));
    if (!plugin_data) return NULL;
    memset(plugin_data, 0, sizeof(FluidSynth));


    plugin_data->settings = new_fluid_settings();
    fluid_settings_setnum(plugin_data->settings, "synth.sample-rate", sample_rate);
    plugin_data->synth = new_fluid_synth(plugin_data->settings);
    plugin_data->sequencer = new_fluid_sequencer2(true);
    plugin_data->synthSeqId = fluid_sequencer_register_fluidsynth(plugin_data->sequencer, plugin_data->synth);

    plugin_data->samples_per_tick = sample_rate/FLUID_TIME_SCALE;

    plugin_data->map = 0;
    plugin_data->schedule = 0;
    for (int i =0; features[i]; i++) {
        printf("%s\n", features[i]->URI);
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            plugin_data->map = (LV2_URID_Map*)features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_WORKER__schedule)) {
            plugin_data->schedule = (LV2_Worker_Schedule*)features[i]->data;
        }
    }

    if (plugin_data->map == 0) {
        fprintf(stderr, "Host does not support urid:map\n");
        goto fail;
    } else if (plugin_data->schedule == 0) {
        fprintf(stderr, "Host does not support work:schedule\n");
        goto fail;
    }
    map_fluidsynth_uris(plugin_data->map, &plugin_data->uris);
    lv2_atom_forge_init(&plugin_data->forge, plugin_data->map);

    return (LV2_Handle)plugin_data;
fail:
    delete_fluid_synth(plugin_data->synth);
    delete_fluid_settings(plugin_data->settings);
    delete_fluid_sequencer(plugin_data->sequencer);
    free(plugin_data);
    return 0;
}

static void runFluidSynth(LV2_Handle instance, uint32_t nframes) {
    fluid_event_t* fluid_evt;
    unsigned int chan;
    unsigned int when;
    bool unhandled_opcode;
    uint8_t* midi_data;

    FluidSynth *plugin_data = (FluidSynth*)instance;
    FluidSynthURIs* uris = &plugin_data->uris;
    /* Set up forge to write directly to notify output port. */
    const uint32_t notify_capacity = plugin_data->notify_port->atom.size;
    lv2_atom_forge_set_buffer(&plugin_data->forge,
                              (uint8_t*)plugin_data->notify_port,
                              notify_capacity);

    /* Start a sequence in the notify output port. */
    lv2_atom_forge_sequence_head(&plugin_data->forge, &plugin_data->notify_frame, 0);

    LV2_ATOM_SEQUENCE_FOREACH(plugin_data->control, ev) {
        if (ev->body.type == uris->midi_Event) {
            midi_data = (uint8_t*)(ev + 1);
            fluid_evt = new_fluid_event();
            fluid_event_set_source(fluid_evt, -1);
            fluid_event_set_dest(fluid_evt, plugin_data->synthSeqId);

            unhandled_opcode = false;
            when = ev->time.frames / plugin_data->samples_per_tick;
            chan = (midi_data[0] & 0x0F);
            switch ((midi_data[0] & 0xF0)) {
            case NOTE_OFF:
                fluid_event_noteoff(fluid_evt, chan, midi_data[1]);
                break;
            case NOTE_ON:
                fluid_event_noteon(fluid_evt, chan, midi_data[1], midi_data[2]);
                break;
            case CONTROL_CHANGE:
                fluid_event_control_change(fluid_evt, chan, midi_data[1], midi_data[2]);
                break;
            case PROGRAM_CHANGE:
                fluid_event_program_change(fluid_evt, chan, midi_data[1]);
                break;
            case PITCH_BEND:
                fluid_event_pitch_bend(fluid_evt, chan, midi_data[1] + (midi_data[2]<<7));
                break;
            case CHANNEL_PRESSURE:
                fluid_event_channel_pressure(fluid_evt, chan, midi_data[1]);
                break;
            case MIDI_SYSTEM_RESET:
                fluid_event_system_reset(fluid_evt);
                break;
            default:
                unhandled_opcode = true;
            }
            if (!unhandled_opcode) {
                fluid_sequencer_send_at(plugin_data->sequencer, fluid_evt, when, 0);
            }
            delete_fluid_event(fluid_evt);

		} else if (is_object_type(uris, ev->body.type)) {
            const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == uris->patch_Set) {
                /* Received a set message, send it to the worker. */
                plugin_data->schedule->schedule_work(plugin_data->schedule->handle,
                                                     lv2_atom_total_size(&ev->body),
                                                     &ev->body);
            } else {
                printf("Unknown object type %d\n", obj->body.otype);
            }
        } else {
            printf("Unknown event type rec'd: %d\n", ev->body.type);
        }
    }

    fluid_synth_write_float(plugin_data->synth,
                            nframes,
                            plugin_data->left,
                            0,
                            1,
                            plugin_data->right,
                            0,
                            1);
}

static LV2_Worker_Status
work(LV2_Handle                  instance,
     LV2_Worker_Respond_Function respond,
     LV2_Worker_Respond_Handle   handle,
     uint32_t                    size,
     const void*                 data)
{
    FluidSynth *plugin_data = (FluidSynth*)instance;
    LV2_Atom* atom = (LV2_Atom*)data;
    if (0) {//atom->type == plugin_data->uris.eg_freeSample) {
        /* Free old sample */
//        SampleMessage* msg = (SampleMessage*)data;
//        free_sample(self, msg->sample);
    } else {
        LV2_Atom_Object* obj = (LV2_Atom_Object*)data;

        /* Get file path from message */
        const LV2_Atom* file_path = read_set_file(&plugin_data->uris, obj);
        if (!file_path) {
            return LV2_WORKER_ERR_UNKNOWN;
        }

        int sf;
        if (sf = fluid_synth_sfload(plugin_data->synth, LV2_ATOM_BODY(file_path), 1)) {
            fluid_synth_sfont_select(plugin_data->synth, 0, sf);

            fluid_sfont_t* sfont = fluid_synth_get_sfont(plugin_data->synth, 0);
            plugin_data->soundfont_data.name = sfont->get_name(sfont);
//            printf("%s\n", (*sfont->get_name)(sfont));
            sfont->iteration_start(sfont);
            fluid_preset_t preset;
//            int first_preset = -1;
            FluidPreset *prev = NULL, *curr=NULL, *first_preset=NULL;
            while(sfont->iteration_next(sfont, &preset)) {
                curr = (FluidPreset*)malloc(sizeof(FluidPreset));
                if (prev) prev->next = (struct FluidPreset*)curr;
                else plugin_data->soundfont_data.preset_list=curr;
                curr->bank = preset.get_banknum(&preset);
                curr->num = preset.get_num(&preset);
                curr->name = preset.get_name(&preset);
                curr->next = NULL;
                prev = curr;
                if (first_preset == NULL) first_preset = curr;
            }
            if (first_preset) {
                fluid_synth_bank_select(plugin_data->synth, 0, first_preset->bank);
                fluid_synth_program_change(plugin_data->synth, 0, first_preset->num);
            }

            respond(handle, 0, NULL);//strlen(LV2_ATOM_BODY(file_path)), LV2_ATOM_BODY(file_path));
        }
    }

    return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status
work_response(LV2_Handle  instance,
              uint32_t    size,
              const void* data)
{
    FluidSynth* plugin_data = (FluidSynth*)instance;


    lv2_atom_forge_frame_time(&plugin_data->forge, plugin_data->frame_offset);
    LV2_Atom_Forge_Frame loaded_frame;
    LV2_Atom_Forge_Frame presetlist_frame;
    LV2_Atom_Forge_Frame preset_frame;

    lv2_atom_forge_blank(&plugin_data->forge, &loaded_frame, 1, plugin_data->uris.sf_loaded);

    lv2_atom_forge_property_head(&plugin_data->forge, plugin_data->uris.sf_file, 0);
    lv2_atom_forge_path(&plugin_data->forge, plugin_data->soundfont_data.name, strlen(plugin_data->soundfont_data.name));

    lv2_atom_forge_property_head(&plugin_data->forge, plugin_data->uris.sf_preset_list, 0);
    lv2_atom_forge_vector_head(&plugin_data->forge, &presetlist_frame, sizeof(LV2_Atom_Tuple), plugin_data->forge.Tuple);
    FluidPreset *curr=NULL;
    int i=0;
    for (curr=plugin_data->soundfont_data.preset_list;curr;curr=curr->next) {
//        printf("%d:%d:%s\n", curr->bank, curr->num, curr->name);
        lv2_atom_forge_tuple(&plugin_data->forge, &preset_frame);
        lv2_atom_forge_int(&plugin_data->forge, curr->bank);
        lv2_atom_forge_int(&plugin_data->forge, curr->num);
        lv2_atom_forge_string(&plugin_data->forge, curr->name, strlen(curr->name));
        lv2_atom_forge_pop(&plugin_data->forge, &preset_frame);
        i++;
        if (i>=16) break; // crappy buffer overflow protection
    }
    lv2_atom_forge_pop(&plugin_data->forge, &presetlist_frame);
    lv2_atom_forge_pop(&plugin_data->forge, &loaded_frame);

    return LV2_WORKER_SUCCESS;
}

static const void*
extension_data(const char* uri)
{
    static const LV2_State_Interface  state  = { NULL, NULL };//save, restore };
    static const LV2_Worker_Interface worker = { work, work_response, NULL };
    if (!strcmp(uri, LV2_STATE__interface)) {
        return &state;
    } else if (!strcmp(uri, LV2_WORKER__interface)) {
        return &worker;
    }
    return NULL;
}

static void init()
{
    fluidDescriptor =
     (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));
    fluidDescriptor->URI = FLUIDSYNTH_URI;
    fluidDescriptor->activate = NULL;
    fluidDescriptor->cleanup = cleanupFluidSynth;
    fluidDescriptor->connect_port = connectPortFluidSynth;
    fluidDescriptor->deactivate = NULL;
    fluidDescriptor->instantiate = instantiateFluidSynth;
    fluidDescriptor->run = runFluidSynth;
    fluidDescriptor->extension_data = extension_data;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (!fluidDescriptor) init();

    switch (index) {
    case 0:
        return fluidDescriptor;
    default:
        return NULL;
    }
}
