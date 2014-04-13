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

#include <stdlib.h>
#include <stdio.h>
#include <aubio/aubio.h>
#include <assert.h>
#include "../lv2_test.h"
#include "../audio_io.h"


void set_controls(float *buf, uint32_t num_blocks, float val) {
    // Set a buffer of control inputs to a value
    for (uint32_t i=0;i<num_blocks;i++) {
        buf[i] = val;
    }
}


int main(int argc, char** argv) {
    uint32_t block_size = 1024;
    uint32_t nsamples = 65536;
    uint32_t num_blocks = nsamples / block_size;

    float *gain_in = (float*)malloc(num_blocks * sizeof(float));
    float * audio_in = malloc(nsamples * sizeof(float));
    float * audio_out = malloc(nsamples * sizeof(float));
    generate_bleepy_noise(audio_in, nsamples);
//    play_audio(audio_in, nsamples);

    Lv2PortBufData gain_in_buf = {0, gain_in};
    Lv2PortBufData audio_in_buf = {1, (void*)audio_in};
    Lv2PortBufData audio_out_buf = {2, (void*)audio_out};
    Lv2PortBufData* bufs[] = {&gain_in_buf, &audio_in_buf, &audio_out_buf, NULL};

    Lv2TestSetup setup = {
        "http://drobilla.net/plugins/blop/amp",
        block_size,
        nsamples,
        SAMPLE_RATE,
        (Lv2PortBufData**)&bufs
    };

    set_controls(gain_in, num_blocks, 0);
    assert(run_plugin(setup));
    for (uint32_t i=0;i<nsamples;i++) {
        assert(audio_in[i] == audio_out[i]);
    }

    set_controls(gain_in, num_blocks, 1);
    assert(run_plugin(setup));
    for (uint32_t i=0;i<nsamples;i++) {
        assert(abs(audio_in[i]) >= abs(audio_out[i]));
    }

    set_controls(gain_in, num_blocks, -1);
    assert(run_plugin(setup));
    for (uint32_t i=0;i<nsamples;i++) {
        assert(abs(audio_in[i]) <= abs(audio_out[i]));
    }
//    play_audio(audio_out, nsamples);

    free(gain_in);
    free(audio_in);
    free(audio_out);

    printf("pass: %s\n", setup.plugin_uri);
    return 0;
}
