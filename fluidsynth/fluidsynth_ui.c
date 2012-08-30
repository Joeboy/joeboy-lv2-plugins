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
    GtkTreeStore *preset_bank_store;
    GtkTreeStore *preset_num_store;
    SoundFontData soundfont_data;
} Ui;


static void sf_chosen(GtkWidget* widget,
                      void* data) {
    char* filename = gtk_file_chooser_get_filename((GtkFileChooser*)widget);
    Ui* ui = (Ui*)data;

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
    Ui* ui = (Ui*)data;
    FluidPresetListItem *curr;
    GtkTreeIter iter;
    int bank;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
        gtk_tree_store_clear(ui->preset_num_store);
        gtk_tree_model_get(GTK_TREE_MODEL(ui->preset_bank_store), &iter, 0, &bank, -1);
        for (curr=ui->soundfont_data.preset_list;curr;curr=curr->next) {
            if (bank == curr->fluidpreset->bank) {
                gtk_tree_store_append(ui->preset_num_store, &iter, NULL);
                gtk_tree_store_set(ui->preset_num_store, &iter, 0, curr->fluidpreset->name, 1, curr->fluidpreset->bank, 2, curr->fluidpreset->num, -1);
            }
        }
    }
}

static void num_changed(GtkWidget *widget, void *data) {
    Ui* ui = (Ui*)data;
    GtkTreeIter iter;
    int bank, num;
    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
        gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(widget)), &iter, 1, &bank, 2, &num, -1);
//        printf("%d:%d\n", bank, num);

        uint8_t obj_buf[32];
        lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 256);
        
        LV2_Atom* a;
        const uint8_t ev1[3] = { CONTROL_CHANGE, 0x00, bank};
        a = (LV2_Atom*)lv2_atom_forge_atom(&ui->forge, sizeof(ev1), ui->uris.midi_Event);
        lv2_atom_forge_write(&ui->forge, &ev1, sizeof(ev1));
        ui->write_function(ui->controller, CONTROL, lv2_atom_total_size(a), ui->uris.atom_eventTransfer, a);

        const uint8_t ev2[3] = { PROGRAM_CHANGE, (uint8_t)num, 0 };
        a = (LV2_Atom*)lv2_atom_forge_atom(&ui->forge, sizeof(ev2), ui->uris.midi_Event);
        lv2_atom_forge_write(&ui->forge, &ev2, sizeof(ev2));

        ui->write_function(ui->controller, CONTROL, lv2_atom_total_size(a),
                  ui->uris.atom_eventTransfer, a);
    }
}


static GtkWidget* make_gui(Ui *ui) {
    // Return a pointer to a gtk widget containing our GUI
    GtkWidget* container = gtk_hbox_new(FALSE, 4);
    GtkWidget* label = gtk_label_new("Soundfont");
    GtkWidget* sf_chooser = gtk_file_chooser_button_new("Select a soundfont",
                                        GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_button_set_width_chars((GtkFileChooserButton*)sf_chooser, 20);
    ui->preset_bank_store = gtk_tree_store_new(1, G_TYPE_UINT);
    ui->preset_num_store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);
    GtkWidget* preset_bank_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ui->preset_bank_store));
    GtkWidget* preset_num_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ui->preset_num_store));

    GtkCellRenderer* bank_cell = gtk_cell_renderer_text_new();
    GtkCellRenderer* num_cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(preset_bank_combo), bank_cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(preset_bank_combo), bank_cell, "text", 0, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(preset_num_combo), num_cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(preset_num_combo), num_cell, "text", 0, NULL);

    gtk_box_pack_start(GTK_BOX(container), label, TRUE, TRUE, 4); 
    gtk_box_pack_start(GTK_BOX(container), (GtkWidget*)sf_chooser, FALSE, TRUE, 4); 
    gtk_box_pack_start(GTK_BOX(container), (GtkWidget*)preset_bank_combo, FALSE, TRUE, 4); 
    gtk_box_pack_start(GTK_BOX(container), (GtkWidget*)preset_num_combo, FALSE, TRUE, 4); 
    g_signal_connect(sf_chooser, "file-set", G_CALLBACK(sf_chosen), ui);
    g_signal_connect(preset_bank_combo, "changed", G_CALLBACK(bank_changed), ui);
    g_signal_connect(preset_num_combo, "changed", G_CALLBACK(num_changed), ui);
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

    Ui* ui = (Ui*)malloc(sizeof(Ui));
    if (ui == NULL) return NULL;

    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            ui->map = (LV2_URID_Map*)features[i]->data;
        }
    }

    if (!ui->map) {
        fprintf(stderr, "sampler_ui: Host does not support urid:Map\n");
        free(ui);
        return NULL;
    }

    map_fluidsynth_uris(ui->map, &ui->uris);
    lv2_atom_forge_init(&ui->forge, ui->map);

    ui->controller = controller;
    ui->write_function = write_function;
    ui->soundfont_data.name = NULL;

    *widget = (LV2UI_Widget)make_gui(ui);
    return (LV2UI_Handle)ui;
}

static void cleanup(LV2UI_Handle ui_handle) {
    Ui *ui = (Ui *) ui_handle;
    free_soundfont_data(ui->soundfont_data);
    free(ui);
}

static void port_event(LV2UI_Handle ui_handle,
               uint32_t port_index,
               uint32_t buffer_size,
               uint32_t format,
               const void * buffer) {

    Ui *ui = (Ui*) ui_handle;
    if (format==ui->uris.atom_eventTransfer) {
        LV2_Atom_Object* obj = (LV2_Atom_Object*) buffer;
        if (obj->body.otype != ui->uris.sf_loaded) {
            fprintf(stderr, "Ignoring unknown message type %d\n", obj->body.otype);
            return;
        }
        const LV2_Atom_Object* sf_file = NULL;
        lv2_atom_object_get(obj, ui->uris.sf_file, &sf_file, 0);
        if (!sf_file) {
            fprintf(stderr, "Port event error: Malformed set message has no sf_file.\n");
            return;
        }
        char* sf_name = LV2_ATOM_BODY(sf_file);
        free_soundfont_data(ui->soundfont_data);
        ui->soundfont_data.name = malloc(1+strlen(sf_name));
        strcpy(ui->soundfont_data.name, sf_name);

        const LV2_Atom_Object* sf_preset_list = NULL;
        lv2_atom_object_get(obj, ui->uris.sf_preset_list, &sf_preset_list, 0);
        LV2_Atom_Vector_Body* presets  = (LV2_Atom_Vector_Body*)LV2_ATOM_BODY(sf_preset_list);

        LV2_Atom_Tuple* tup = (LV2_Atom_Tuple*)(1+presets);
        LV2_Atom* el;
        GtkTreeIter iter;
        int preset_bank, preset_num, prev_preset_bank=-1;
        char* preset_name;
        FluidPresetListItem *prev=NULL, *fluid_preset_list_item;
        gtk_tree_store_clear(ui->preset_bank_store);
        gtk_tree_store_clear(ui->preset_num_store);
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
            else ui->soundfont_data.preset_list = fluid_preset_list_item;
            fluid_preset_list_item->fluidpreset = new_fluid_preset(preset_bank, preset_num, preset_name);
            fluid_preset_list_item->next = NULL;
            prev = fluid_preset_list_item;

            if (preset_bank != prev_preset_bank) {
                gtk_tree_store_append(ui->preset_bank_store, &iter, NULL);
                gtk_tree_store_set(ui->preset_bank_store, &iter, 0, preset_bank, -1);
            }

            tup = (LV2_Atom_Tuple*)((char*)tup + lv2_atom_total_size((LV2_Atom*)tup));
            if (tup >= (LV2_Atom_Tuple*)((char*)presets + ((LV2_Atom_Vector*)sf_preset_list)->atom.size)) break;
            prev_preset_bank = preset_bank;
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
