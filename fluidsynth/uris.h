#ifndef FLUIDSYNTH_URIS_H
#define FLUIDSYNTH_URIS_H

#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

#define LV2_MIDI_EVENT_URI      "http://lv2plug.in/ns/ext/midi#MidiEvent"
#define FLUIDSYNTH_URI          "http://www.joebutton.co.uk/software/lv2/fluidsynth"
#define FLUIDSYNTH_UI_URI       FLUIDSYNTH_URI "#ui"
#define FLUIDSYNTH__file        FLUIDSYNTH_URI "#file"


typedef struct {
    LV2_URID atom_Blank;
    LV2_URID atom_Path;
    LV2_URID atom_Resource;
    LV2_URID atom_Sequence;
    LV2_URID atom_eventTransfer;
    LV2_URID fluidsynth_file;
    LV2_URID midi_Event;
    LV2_URID patch_Set;
    LV2_URID patch_body;
} FluidSynthURIs;

static inline void
map_fluidsynth_uris(LV2_URID_Map* map, FluidSynthURIs* uris)
{
    uris->atom_Blank         = map->map(map->handle, LV2_ATOM__Blank);
    uris->atom_Path          = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_Resource      = map->map(map->handle, LV2_ATOM__Resource);
    uris->atom_Sequence      = map->map(map->handle, LV2_ATOM__Sequence);
    uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    uris->fluidsynth_file    = map->map(map->handle, FLUIDSYNTH__file);
    uris->midi_Event         = map->map(map->handle, LV2_MIDI_EVENT_URI);
    uris->patch_Set          = map->map(map->handle, LV2_PATCH__Set);
    uris->patch_body         = map->map(map->handle, LV2_PATCH__body);
}

static inline bool
is_object_type(const FluidSynthURIs* uris, LV2_URID type)
{
    return type == uris->atom_Resource
        || type == uris->atom_Blank;
}

static inline LV2_Atom*
write_set_file(LV2_Atom_Forge*    forge,
               const FluidSynthURIs* uris,
               const char*        filename,
               const size_t       filename_len)
{
    LV2_Atom_Forge_Frame set_frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_blank(
        forge, &set_frame, 1, uris->patch_Set);

    lv2_atom_forge_property_head(forge, uris->patch_body, 0);
    LV2_Atom_Forge_Frame body_frame;
    lv2_atom_forge_blank(forge, &body_frame, 2, 0);

    lv2_atom_forge_property_head(forge, uris->fluidsynth_file, 0);
    lv2_atom_forge_path(forge, filename, filename_len);

    lv2_atom_forge_pop(forge, &body_frame);
    lv2_atom_forge_pop(forge, &set_frame);

    return set;
}

static inline const LV2_Atom*
read_set_file(const FluidSynthURIs*     uris,
              const LV2_Atom_Object* obj)
{
    if (obj->body.otype != uris->patch_Set) {
        fprintf(stderr, "Ignoring unknown message type %d\n", obj->body.otype);
        return NULL;
    }

    /* Get body of message. */
    const LV2_Atom_Object* body = NULL;
    lv2_atom_object_get(obj, uris->patch_body, &body, 0);
    if (!body) {
        fprintf(stderr, "Malformed set message has no body.\n");
        return NULL;
    }
    if (!is_object_type(uris, body->atom.type)) {
        fprintf(stderr, "Malformed set message has non-object body.\n");
        return NULL;
    }

    /* Get file path from body. */
    const LV2_Atom* file_path = NULL;
    lv2_atom_object_get(body, uris->fluidsynth_file, &file_path, 0);
    if (!file_path) {
        fprintf(stderr, "Ignored set message with no file PATH.\n");
        return NULL;
    }

    return file_path;
}

#endif
