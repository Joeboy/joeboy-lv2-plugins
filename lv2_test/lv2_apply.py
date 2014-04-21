#!/usr/bin/env python
# -*- coding: utf-8 -*-

import lilv
import sys
import wave
import numpy


def read_wav(wf, nframes):
    """Read data from an open wav file. Return a list of channels, where each
    channel is a list of float samples."""
    wav = wf.readframes(nframes)
    sampwidth = wf.getsampwidth()
    print "sampwidth=", sampwidth
    if sampwidth == 4:
        wav = wave.struct.unpack("<%ul" % (len(wav) / 4), wav)
        wav = [ i / float(2**32) for i in wav ]
    elif sampwidth == 2:
        wav = wave.struct.unpack("<%uh" % (len(wav) / 2), wav)
        wav = [ i / float(2**16) for i in wav ]
    else:
        wav = wave.struct.unpack("%uB"  % (len(wav)),     wav)
        wav = [ s - 128 for s in wav ]
        wav = [ i / float(2**8) for i in wav ]

    n_channels = wf.getnchannels()
    channels = []
    for i in xrange(n_channels):
        channels.append([ wav[j] for j in xrange(0, len(wav), n_channels) ])

    return sampwidth, channels

def main():
    # Read command line arguments
    if len(sys.argv) != 4:
        print('USAGE: lv2_apply.py PLUGIN_URI INPUT_WAV OUTPUT_WAV')
        sys.exit(1)

    # Initialise Lilv
    world = lilv.World()
    world.load_all()

    plugin_uri = sys.argv[1]
    wav_in_path  = sys.argv[2]
    wav_out_path = sys.argv[3]

    # Find plugin
    plugin_uri_node   = world.new_uri(plugin_uri)
    plugin = world.get_all_plugins().get_by_uri(plugin_uri_node)
    if not plugin:
        print("Unknown plugin `%s'\n" % plugin_uri)
        sys.exit(1)

    lv2_InputPort  = world.new_uri(lilv.LILV_URI_INPUT_PORT)
    lv2_OutputPort = world.new_uri(lilv.LILV_URI_OUTPUT_PORT)
    lv2_AudioPort  = world.new_uri(lilv.LILV_URI_AUDIO_PORT)
    lv2_ControlPort  = world.new_uri(lilv.LILV_URI_CONTROL_PORT)
    lv2_DefaultProperty = world.new_uri("http://lv2plug.in/ns/lv2core#default")

    n_audio_in  = plugin.get_num_ports_of_class(lv2_InputPort,  lv2_AudioPort)
    n_audio_out = plugin.get_num_ports_of_class(lv2_OutputPort, lv2_AudioPort)
    if n_audio_out == 0:
        print("Plugin has no audio outputs\n")
        sys.exit(1)

    # Open input file
    wav_in = wave.open(wav_in_path, 'r')
    if not wav_in:
        print("Failed to open input `%s'\n" % wav_in_path)
        sys.exit(1)
    if wav_in.getnchannels() != n_audio_in:
        print("Input has %d channels, but plugin has %d audio inputs\n" % (
            wav_in.getnchannels(), n_audio_in))
        sys.exit(1)

    # Open output file
    wav_out = wave.open(wav_out_path, 'w')
    if not wav_out:
        print("Failed to open output `%s'\n" % wav_out_path)
        sys.exit(1)

    # Set output file to same format as input (except possibly nchannels)
    wav_out.setparams(wav_in.getparams())
    wav_out.setnchannels(n_audio_out)

    rate    = wav_in.getframerate()
    nframes = wav_in.getnframes()

    print('%s => %s => %s @ %d Hz'
          % (wav_in_path, plugin.get_name(), wav_out_path, rate))

    instance = lilv.Instance(plugin, rate)

    sampwidth, channels = read_wav(wav_in, nframes)
    buf_size = len(channels[0])

    # Connect buffers. NB if we fail to connect any buffer, lilv will segfault.
    audio_input_buffers = []
    audio_output_buffers = []
    control_input_buffers = []
    control_output_buffers = []
    for index in range(plugin.get_num_ports()):
        port = plugin.get_port_by_index(index)
        if port.is_a(lv2_InputPort):
            if port.is_a(lv2_AudioPort):
                audio_input_buffers.append(numpy.array(channels[len(audio_input_buffers)], numpy.float32))
                instance.connect_port(index, audio_input_buffers[-1])
            elif port.is_a(lv2_ControlPort):
                #if port.has_property(lv2_DefaultProperty):  # Doesn't seem to work
                default = lilv.lilv_node_as_float(lilv.lilv_nodes_get_first(port.get_value(lv2_DefaultProperty)))
                print "def: ",default
                control_input_buffers.append(numpy.array([default], numpy.float32))
                instance.connect_port(index, control_input_buffers[-1])
            else:
                raise Exception, "Unhandled port type"
        elif port.is_a(lv2_OutputPort):
            if port.is_a(lv2_AudioPort):
                audio_output_buffers.append(numpy.array([0] * buf_size, numpy.float32))
                instance.connect_port(index, audio_output_buffers[-1])
            elif port.is_a(lv2_ControlPort):
                control_output_buffers.append(numpy.array([0], numpy.float32))
                instance.connect_port(index, control_output_buffers[-1])
            else:
                raise Exception, "Unhandled port type"

    print len(audio_input_buffers)
    print len(audio_output_buffers)
    # Run the plugin:
    instance.run(buf_size)

    # Write the output file:
    print len(audio_output_buffers), len(audio_input_buffers)
    print sampwidth
    for buf in audio_output_buffers:
#        buf *= 2 ** (8 * sampwidth)
        buf *= 32768
        out = buf.astype(numpy.short)
        wav_out.writeframes(out)


if __name__ == "__main__":
    main()
