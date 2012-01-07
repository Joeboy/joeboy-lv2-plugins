#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "lv2_ui.h"
#include "amp.h"


typedef struct {
    GtkWidget *volume_control;
    float gain;

    LV2UI_Controller controller;
    LV2UI_Write_Function write_function;
} AmpGui;


static int value_changed_cb(GtkWidget * widget, gpointer data) {
    AmpGui *pluginGui = (AmpGui *) data;
    pluginGui->gain = (float)gtk_range_get_value((GtkRange*)widget);
    pluginGui->write_function(pluginGui->controller, AMP_GAIN, sizeof(pluginGui->gain), 0, &pluginGui->gain);
}


static GtkWidget* make_gui(AmpGui *pluginGui) {
    // Return a pointer to a gtk widget containing our GUI
    GtkWidget* container = gtk_vbox_new(FALSE, 2);
    pluginGui->volume_control = gtk_vscale_new_with_range(-90, 24, 1);
    gtk_range_set_inverted((GtkRange*)pluginGui->volume_control, TRUE);
    gtk_range_set_value((GtkRange*)pluginGui->volume_control, 0);
    g_signal_connect_after(pluginGui->volume_control, "value_changed",
             G_CALLBACK(value_changed_cb), pluginGui);
    gtk_container_add((GtkContainer*)container, (GtkWidget*)pluginGui->volume_control);
    gtk_widget_set_size_request(container, 50,200);
    return container;
}


static LV2UI_Handle instantiate(const struct _LV2UI_Descriptor * descriptor,
                const char * plugin_uri,
                const char * bundle_path,
                LV2UI_Write_Function write_function,
                LV2UI_Controller controller,
                LV2UI_Widget * widget,
                const LV2_Feature * const * features) {

    if (strcmp(plugin_uri, AMP_URI) != 0) {
        fprintf(stderr, "AMP_UI error: this GUI does not support plugin with URI %s\n", plugin_uri);
        return NULL;
    }

    AmpGui* pluginGui = (AmpGui*)malloc(sizeof(AmpGui));
    if (pluginGui == NULL) return NULL;

    pluginGui->controller = controller;
    pluginGui->write_function = write_function;

    *widget = (LV2UI_Widget)make_gui(pluginGui);
    return (LV2UI_Handle)pluginGui;
}

static void cleanup(LV2UI_Handle ui) {
    printf("cleanup()\n");
    AmpGui *pluginGui = (AmpGui *) ui;
    free(pluginGui);
}

static void port_event(LV2UI_Handle ui,
               uint32_t port_index,
               uint32_t buffer_size,
               uint32_t format,
               const void * buffer) {

    AmpGui *pluginGui = (AmpGui *) ui;
    float * pval = (float *)buffer;
    printf("port_event(%u, %p) called\n", (unsigned int)port_index, buffer);

    if (format != 0) {
        return;
    }

    if ((port_index < 0) || (port_index >= AMP_N_PORTS)) {
        return;
    }

//    if (!set_port_value(cp, port_index, *pval)) {
//        return;
//    }
//    cp->port_buffer[port_index] = *pval;
//
//    if (!cp->ir->first_conf_done) {
//        port_event_t * pe = (port_event_t *)malloc(sizeof(port_event_t));
//        pe->port_index = port_index;
//        pe->value = *pval;
//        cp->port_event_q = g_slist_prepend(cp->port_event_q, pe);
//        return;
//    }
//
//    int update_ui = 0;
//    if (port_index == IR_PORT_REVERSE) {
//        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cp->toggle_reverse),
//                         (*pval > 0.0f));
//        update_ui = 1;
//    } else if (port_index == IR_PORT_PREDELAY) {
//        cp->predelay = *pval;
//        set_adjustment(cp, cp->adj_predelay, *pval);
//        update_ui = 1;
//    } else if (port_index == IR_PORT_ATTACK) {
//        cp->attack = *pval;
//        set_adjustment(cp, cp->adj_attack, *pval);
//        update_ui = 1;
//    } else if (port_index == IR_PORT_ATTACKTIME) {
//        cp->attacktime = *pval;
//        set_adjustment(cp, cp->adj_attacktime, *pval);
//        update_ui = 1;
//    } else if (port_index == IR_PORT_ENVELOPE) {
//        cp->envelope = *pval;
//        set_adjustment(cp, cp->adj_envelope, *pval);
//        update_ui = 1;
//    } else if (port_index == IR_PORT_LENGTH) {
//        cp->length = *pval;
//        set_adjustment(cp, cp->adj_length, *pval);
//        update_ui = 1;
//    } else if (port_index == IR_PORT_STRETCH) {
//        cp->stretch = *pval;
//        set_adjustment(cp, cp->adj_stretch, *pval);
//        update_ui = 1;
//    } else if (port_index == IR_PORT_STEREO_IN) {
//        set_adjustment(cp, cp->adj_stereo_in, *pval);
//    } else if (port_index == IR_PORT_STEREO_IR) {
//        cp->stereo_ir = *pval;
//        set_adjustment(cp, cp->adj_stereo_ir, *pval);
//    } else if (port_index == IR_PORT_AGC_SW) {
//        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cp->toggle_agc_sw),
//                         (*pval > 0.0f));
//    } else if (port_index == IR_PORT_DRY_SW) {
//        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cp->toggle_dry_sw),
//                         (*pval > 0.0f));
//    } else if (port_index == IR_PORT_DRY_GAIN) {
//        set_adjustment(cp, cp->adj_dry_gain, *pval);
//    } else if (port_index == IR_PORT_WET_SW) {
//        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cp->toggle_wet_sw),
//                         (*pval > 0.0f));
//    } else if (port_index == IR_PORT_WET_GAIN) {
//        set_adjustment(cp, cp->adj_wet_gain, *pval);
//    } else if (port_index == IR_PORT_FHASH_0) { /* NOP: plugin itself handles IR loading on session resume */
//    } else if (port_index == IR_PORT_FHASH_1) { /* NOP */
//    } else if (port_index == IR_PORT_FHASH_2) { /* NOP */
//    } else if (port_index == IR_PORT_METER_DRY_L) {
//        ir_meter_set_level(IR_METER(cp->meter_L_dry), convert_real_to_scale(ADJ_DRY_GAIN, CO_DB(*pval)));
//    } else if (port_index == IR_PORT_METER_DRY_R) {
//        ir_meter_set_level(IR_METER(cp->meter_R_dry), convert_real_to_scale(ADJ_DRY_GAIN, CO_DB(*pval)));
//    } else if (port_index == IR_PORT_METER_WET_L) {
//        ir_meter_set_level(IR_METER(cp->meter_L_wet), convert_real_to_scale(ADJ_WET_GAIN, CO_DB(*pval)));
//    } else if (port_index == IR_PORT_METER_WET_R) {
//        ir_meter_set_level(IR_METER(cp->meter_R_wet), convert_real_to_scale(ADJ_WET_GAIN, CO_DB(*pval)));
//    }
//
//    if (update_ui) {
//        update_envdisplay(cp);
//    }
}

static LV2UI_Descriptor descriptors[] = {
    {AMP_UI_URI, instantiate, cleanup, port_event, NULL}
};

const LV2UI_Descriptor * lv2ui_descriptor(uint32_t index) {
    printf("lv2ui_descriptor(%u) called\n", (unsigned int)index); 
    if (index >= sizeof(descriptors) / sizeof(descriptors[0])) {
        return NULL;
    }
    return descriptors + index;
}
