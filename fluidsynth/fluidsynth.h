#include "uris.h"

#ifndef FLUIDSYNTH_H
#define FLUIDSYNTH_H

// Ports
#define CONTROL                 0
#define LEFT                    1
#define RIGHT                   2
#define NOTIFY                  3
#define FLUID_TIME_SCALE        1000

// MIDI opcodes
#define NOTE_OFF                0x80
#define NOTE_ON                 0x90
#define KEY_PRESSURE            0xA0
#define CONTROL_CHANGE          0xB0
#define PROGRAM_CHANGE          0xC0
#define CHANNEL_PRESSURE        0xD0
#define PITCH_BEND              0xE0
#define MIDI_SYSTEM_RESET       0xFF


LV2_Atom* sf_load_atom(LV2_Atom_Forge *forge, FluidSynthURIs uris, const char* filename, const uint32_t *bank, const uint32_t *num) {
    LV2_Atom_Forge_Frame set_frame;
    LV2_Atom* set_msg = (LV2_Atom*)lv2_atom_forge_resource(
        forge, &set_frame, 1, uris.patch_Set);

    lv2_atom_forge_property_head(forge, uris.patch_body, 0);
    LV2_Atom_Forge_Frame body_frame;
    lv2_atom_forge_blank(forge, &body_frame, 2, 0);

    lv2_atom_forge_property_head(forge, uris.sf_file, 0);
    lv2_atom_forge_path(forge, filename, strlen(filename));

    if (bank) {
        lv2_atom_forge_property_head(forge, uris.sf_preset_bank, 0);
        lv2_atom_forge_int(forge, *bank);
    }
    if (num) {
        lv2_atom_forge_property_head(forge, uris.sf_preset_num, 0);
        lv2_atom_forge_int(forge, *num);
    }

    lv2_atom_forge_pop(forge, &body_frame);
    lv2_atom_forge_pop(forge, &set_frame);
    return set_msg;
}


typedef struct {
    int bank;
    int num;
    char* name;
} FluidPreset;


typedef struct FluidPresetListItem {
    FluidPreset* fluidpreset;
    struct FluidPresetListItem* next;
} FluidPresetListItem;


typedef struct {
    char* name;
    FluidPresetListItem* preset_list;
} SoundFontData;


FluidPreset* new_fluid_preset(int bank, int num, char* name) {
    FluidPreset *fp = malloc(sizeof(FluidPreset));
    if (!fp) return NULL;
    fp->bank = bank;
    fp->num = num;
    fp->name = (char*)malloc(1 + strlen(name));
    if (!fp->name) return NULL;
    strcpy(fp->name, name);
    return fp;
}


int free_soundfont_data(SoundFontData soundfont_data) {
    if (soundfont_data.name) free(soundfont_data.name);
    FluidPresetListItem *curr, *next;
    for (curr=soundfont_data.preset_list;curr;) {
        free(curr->fluidpreset->name);
        free(curr->fluidpreset);
        next = curr->next;
        free(curr);
        curr = next;
    }
}

#endif
