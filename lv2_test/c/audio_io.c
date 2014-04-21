/*
  Copyright 2014 Joe Button

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
#include <gst/gst.h>
#include <gio/gio.h>
#include <math.h>
#include "audio_io.h"



int timer_callback (const void *data) {
	g_main_loop_quit ((GMainLoop *) data);
	return FALSE;
}


int play_audio(float *buf, uint32_t nsamples) {
    // Stupid function to play a buffer of floats using gstreamer. Converts to
    // 8 bit ints, which is cleary not the right way to do this but I couldn't
    // immediately work out how to make it work in any other mode.
	int i;
	uint8_t *wave;
	GMemoryInputStream *mistream;
	GstElement *source, *sink, *pipeline;
	GstPad *sourcepad;
	GMainLoop *loop;

	gst_init (NULL, NULL);

	wave = g_new (uint8_t, nsamples);
	for (i = 0; i < nsamples; i ++)
	{
        wave[i] = 128 + 128 * buf[i];
    }
	mistream = G_MEMORY_INPUT_STREAM(g_memory_input_stream_new_from_data(wave,
									     nsamples,
									     (GDestroyNotify) g_free));

	source = gst_element_factory_make ("giostreamsrc", "source");
	g_object_set (G_OBJECT (source), "stream", G_INPUT_STREAM (mistream), NULL);
	sourcepad = gst_element_get_static_pad(source, "src");
	gst_pad_set_caps (sourcepad,
			  gst_caps_new_simple ("audio/x-raw-int",
					       "rate", G_TYPE_INT, 48000,
					       "channels", G_TYPE_INT, 1,
					       "width", G_TYPE_INT, 8,
					       "depth", G_TYPE_INT, 8,
					       "signed", G_TYPE_BOOLEAN, FALSE,
					       NULL));
	gst_object_unref (sourcepad);

	sink = gst_element_factory_make ("alsasink", "sink");

	pipeline = gst_pipeline_new ("buffer-player");

	gst_bin_add_many (GST_BIN (pipeline),
			  source, sink, NULL);
	gst_element_link_many (source, sink, NULL);
	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (1000 * nsamples / SAMPLE_RATE, (GSourceFunc) timer_callback, loop);
	g_main_loop_run (loop);

	gst_element_set_state (pipeline, GST_STATE_NULL);

	gst_object_unref (GST_OBJECT (pipeline));
	g_main_loop_unref (loop);

	return 0;
}


int generate_beep(float *buf, uint32_t nsamples, uint32_t freq, float volume) {
    // Fill an audio buffer of floats with a beep of the specified frequency
    // and volume
    for (uint32_t i=0;i<nsamples;i++) {
        buf[i] = volume * sinf(freq * 2 * PI * (float)i / SAMPLE_RATE);
    }
    return 0;
}


int generate_bleepy_noise(float *buf, uint32_t nsamples) {
    // Fill an audio buffer of floats with short ascending / descending
    // bleeps
    int32_t direction = 1;
    float freq = 256;
    float volume = 0.2;
    int32_t samples_per_note = SAMPLE_RATE / 15;

    for (uint32_t i=1;i<nsamples;i++) {
        buf[i] = volume * sinf(freq * 2 * PI * (float)i / SAMPLE_RATE);
        if (i % samples_per_note == 0) {
            if (direction == 1) {
                freq *= 1.4;
            } else {
                freq /= 1.4;
            }
            if ((direction == 1 && freq > 1024) || (direction == -1 && freq < 256)) {
                direction = -direction;
            }
        }
    }
    return 0;
}


//int main(int argc, char** argv) {
//    uint32_t nsamples = 48000; // 1 seconds worth
//    float *buf = malloc(nsamples * sizeof(float));
//    generate_bleepy_noise(buf, nsamples);
//    play_audio(buf, nsamples);
//}
