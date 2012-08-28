#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib-object.h>
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
    GtkListStore* preset_num_model;
    GtkListStore* preset_bank_model;
    SoundFontData soundfont_data;
} FluidSynthGui;


static void sf_chosen(GtkWidget* widget,
                      void* data) {
    char* filename = gtk_file_chooser_get_filename((GtkFileChooser*)widget);
    FluidSynthGui* ui = (FluidSynthGui*)data;

    uint8_t obj_buf[512];
    lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 512);

    LV2_Atom_Forge_Frame set_frame;
    LV2_Atom* set_msg = (LV2_Atom*)lv2_atom_forge_resource(
        &ui->forge, &set_frame, 1, ui->uris.patch_Set);

    lv2_atom_forge_property_head(&ui->forge, ui->uris.patch_body, 0);
    LV2_Atom_Forge_Frame body_frame;
    lv2_atom_forge_blank(&ui->forge, &body_frame, 2, 0);

    lv2_atom_forge_property_head(&ui->forge, ui->uris.sf_file, 0);
    lv2_atom_forge_path(&ui->forge, filename, strlen(filename));

    lv2_atom_forge_pop(&ui->forge, &body_frame);
    lv2_atom_forge_pop(&ui->forge, &set_frame);

    ui->write_function(ui->controller, CONTROL, lv2_atom_total_size(set_msg),
              ui->uris.atom_eventTransfer,
              set_msg);

    g_free(filename);
}


static void bank_changed(GtkWidget* widget, void* data) {
    FluidSynthGui* ui = (FluidSynthGui*)data;
    FluidPresetListItem *curr;
    GtkTreeIter iter;
    int bank;

    gtk_list_store_clear(ui->preset_num_model);
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter);
    gtk_tree_model_get(GTK_TREE_MODEL(ui->preset_bank_model), &iter, 0, &bank, -1);
    for (curr=ui->soundfont_data.preset_list;curr;curr=curr->next) {
        if (bank == curr->fluidpreset->bank) {
            gtk_list_store_append(GTK_LIST_STORE(ui->preset_num_model), &iter);
            gtk_list_store_set(GTK_LIST_STORE(ui->preset_num_model), &iter, 0, curr->fluidpreset->name, 1, curr->fluidpreset->bank, 2, curr->fluidpreset->num, -1);
        }
    }
}

static void num_changed(GtkWidget *widget, void *data) {
    FluidSynthGui* ui = (FluidSynthGui*)data;
    GtkTreeIter iter;
    int bank, num;
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter);
    gtk_tree_model_get(GTK_TREE_MODEL(ui->preset_num_model), &iter, 1, &bank, 2, &num, -1);
    printf("%d, %d\n", bank, num);

    uint8_t obj_buf[256];
    lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 256);
    
    LV2_Atom* a;
    const uint8_t ev1[3] = { CONTROL_CHANGE, 0x00, bank};
    printf("%02x%02x%02x\n", ev1[0], ev1[1], ev1[2]);
    a = (LV2_Atom*)lv2_atom_forge_atom(&ui->forge, sizeof(ev1), ui->uris.midi_Event);
    lv2_atom_forge_write(&ui->forge, &ev1, sizeof(ev1));
    ui->write_function(ui->controller, CONTROL, lv2_atom_total_size(a), ui->uris.atom_eventTransfer, a);

    const uint8_t ev2[3] = { PROGRAM_CHANGE, (uint8_t)num, 0 };
    printf("%02x%02x%02x\n", ev2[0], ev2[1], ev2[2]);
    a = (LV2_Atom*)lv2_atom_forge_atom(&ui->forge, sizeof(ev2), ui->uris.midi_Event);
    lv2_atom_forge_write(&ui->forge, &ev2, sizeof(ev2));

    ui->write_function(ui->controller, CONTROL, lv2_atom_total_size(a),
              ui->uris.atom_eventTransfer, a);
}


static GtkWidget* make_gui(FluidSynthGui *pluginGui) {
    // Return a pointer to a gtk widget containing our GUI
    GtkWidget* container = gtk_hbox_new(FALSE, 4);
    GtkWidget* label = gtk_label_new("Soundfont");
    GtkWidget* sf_chooser = gtk_file_chooser_button_new("Select a soundfont",
                                        GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_button_set_width_chars((GtkFileChooserButton*)sf_chooser, 20);
    pluginGui->preset_num_model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);
    pluginGui->preset_bank_model = gtk_list_store_new(1, G_TYPE_UINT);

    GtkTreeIter list_iterator;

    GtkWidget* preset_num_chooser = gtk_combo_box_new_with_model(GTK_TREE_MODEL(pluginGui->preset_num_model));
    GtkWidget* preset_bank_chooser = gtk_combo_box_new_with_model(GTK_TREE_MODEL(pluginGui->preset_bank_model));
    GtkCellRenderer* num_cell = gtk_cell_renderer_text_new();
    GtkCellRenderer* bank_cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(preset_num_chooser), num_cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(preset_num_chooser), num_cell, "text", 0, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(preset_bank_chooser), bank_cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(preset_bank_chooser), bank_cell, "text", 0, NULL);

    gtk_box_pack_start(GTK_BOX(container), label, TRUE, TRUE, 4); 
    gtk_box_pack_start(GTK_BOX(container), (GtkWidget*)sf_chooser, FALSE, TRUE, 4); 
    gtk_box_pack_start(GTK_BOX(container), (GtkWidget*)preset_bank_chooser, FALSE, TRUE, 4); 
    gtk_box_pack_start(GTK_BOX(container), (GtkWidget*)preset_num_chooser, FALSE, TRUE, 4); 
    g_signal_connect(sf_chooser, "file-set", G_CALLBACK(sf_chosen), pluginGui);
    g_signal_connect(preset_bank_chooser, "changed", G_CALLBACK(bank_changed), pluginGui);
    g_signal_connect(preset_num_chooser, "changed", G_CALLBACK(num_changed), pluginGui);
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
    pluginGui->soundfont_data.name = NULL;

    *widget = (LV2UI_Widget)make_gui(pluginGui);
    return (LV2UI_Handle)pluginGui;
}

static void cleanup(LV2UI_Handle ui) {
    FluidSynthGui *pluginGui = (FluidSynthGui *) ui;
    free_soundfont_data(pluginGui->soundfont_data);
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
        char* sf_name = LV2_ATOM_BODY(sf_file);
        free_soundfont_data(pluginGui->soundfont_data);
        pluginGui->soundfont_data.name = malloc(1+strlen(sf_name));
        strcpy(pluginGui->soundfont_data.name, sf_name);

        const LV2_Atom_Object* sf_preset_list = NULL;
        lv2_atom_object_get(obj, pluginGui->uris.sf_preset_list, &sf_preset_list, 0);
        LV2_Atom_Vector_Body* presets  = (LV2_Atom_Vector_Body*)LV2_ATOM_BODY(sf_preset_list);

        LV2_Atom_Tuple* tup = (LV2_Atom_Tuple*)(1+presets);
        LV2_Atom* el;
        GtkTreeIter iter;
//        gtk_list_store_clear(pluginGui->preset_num_model);
//        gtk_list_store_clear(pluginGui->preset_bank_model); // This breaks stuff - something screwey is going on...
        int preset_bank, preset_num;
        char* preset_name;
        GtkTreePath* path;//      gtk_tree_path_new_from_string       (const gchar *path);
        char buf[3];
        FluidPresetListItem *prev=NULL, *fluid_preset_list_item;
        for(;;) {
            el = lv2_atom_tuple_begin(tup);
            preset_bank = *(int*)LV2_ATOM_BODY(el);
            el = lv2_atom_tuple_next(el);
            preset_num = *(int*)LV2_ATOM_BODY(el);
            el = lv2_atom_tuple_next(el);
            preset_name = (char*)LV2_ATOM_CONTENTS(LV2_Atom_String, el);
//            printf("received %d:%d:%s\n", preset_bank, preset_num, preset_name);
            fluid_preset_list_item = malloc(sizeof(FluidPresetListItem));
            if (prev) prev->next = fluid_preset_list_item;
            else pluginGui->soundfont_data.preset_list = fluid_preset_list_item;
            fluid_preset_list_item->fluidpreset = new_fluid_preset(preset_bank, preset_num, preset_name);
            fluid_preset_list_item->next = NULL;
            prev = fluid_preset_list_item;

            sprintf(buf, "%d", preset_bank);
//            printf("%d:%s\n", preset_bank, buf);
            if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(pluginGui->preset_bank_model), &iter, buf)) {
                gtk_list_store_append(GTK_LIST_STORE(pluginGui->preset_bank_model), &iter);
                gtk_list_store_set(GTK_LIST_STORE(pluginGui->preset_bank_model), &iter, 0, preset_bank, -1);
            }

            tup = (LV2_Atom_Tuple*)((char*)tup + lv2_atom_total_size((LV2_Atom*)tup));
            if (tup >= (LV2_Atom_Tuple*)((char*)presets + ((LV2_Atom_Vector*)sf_preset_list)->atom.size)) break;
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
