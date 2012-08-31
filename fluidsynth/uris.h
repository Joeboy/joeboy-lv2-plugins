#ifndef FLUIDSYNTH_URIS_H
#define FLUIDSYNTH_URIS_H

#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

#define LV2_MIDI_EVENT_URI          "http://lv2plug.in/ns/ext/midi#MidiEvent"
#define FLUIDSYNTH_URI              "http://www.joebutton.co.uk/software/lv2/fluidsynth"
#define FLUIDSYNTH_UI_URI           FLUIDSYNTH_URI "#ui"
#define FLUIDSYNTH__sf_loaded       FLUIDSYNTH_URI "#sf_loaded"
#define FLUIDSYNTH__sf_file         FLUIDSYNTH_URI "#sf_file"
#define FLUIDSYNTH__sf_preset_list  FLUIDSYNTH_URI "#sf_preset_list"
#define FLUIDSYNTH__sf_preset_bank  FLUIDSYNTH_URI "#sf_preset_bank"
#define FLUIDSYNTH__sf_preset_num   FLUIDSYNTH_URI "#sf_preset_num"


typedef struct {
    LV2_URID atom_Blank;
    LV2_URID atom_Path;
    LV2_URID atom_Resource;
    LV2_URID atom_Int;
    LV2_URID atom_eventTransfer;
    LV2_URID midi_Event;
    LV2_URID patch_Set;
    LV2_URID patch_body;
    LV2_URID sf_loaded;
    LV2_URID sf_file;
    LV2_URID sf_preset_list;
    LV2_URID sf_preset_bank;
    LV2_URID sf_preset_num;
} FluidSynthURIs;

static inline void
map_fluidsynth_uris(LV2_URID_Map* map, FluidSynthURIs* uris)
{
    uris->atom_Blank         = map->map(map->handle, LV2_ATOM__Blank);
    uris->atom_Path          = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_Resource      = map->map(map->handle, LV2_ATOM__Resource);
    uris->atom_Int           = map->map(map->handle, LV2_ATOM__Int);
    uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    uris->midi_Event         = map->map(map->handle, LV2_MIDI_EVENT_URI);
    uris->patch_Set          = map->map(map->handle, LV2_PATCH__Set);
    uris->patch_body         = map->map(map->handle, LV2_PATCH__body);
    uris->sf_loaded          = map->map(map->handle, FLUIDSYNTH__sf_loaded);
    uris->sf_file            = map->map(map->handle, FLUIDSYNTH__sf_file);
    uris->sf_preset_list     = map->map(map->handle, FLUIDSYNTH__sf_preset_list);
    uris->sf_preset_bank     = map->map(map->handle, FLUIDSYNTH__sf_preset_bank);
    uris->sf_preset_num      = map->map(map->handle, FLUIDSYNTH__sf_preset_num);
}

#endif
