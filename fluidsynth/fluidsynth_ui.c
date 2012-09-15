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
    GtkWidget* preset_num_combo;
    GtkWidget* preset_bank_combo;
    SoundFontData soundfont_data;
} Ui;


static void sf_chosen(GtkWidget* widget,
                      void* data) {
    char* filename = gtk_file_chooser_get_filename((GtkFileChooser*)widget);
    Ui* ui = (Ui*)data;

    uint8_t obj_buf[512];
    lv2_atom_forge_set_buffer(&ui->forge, obj_buf, 512);
    LV2_Atom *set_msg = sf_load_atom(&ui->forge, ui->uris, filename, NULL, NULL);


    ui->write_function(ui->controller, CONTROL, lv2_atom_total_size(set_msg),
              ui->uris.atom_eventTransfer,
              set_msg);

    g_free(filename);
}


static void update_presets(LV2UI_Handle ui_handle, uint32_t selected_bank, int32_t selected_num) {
    // Set the preset num combobox entries to reflect the selected bank
    Ui *ui = (Ui*)ui_handle;
    FluidPresetListItem *curr;
    GtkTreeIter iter;

    gtk_tree_store_clear(ui->preset_num_store);
    for (curr=ui->soundfont_data.preset_list;curr;curr=curr->next) {
        if (selected_bank == curr->fluidpreset->bank) {
            gtk_tree_store_append(ui->preset_num_store, &iter, NULL);
            gtk_tree_store_set(ui->preset_num_store, &iter, 0, curr->fluidpreset->name, 1, curr->fluidpreset->bank, 2, curr->fluidpreset->num, -1);
            if (selected_num == curr->fluidpreset->num) {
                gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ui->preset_num_combo), &iter);
            }
        }
    }
}


static void bank_changed(GtkWidget* widget, void* data) {
    Ui* ui = (Ui*)data;
    GtkTreeIter iter;
    int bank;
    
    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(ui->preset_bank_store), &iter, 0, &bank, -1);
        update_presets(ui, bank, -1);
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
    GtkWidget *container = gtk_table_new(3, 2, 0);
    GtkWidget* soundfont_label = gtk_label_new("Soundfont");
    gtk_misc_set_alignment(GTK_MISC(soundfont_label), 1, 0);
    GtkWidget* preset_bank_label = gtk_label_new("Bank");
    gtk_misc_set_alignment(GTK_MISC(preset_bank_label), 1, 0);
    GtkWidget* preset_num_label = gtk_label_new("Preset");
    gtk_misc_set_alignment(GTK_MISC(preset_num_label), 1, 0);
    GtkWidget *sf_chooser = gtk_file_chooser_button_new("Select a soundfont", GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter *sf_chooser_sf_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(sf_chooser_sf_filter, "Soundfonts (.sf2)");
    gtk_file_filter_add_pattern(sf_chooser_sf_filter, "*.sf2");
    gtk_file_filter_add_pattern(sf_chooser_sf_filter, "*.SF2");
    gtk_file_filter_add_pattern(sf_chooser_sf_filter, "*.Sf2");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(sf_chooser), sf_chooser_sf_filter);
    GtkFileFilter *sf_chooser_all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(sf_chooser_all_filter, "All files");
    gtk_file_filter_add_pattern(sf_chooser_all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(sf_chooser), sf_chooser_all_filter);
    gtk_file_chooser_button_set_width_chars((GtkFileChooserButton*)sf_chooser, 20);
    ui->preset_bank_store = gtk_tree_store_new(1, G_TYPE_UINT);
    ui->preset_num_store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);
    ui->preset_bank_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ui->preset_bank_store));
    ui->preset_num_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ui->preset_num_store));

    GtkCellRenderer* bank_cell = gtk_cell_renderer_text_new();
    GtkCellRenderer* num_cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ui->preset_bank_combo), bank_cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ui->preset_bank_combo), bank_cell, "text", 0, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ui->preset_num_combo), num_cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ui->preset_num_combo), num_cell, "text", 0, NULL);

    gtk_table_attach(GTK_TABLE(container), soundfont_label, 0, 1, 0, 1, GTK_FILL, GTK_SHRINK, 3, 3);
    gtk_table_attach(GTK_TABLE(container), sf_chooser, 1, 2, 0, 1, GTK_FILL, GTK_SHRINK, 3,3);
    gtk_table_attach(GTK_TABLE(container), preset_bank_label, 0, 1, 1, 2, GTK_FILL, GTK_SHRINK, 3, 3);
    gtk_table_attach(GTK_TABLE(container), ui->preset_bank_combo, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 3, 3);
    gtk_table_attach(GTK_TABLE(container), preset_num_label, 0, 1, 2, 3, GTK_FILL, GTK_SHRINK, 3, 3);
    gtk_table_attach(GTK_TABLE(container), ui->preset_num_combo, 1, 2, 2, 3, GTK_FILL, GTK_SHRINK, 3, 3);
    g_signal_connect(sf_chooser, "file-set", G_CALLBACK(sf_chosen), ui);
    g_signal_connect(ui->preset_bank_combo, "changed", G_CALLBACK(bank_changed), ui);
    g_signal_connect(ui->preset_num_combo, "changed", G_CALLBACK(num_changed), ui);
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
        if (!lv2_atom_object_get(obj, ui->uris.sf_file, &sf_file, 0)) {
            fprintf(stderr, "Port event error: Malformed set message has no sf_file.\n");
            return;
        }
        char* sf_name = LV2_ATOM_BODY(sf_file);
        free_soundfont_data(ui->soundfont_data);
        ui->soundfont_data.name = malloc(1+strlen(sf_name));
        strcpy(ui->soundfont_data.name, sf_name);

        LV2_Atom_Object *selected_bank_atom = NULL, *selected_num_atom = NULL;
        uint32_t selected_bank, selected_num;
        if (lv2_atom_object_get(obj, ui->uris.sf_preset_bank, &selected_bank_atom, 0)) {
            selected_bank = *(uint32_t*)LV2_ATOM_BODY(selected_bank_atom);
        }
        if (lv2_atom_object_get(obj, ui->uris.sf_preset_num, &selected_num_atom, 0)) {
            selected_num = *(uint32_t*)LV2_ATOM_BODY(selected_num_atom);
        }

        const LV2_Atom_Object* sf_preset_list = NULL;
        lv2_atom_object_get(obj, ui->uris.sf_preset_list, &sf_preset_list, 0);
        LV2_Atom_Vector_Body* presets  = (LV2_Atom_Vector_Body*)LV2_ATOM_BODY(sf_preset_list);

        LV2_Atom_Tuple* tup = (LV2_Atom_Tuple*)(1+presets);
        LV2_Atom* el;
        GtkTreeIter iter;
        int32_t preset_bank, preset_num, prev_preset_bank=-1;
        char* preset_name;
        FluidPresetListItem *prev=NULL, *fluid_preset_list_item;
        gtk_tree_store_clear(ui->preset_bank_store);
        gtk_tree_store_clear(ui->preset_num_store);
        for(;;) {
            el = lv2_atom_tuple_begin(tup);
            preset_bank = *(uint32_t*)LV2_ATOM_BODY(el);
            el = lv2_atom_tuple_next(el);
            preset_num = *(uint32_t*)LV2_ATOM_BODY(el);
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
                if (selected_bank_atom && selected_bank == preset_bank) {
                    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ui->preset_bank_combo), &iter);
                }
            }

            tup = (LV2_Atom_Tuple*)((char*)tup + lv2_atom_total_size((LV2_Atom*)tup));
            if (tup >= (LV2_Atom_Tuple*)((char*)presets + ((LV2_Atom_Vector*)sf_preset_list)->atom.size)) break;
            prev_preset_bank = preset_bank;
        }
        if (selected_bank_atom) {
            if (selected_num_atom) {
                update_presets(ui, selected_bank, selected_num);
            } else {
                update_presets(ui, selected_bank, -1);
            }
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
