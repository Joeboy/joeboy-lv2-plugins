#!/usr/bin/env python
# -*- coding: utf-8 -*-

# It would be nice to be able to run plugins from python for testing purposes.
# Unfortunately this segfaults when calling run() though.

import lilv
import sys
import numpy

world = lilv.World()
world.load_all()

plugin_uri   = world.new_uri("http://lv2plug.in/plugins/eg-amp")

plugin = world.get_all_plugins().get_by_uri(plugin_uri)
assert plugin

lv2_InputPort  = world.new_uri(lilv.LILV_URI_INPUT_PORT)
lv2_OutputPort = world.new_uri(lilv.LILV_URI_OUTPUT_PORT)
lv2_AudioPort  = world.new_uri(lilv.LILV_URI_AUDIO_PORT)

# Find first audio in/out ports:
input_port_index = None
output_port_index = None
for i in range(plugin.get_num_ports()):
    port = plugin.get_port_by_index(i)
    if input_port_index is None and port.is_a(lv2_InputPort) and port.is_a(lv2_AudioPort):
        input_port_index = i
    if output_port_index is None and port.is_a(lv2_OutputPort) and port.is_a(lv2_AudioPort):
        output_port_index = i
assert input_port_index and output_port_index

instance = lilv.Instance(plugin, 48000)
in_buf = numpy.array([23] * 256, numpy.float32)
out_buf = numpy.array([0] * 256, numpy.float32)
instance.connect_port(input_port_index, in_buf)
instance.connect_port(output_port_index, out_buf)
instance.run(len(in_buf))   # <-- Segfaults
