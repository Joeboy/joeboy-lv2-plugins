#include <lv2.h>
#include <malloc.h>
#include <fluidsynth.h>
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#include "fluidsynth.h"
#include "uris.h"



static LV2_Descriptor *fluidDescriptor = NULL;

typedef struct {
    LV2_Atom_Sequence* control;
    float* left;
    float* right;

    LV2_URID_Map*        map;
    LV2_Worker_Schedule* schedule;

    FluidSynthURIs uris;
    
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
        /* Handle set message (load sample). */
        LV2_Atom_Object* obj = (LV2_Atom_Object*)data;

        /* Get file path from message */
        const LV2_Atom* file_path = read_set_file(&plugin_data->uris, obj);
        if (!file_path) {
            return LV2_WORKER_ERR_UNKNOWN;
        }

        /* Load sample. */
//        Sample* sample = load_sample(self, LV2_ATOM_BODY(file_path));
        if (fluid_synth_sfload(plugin_data->synth, LV2_ATOM_BODY(file_path), 1)) {
            /* Loaded sample, send it to run() to be applied. */
            respond(handle, strlen(LV2_ATOM_BODY(file_path)), LV2_ATOM_BODY(file_path));
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
    //printf("loaded %s\n", data);

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
