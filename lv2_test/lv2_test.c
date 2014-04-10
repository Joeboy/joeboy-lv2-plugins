/*
  Copyright 2014 Joe Button
  Based on code by David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lv2.h>
#include "lilv/lilv.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "uri_table.h"
#include "lv2_test.h"

static LilvNode* atom_AtomPort   = NULL;
static LilvNode* atom_Sequence   = NULL;
static LilvNode* lv2_AudioPort   = NULL;
static LilvNode* lv2_CVPort      = NULL;
static LilvNode* lv2_ControlPort = NULL;
static LilvNode* lv2_InputPort   = NULL;
static LilvNode* lv2_OutputPort  = NULL;
static LilvNode* urid_map        = NULL;

int run_plugin(Lv2TestSetup setup) {
    LilvWorld* world = lilv_world_new();
    lilv_world_load_all(world);

    atom_AtomPort   = lilv_new_uri(world, LV2_ATOM__AtomPort);
    atom_Sequence   = lilv_new_uri(world, LV2_ATOM__Sequence);
    lv2_AudioPort   = lilv_new_uri(world, LV2_CORE__AudioPort);
    lv2_CVPort      = lilv_new_uri(world, LV2_CORE__CVPort);
    lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);
    lv2_InputPort   = lilv_new_uri(world, LV2_CORE__InputPort);
    lv2_OutputPort  = lilv_new_uri(world, LV2_CORE__OutputPort);
    urid_map        = lilv_new_uri(world, LV2_URID__map);
    URITable uri_table;
    uri_table_init(&uri_table);

    LV2_URID_Map       map           = { &uri_table, uri_table_map };
    LV2_Feature        map_feature   = { LV2_URID_MAP_URI, &map };
    LV2_URID_Unmap     unmap         = { &uri_table, uri_table_unmap };
    LV2_Feature        unmap_feature = { LV2_URID_UNMAP_URI, &unmap };
    const LV2_Feature* features[]    = { &map_feature, &unmap_feature, NULL };

    const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
    const LilvPlugin* plugin = lilv_plugins_get_by_uri (plugins, lilv_new_uri(world, setup.plugin_uri));

    const char* plugin_uri = lilv_node_as_string(lilv_plugin_get_uri(plugin));
    const char* plugin_name = lilv_node_as_string(lilv_plugin_get_name(plugin));

//    LV2_Atom_Sequence seq = {
//        { sizeof(LV2_Atom_Sequence_Body),
//          uri_table_map(&uri_table, LV2_ATOM__Sequence) },
//        { 0, 0 } };

    LilvNodes*  required = lilv_plugin_get_required_features(plugin);
    LILV_FOREACH(nodes, i, required) {
        const LilvNode* feature = lilv_nodes_get(required, i);
        if (!lilv_node_equals(feature, urid_map)) {
            fprintf(stderr, "<%s> requires feature <%s>, skipping\n",
                    plugin_uri, lilv_node_as_uri(feature));
            return 0;
        }
    }

    LilvInstance* instance = lilv_plugin_instantiate(plugin, setup.sample_rate, features);
    if (!instance) {
        fprintf(stderr, "Failed to instantiate <%s>\n",
                lilv_node_as_uri(lilv_plugin_get_uri(plugin)));
        return 0;
    }

//    float* controls = (float*)calloc(lilv_plugin_get_num_ports(plugin), sizeof(float));
//    lilv_plugin_get_port_ranges_float(plugin, NULL, NULL, controls);

    const uint32_t n_ports = lilv_plugin_get_num_ports(plugin);
    for (uint32_t index=0;index < n_ports; ++index) {
        // Does our setup contain a port buffer for this port?
        int found = 0;
        for (Lv2PortBufData** portdata = setup.lv2_port_buffers; *portdata; portdata++) {
            if ((*portdata)->index == index) {
                found = 1;
                lilv_instance_connect_port(instance, index, (*portdata)->data);
//                printf("%d: %f\n", index, *(float*)(*portdata)->data);
//                printf("%d: %x\n", index, (*portdata)->data);
                break;
            }
        }
        if (!found) {
            printf("Port buffer not found:%d\n", index);
        }
    }

    /*
    for (uint32_t index = 0; index < n_ports; ++index) {
        const LilvPort* port = lilv_plugin_get_port_by_index(plugin, index);
        if (lilv_port_is_a(plugin, port, lv2_ControlPort)) {
            lilv_instance_connect_port(instance, index, &controls[index]);
        } else if (lilv_port_is_a(plugin, port, lv2_AudioPort) ||
                   lilv_port_is_a(plugin, port, lv2_CVPort)) {
            if (lilv_port_is_a(plugin, port, lv2_InputPort)) {
                lilv_instance_connect_port(instance, index, in);
            } else if (lilv_port_is_a(plugin, port, lv2_OutputPort)) {
                lilv_instance_connect_port(instance, index, out);
            } else {
                fprintf(stderr, "<%s> port %d neither input nor output, skipping\n",
                        plugin_uri, index);
                lilv_instance_free(instance);
                free(controls);
                return 0;
            }
        } else if (lilv_port_is_a(plugin, port, atom_AtomPort)) {
            lilv_instance_connect_port(instance, index, &seq);
        } else {
            fprintf(stderr, "<%s> port %d has unknown type, skipping\n",
                    plugin_uri, index);
            lilv_instance_free(instance);
            free(controls);
            return 0;
        }
    }
    */

    lilv_instance_activate(instance);

    lilv_instance_run(instance, setup.block_size);

    lilv_instance_deactivate(instance);
    lilv_instance_free(instance);
    uri_table_destroy(&uri_table);
//    free(controls);

    lilv_node_free(urid_map);
    lilv_node_free(lv2_OutputPort);
    lilv_node_free(lv2_InputPort);
    lilv_node_free(lv2_ControlPort);
    lilv_node_free(lv2_CVPort);
    lilv_node_free(lv2_AudioPort);
    lilv_node_free(atom_Sequence);
    lilv_node_free(atom_AtomPort);

    lilv_world_free(world);

    return 1;
}
