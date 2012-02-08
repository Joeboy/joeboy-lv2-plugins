#include <lv2.h>
#include <malloc.h>
#include <fluidsynth.h>
#include "urid.h"
#include "fluidsynth.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"


static LV2_Descriptor *fluidDescriptor = NULL;

typedef struct {
    LV2_Atom_Sequence* midi_in;
    float* left;
    float* right;

    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_sequencer_t* sequencer;
    short synthSeqId;
    LV2_URID midi_event_id;
    unsigned int samples_per_tick;
    bool program_is_ready;
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
    case MIDI:
        plugin->midi_in = (LV2_Atom_Sequence*)data;
        break;
    case LEFT:
        plugin->left = data;
        break;
    case RIGHT:
        plugin->right = data;
        break;
    }
}

static LV2_Handle instantiateFluidSynth( const LV2_Descriptor *desc, double sample_rate,
                                         const char *bundle_path, const LV2_Feature * const * features) {

    FluidSynth* plugin_data = (FluidSynth*)malloc(sizeof(FluidSynth));

    plugin_data->settings = new_fluid_settings();
    fluid_settings_setnum(plugin_data->settings, "synth.sample-rate", sample_rate);
    plugin_data->synth = new_fluid_synth(plugin_data->settings);
    plugin_data->sequencer = new_fluid_sequencer2(true);
    plugin_data->synthSeqId = fluid_sequencer_register_fluidsynth(plugin_data->sequencer, plugin_data->synth);

    // TODO: unhardcode this and move it somewhere useful
    fluid_synth_sfload(plugin_data->synth, "/usr/share/sounds/sf2/FluidR3_GM.sf2", 1);
    plugin_data->program_is_ready = true;

    plugin_data->samples_per_tick = sample_rate/FLUID_TIME_SCALE;

    LV2_URID_Map* map_feature = 0;
    for (int j =0; features[j]; j++) {
        if (!strcmp(features[j]->URI, "http://lv2plug.in/ns/ext/urid#map")) {
            map_feature = (LV2_URID_Map*)features[j]->data;
            plugin_data->midi_event_id = map_feature->map(map_feature->handle,
                                                          LV2_MIDI_EVENT_URI);
        }
    }
    if (map_feature == 0) {
        fprintf(stderr, "Host does not support urid:map\n");
        goto fail;
    }

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
    if (!plugin_data->program_is_ready) {
        return;
    }

    LV2_SEQUENCE_FOREACH(plugin_data->midi_in, i) {
        LV2_Atom_Event* const ev = lv2_sequence_iter_get(i);
        if (ev->body.type == plugin_data->midi_event_id) {
            midi_data = (uint8_t*)(ev + 1);
            fluid_evt = new_fluid_event();
            fluid_event_set_source(fluid_evt, -1);
            fluid_event_set_dest(fluid_evt, plugin_data->synthSeqId);

            unhandled_opcode = false;
            when = ev->time.audio.frames / plugin_data->samples_per_tick;
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
    fluidDescriptor->extension_data = NULL;
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
