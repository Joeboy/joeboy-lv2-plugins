#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2_ui.h"
#include "fluidsynth.h"
#include "uris.h"



typedef struct {
    LV2_Atom_Forge forge;
    LV2UI_Controller controller;
    LV2UI_Write_Function write_function;
    FluidSynthURIs uris;
    LV2_URID_Map* map;
} FluidSynthGui;

static void sf_chosen(GtkWidget* widget,
                      void* data) {
    char* filename = gtk_file_chooser_get_filename((GtkFileChooser*)widget);
    FluidSynthGui* ui = (FluidSynthGui*)data;

    #define OBJ_BUF_SIZE 1024
    uint8_t obj_buf[OBJ_BUF_SIZE];
    lv2_atom_forge_set_buffer(&ui->forge, obj_buf, OBJ_BUF_SIZE);

    LV2_Atom* msg = write_set_file(&ui->forge, &ui->uris,
                                   filename, strlen(filename));

    ui->write_function(ui->controller, CONTROL, lv2_atom_total_size(msg),
              ui->uris.atom_eventTransfer,
              msg);

    g_free(filename);
}


static GtkWidget* make_gui(FluidSynthGui *pluginGui) {
    // Return a pointer to a gtk widget containing our GUI
    GtkWidget* container = gtk_hbox_new(FALSE, 4);
    GtkWidget* label = gtk_label_new("Soundfont");
    GtkWidget* sf_chooser = gtk_file_chooser_button_new("Select a soundfont",
                                        GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_button_set_width_chars((GtkFileChooserButton*)sf_chooser, 20);
    gtk_box_pack_start(GTK_BOX(container), label, TRUE, TRUE, 4); 
    gtk_box_pack_start(GTK_BOX(container), (GtkWidget*)sf_chooser, FALSE, TRUE, 4); 
    g_signal_connect(sf_chooser, "file-set",
                     G_CALLBACK(sf_chosen),
                     pluginGui);
    return container;
}


static LV2UI_Handle instantiate(const struct _LV2UI_Descriptor * descriptor,
                const char * plugin_uri,
                const char * bundle_path,
                LV2UI_Write_Function write_function,
                LV2UI_Controller controller,
                LV2UI_Widget * widget,
                const LV2_Feature * const * features) {

    if (strcmp(plugin_uri, FLUIDSYNTH_URI) != 0) {
        fprintf(stderr, "Fluidsynth UI error: this GUI does not support plugin with URI %s\n", plugin_uri);
        return NULL;
    }

    FluidSynthGui* pluginGui = (FluidSynthGui*)malloc(sizeof(FluidSynthGui));
    if (pluginGui == NULL) return NULL;

    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            pluginGui->map = (LV2_URID_Map*)features[i]->data;
        }
    }

    if (!pluginGui->map) {
        fprintf(stderr, "sampler_ui: Host does not support urid:Map\n");
        free(pluginGui);
        return NULL;
    }

    map_fluidsynth_uris(pluginGui->map, &pluginGui->uris);
    lv2_atom_forge_init(&pluginGui->forge, pluginGui->map);

    pluginGui->controller = controller;
    pluginGui->write_function = write_function;

    *widget = (LV2UI_Widget)make_gui(pluginGui);
    return (LV2UI_Handle)pluginGui;
}

static void cleanup(LV2UI_Handle ui) {
    FluidSynthGui *pluginGui = (FluidSynthGui *) ui;
    free(pluginGui);
}

static void port_event(LV2UI_Handle ui,
               uint32_t port_index,
               uint32_t buffer_size,
               uint32_t format,
               const void * buffer) {

    FluidSynthGui *pluginGui = (FluidSynthGui *) ui;
    if (format==pluginGui->uris.atom_eventTransfer) {
        LV2_Atom_Object* obj = (LV2_Atom_Object*) buffer;
        if (obj->body.otype != pluginGui->uris.sf_loaded) {
            fprintf(stderr, "Ignoring unknown message type %d\n", obj->body.otype);
            return;
        }
        const LV2_Atom_Object* sf_file = NULL;
        lv2_atom_object_get(obj, pluginGui->uris.sf_file, &sf_file, 0);
        if (!sf_file) {
            fprintf(stderr, "Port event error: Malformed set message has no sf_file.\n");
            return;
        }
        printf("%s\n", (char*)LV2_ATOM_BODY(sf_file));

        const LV2_Atom_Object* sf_preset_list = NULL;
        lv2_atom_object_get(obj, pluginGui->uris.sf_preset_list, &sf_preset_list, 0);
        LV2_Atom_Vector_Body* presets  = (LV2_Atom_Vector_Body*)LV2_ATOM_BODY(sf_preset_list);

        LV2_Atom_Tuple* tup = (char*)(1+presets);
        LV2_Atom* el;
        for(;;) {
            LV2_Atom* el = lv2_atom_tuple_begin(tup);
            printf("var0 = %d\n", *(int*)LV2_ATOM_BODY(el));
            el = lv2_atom_tuple_next(el);
            printf("var1 = %d\n", *(int*)LV2_ATOM_BODY(el));
            el = lv2_atom_tuple_next(el);
            printf("var2 = %s\n", (char*)LV2_ATOM_CONTENTS(LV2_Atom_String, el));
            tup = (char*)tup + lv2_atom_total_size(tup);
            if (tup >= (char*)presets + ((LV2_Atom_Vector*)sf_preset_list)->atom.size) break;
        }

    } else {
        printf("Unknown format\n");
    }
}

static LV2UI_Descriptor descriptors[] = {
    {FLUIDSYNTH_UI_URI, instantiate, cleanup, port_event, NULL}
};

const LV2UI_Descriptor * lv2ui_descriptor(uint32_t index) {
    printf("lv2ui_descriptor(%u) called\n", (unsigned int)index); 
    if (index >= sizeof(descriptors) / sizeof(descriptors[0])) {
        return NULL;
    }
    return &descriptors[index];
}
