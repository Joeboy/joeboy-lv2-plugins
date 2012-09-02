#include <lv2.h>
#include <malloc.h>
#include <fluidsynth.h>
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"


#include "fluidsynth.h"
#include "uris.h"



static LV2_Descriptor *fluidDescriptor = NULL;

typedef struct {
    LV2_Atom_Sequence* control;
    LV2_Atom_Sequence* notify_port;
    uint32_t notify_buffer_size;
    LV2_Atom_Forge_Frame notify_frame;

    float* left;
    float* right;

    LV2_URID_Map*        map;
    LV2_Worker_Schedule* schedule;
    LV2_Atom_Forge forge;
    LV2_Atom_Forge presetlist_forge;
    uint64_t frame_offset;
    uint8_t *presetlist_buffer;
    uint32_t presetlist_buffer_size;

    FluidSynthURIs uris;
    
    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_sequencer_t* sequencer;
    short synthSeqId;
    unsigned int samples_per_tick;

    char* sf_file;
    int preset_bank;
    int preset_num;
} Plugin;

static void cleanupFluidSynth(LV2_Handle instance) {
    Plugin *plugin = (Plugin*)instance;
    delete_fluid_synth(plugin->synth);
    delete_fluid_settings(plugin->settings);
    delete_fluid_sequencer(plugin->sequencer);
    free(plugin->presetlist_buffer);
    free(instance);
}

static void connectPortFluidSynth(LV2_Handle instance,
                                  uint32_t port, void *data) {

    Plugin* plugin = (Plugin*)instance;

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

static LV2_Handle
instantiateFluidSynth( const LV2_Descriptor *desc, double sample_rate,
                       const char *bundle_path, const LV2_Feature * const * features) {

    Plugin* plugin = (Plugin*)malloc(sizeof(Plugin));
    if (!plugin) return NULL;
    memset(plugin, 0, sizeof(Plugin));

    plugin->settings = new_fluid_settings();
    fluid_settings_setnum(plugin->settings, "synth.sample-rate", sample_rate);
    plugin->synth = new_fluid_synth(plugin->settings);
    plugin->sequencer = new_fluid_sequencer2(true);
    plugin->synthSeqId = fluid_sequencer_register_fluidsynth(plugin->sequencer, plugin->synth);
    plugin->samples_per_tick = sample_rate/FLUID_TIME_SCALE;
    plugin->preset_bank = plugin->preset_num = -1;

    LV2_Options_Option* options = NULL;

    LV2_URID_Unmap * unmap;
    for (int i =0; features[i]; i++) {
//        printf("%s\n", features[i]->URI);
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            plugin->map = (LV2_URID_Map*)features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_WORKER__schedule)) {
            plugin->schedule = (LV2_Worker_Schedule*)features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_OPTIONS__options)) {
            options = features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_URID__unmap)) {
            unmap = (LV2_URID_Unmap*)features[i]->data;
        }
    }
//    for (int i=0;i<35;i++) printf("%02x:%s\n", i, unmap->unmap(unmap->handle, i));

    if (plugin->map == 0) {
        fprintf(stderr, "Host does not support urid:map\n");
        goto fail;
    } else if (plugin->schedule == 0) {
        fprintf(stderr, "Host does not support work:schedule\n");
        goto fail;
    }
    map_fluidsynth_uris(plugin->map, &plugin->uris);
    LV2_URID sequence_size = plugin->map->map(plugin->map->handle, LV2_BUF_SIZE__sequenceSize);
    LV2_URID atom_Int  = plugin->map->map(plugin->map->handle, LV2_ATOM__Int);
    for (const LV2_Options_Option* o = options; o->key; o++) {
        //printf("%s:%s\n", unmap->unmap(unmap->handle, o->key), unmap->unmap(unmap->handle, o->type));
        if (o->key == sequence_size && o->type == atom_Int) {
            plugin->presetlist_buffer_size = *(int*)o->value;
        }
    }

    plugin->presetlist_buffer = malloc(plugin->presetlist_buffer_size);
//    printf("%d\n", plugin->presetlist_buffer_size);
    lv2_atom_forge_init(&plugin->forge, plugin->map);
    lv2_atom_forge_init(&plugin->presetlist_forge, plugin->map);

    return (LV2_Handle)plugin;
fail:
    delete_fluid_synth(plugin->synth);
    delete_fluid_settings(plugin->settings);
    delete_fluid_sequencer(plugin->sequencer);
    free(plugin);
    return 0;
}

static void runFluidSynth(LV2_Handle instance, uint32_t nframes) {
    fluid_event_t* fluid_evt;
    unsigned int chan;
    unsigned int when;
    bool unhandled_opcode;
    uint8_t* midi_data;

    Plugin *plugin = (Plugin*)instance;
    plugin->notify_buffer_size = plugin->notify_port->atom.size;
    lv2_atom_forge_set_buffer(&plugin->forge,
                              (uint8_t*)plugin->notify_port,
                              plugin->notify_buffer_size);

    /* Start a sequence in the notify output port. */
    lv2_atom_forge_sequence_head(&plugin->forge, &plugin->notify_frame, 0);

    LV2_ATOM_SEQUENCE_FOREACH(plugin->control, ev) {
        plugin->frame_offset = ev->time.frames;
        if (ev->body.type == plugin->uris.midi_Event) {
            midi_data = (uint8_t*)(ev + 1);
//            printf("rec'd %02x%02x%02x\n", midi_data[0], midi_data[1], midi_data[2]);
            fluid_evt = new_fluid_event();
            fluid_event_set_source(fluid_evt, -1);
            fluid_event_set_dest(fluid_evt, plugin->synthSeqId);

            unhandled_opcode = false;
            when = ev->time.frames / plugin->samples_per_tick;
            chan = (midi_data[0] & 0x0F);
            switch ((midi_data[0] & 0xF0)) {
            case NOTE_OFF:
                fluid_event_noteoff(fluid_evt, chan, midi_data[1]);
                break;
            case NOTE_ON:
                fluid_event_noteon(fluid_evt, chan, midi_data[1], midi_data[2]);
                break;
            case CONTROL_CHANGE:
                plugin->preset_bank = midi_data[2];
                fluid_event_control_change(fluid_evt, chan, midi_data[1], midi_data[2]);
                break;
            case PROGRAM_CHANGE:
                plugin->preset_num = midi_data[1];
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
                fluid_sequencer_send_at(plugin->sequencer, fluid_evt, when, 0);
            }
            delete_fluid_event(fluid_evt);

        } else if (ev->body.type == plugin->uris.atom_Resource
                || ev->body.type == plugin->uris.atom_Blank) {

            const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == plugin->uris.patch_Set) {
                plugin->schedule->schedule_work(plugin->schedule->handle, lv2_atom_total_size(&ev->body), &ev->body);
            } else {
                printf("Unknown object type %d\n", obj->body.otype);
            }
        } else {
            printf("Unknown event type rec'd: %d\n", ev->body.type);
        }
    }

    fluid_synth_write_float(plugin->synth,
                            nframes,
                            plugin->left,
                            0,
                            1,
                            plugin->right,
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
    Plugin *plugin = (Plugin*)instance;
    LV2_Atom_Object* obj = (LV2_Atom_Object*)data;

    if (obj->body.otype == plugin->uris.patch_Set) {
        const LV2_Atom_Object* body = NULL;
        const LV2_Atom* file_path_atom = NULL;
        lv2_atom_object_get(obj, plugin->uris.patch_body, &body, 0);
        lv2_atom_object_get(body, plugin->uris.sf_file, &file_path_atom, 0);

        if (!file_path_atom) return LV2_WORKER_ERR_UNKNOWN;

        int sf;
        char *sf_name, *file_path;
        file_path = LV2_ATOM_BODY(file_path_atom);
        sf = fluid_synth_sfload(plugin->synth, file_path, 1);
        if (sf != FLUID_FAILED) {
            fluid_synth_sfont_select(plugin->synth, 0, sf);
            fluid_sfont_t* sfont = fluid_synth_get_sfont(plugin->synth, 0);
            sf_name = sfont->get_name(sfont);
            plugin->sf_file = malloc(1+strlen(file_path));
            strcpy(plugin->sf_file, file_path);
            fluid_preset_t preset;

            const uint32_t capacity = 16384;//plugin->notify_port->atom.size;
            lv2_atom_forge_set_buffer(&plugin->presetlist_forge, plugin->presetlist_buffer, capacity);
            LV2_Atom_Forge_Frame loaded_frame;
            LV2_Atom_Forge_Frame presetlist_frame;
            LV2_Atom_Forge_Frame preset_frame;

            lv2_atom_forge_blank(&plugin->presetlist_forge, &loaded_frame, 1, plugin->uris.sf_loaded);
            lv2_atom_forge_property_head(&plugin->presetlist_forge, plugin->uris.sf_file, 0);
            lv2_atom_forge_path(&plugin->presetlist_forge, sf_name, strlen(sf_name));

            lv2_atom_forge_property_head(&plugin->presetlist_forge, plugin->uris.sf_preset_list, 0);
            lv2_atom_forge_vector_head(&plugin->presetlist_forge, &presetlist_frame, sizeof(LV2_Atom_Tuple), plugin->forge.Tuple);
            char* preset_name;
            sfont->iteration_start(sfont);
            int i = 0;
            while(sfont->iteration_next(sfont, &preset)) {
                lv2_atom_forge_tuple(&plugin->presetlist_forge, &preset_frame);
                lv2_atom_forge_int(&plugin->presetlist_forge, preset.get_banknum(&preset));
                lv2_atom_forge_int(&plugin->presetlist_forge, preset.get_num(&preset));
                preset_name = preset.get_name(&preset);
                lv2_atom_forge_string(&plugin->presetlist_forge, preset_name, strlen(preset_name));
                lv2_atom_forge_pop(&plugin->presetlist_forge, &preset_frame);
                i++;
//                if (i>=8) break; // crappy buffer overflow protection
            }
//            if (first_preset) {
//                fluid_synth_bank_select(plugin->synth, 0, first_preset->bank);
//                fluid_synth_program_change(plugin->synth, 0, first_preset->num);
//            }
            lv2_atom_forge_pop(&plugin->presetlist_forge, &presetlist_frame);
            lv2_atom_forge_pop(&plugin->presetlist_forge, &loaded_frame);

            respond(handle, 0, NULL);
        } else {
            printf("Failed to load soundfont %s\n", file_path);
            // TODO
        }
    } else {
        printf("Ignoring unknown message type %d\n", obj->body.otype);
    }

    return LV2_WORKER_SUCCESS;
}


static LV2_Worker_Status
work_response(LV2_Handle  instance,
              uint32_t    size,
              const void* data) {
    Plugin* plugin = (Plugin*)instance;
    LV2_Atom* a = (LV2_Atom*)plugin->presetlist_buffer;
    lv2_atom_forge_frame_time(&plugin->forge, plugin->frame_offset);
    memcpy((uint8_t*)plugin->forge.buf+plugin->forge.offset, plugin->presetlist_buffer, a->size);
    return LV2_WORKER_SUCCESS;
}

LV2_State_Status
save(LV2_Handle                 instance,
     LV2_State_Store_Function   store,
     LV2_State_Handle           handle,
     uint32_t                   flags,
     const LV2_Feature *const * features)
{
    Plugin*   plugin   = (Plugin*)instance;

    if (plugin->sf_file) {
        store(handle,
              plugin->uris.sf_file,
              plugin->sf_file,
              strlen(plugin->sf_file) + 1,
              plugin->uris.atom_Path,
              LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
    }
    if (plugin->preset_bank != -1) {
        store(handle,
              plugin->uris.sf_preset_bank,
              &plugin->preset_bank,
              sizeof(int),
              plugin->uris.atom_Int,
              LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
    }
    if (plugin->preset_num != -1) {
        store(handle,
              plugin->uris.sf_preset_num,
              &plugin->preset_num,
              sizeof(int),
              plugin->uris.atom_Int,
              LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
    }

    return LV2_STATE_SUCCESS;
}

LV2_State_Status
restore(LV2_Handle                  instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature *const *  features)
{
    Plugin* plugin = (Plugin*)instance;

    size_t      size;
    uint32_t    type;
    uint32_t    valflags;
    const char* sf_file = retrieve(
        handle, plugin->uris.sf_file, &size, &type, &valflags);
    printf("TODO: restore: sf_file=%s\n", sf_file);
    // TODO

    return LV2_STATE_SUCCESS;
}

static const void*
extension_data(const char* uri)
{
    static const LV2_State_Interface  state  = { save, restore };
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
