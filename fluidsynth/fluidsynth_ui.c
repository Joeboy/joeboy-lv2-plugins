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


static void
on_load_clicked(GtkWidget* widget,
                void*      handle)
{
    FluidSynthGui* ui = (FluidSynthGui*)handle;

    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Load SoundFont",
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);

    /* Run the dialog, and return if it is cancelled. */
    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return;
    }

    /* Get the file path from the dialog. */
    char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    /* Got what we need, destroy the dialog. */
    gtk_widget_destroy(dialog);
//    printf("%s\n", filename);

    #define OBJ_BUF_SIZE 1024
    uint8_t obj_buf[OBJ_BUF_SIZE];
    lv2_atom_forge_set_buffer(&ui->forge, obj_buf, OBJ_BUF_SIZE);

    LV2_Atom* msg = write_set_file(&ui->forge, &ui->uris,
                                   filename, strlen(filename));

    ui->write_function(ui->controller, CONTROL, lv2_atom_total_size(msg),
              ui->uris.atom_eventTransfer,
              msg);
//    ui->write_function(ui->controller, CONTROL, 4, ui->uris.atom_eventTransfer, obj_buf);

    g_free(filename);
}


static GtkWidget* make_gui(FluidSynthGui *pluginGui) {
    // Return a pointer to a gtk widget containing our GUI
    GtkWidget* container = gtk_hbox_new(FALSE, 4);
    GtkWidget* label = gtk_label_new("?");
    GtkWidget* button = gtk_button_new_with_label("Load SF");
    gtk_box_pack_start(GTK_BOX(container), label, TRUE, TRUE, 4); 
    gtk_box_pack_start(GTK_BOX(container), button, FALSE, TRUE, 4); 
    g_signal_connect(button, "clicked",
                     G_CALLBACK(on_load_clicked),
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
    printf("cleanup()\n");
    FluidSynthGui *pluginGui = (FluidSynthGui *) ui;
    free(pluginGui);
}

static void port_event(LV2UI_Handle ui,
               uint32_t port_index,
               uint32_t buffer_size,
               uint32_t format,
               const void * buffer) {

    FluidSynthGui *pluginGui = (FluidSynthGui *) ui;
    float * pval = (float *)buffer;
    printf("port_event(%u, %p) called\n", (unsigned int)port_index, buffer);
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
