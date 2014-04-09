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
#include "lv2_test.h"


int main(int argc, char** argv) {
    // Check that the blop square wave generator, when a frequency is requested
    // by the input, generates an output which aubio identifies as a tone of
    // approximately that frequency
    float freq = 500;
    uint32_t block_size = 1024;
    float* out = (float*)malloc(block_size * sizeof(float));
    Lv2PortBufData in_buf = {0, &freq};
    Lv2PortBufData out_buf = {1, (void*)out};
    Lv2PortBufData* bufs[] = {&in_buf, &out_buf, NULL};

    Lv2TestSetup setup = {
        "http://drobilla.net/plugins/blop/square",
        block_size,
        48000,
        (Lv2PortBufData**)&bufs
    };

    // Run the plugin
    assert(run_plugin(setup));

    // Use aubio to check that the output looks like it's within a few hz of
    // the frequency we requested:
    fvec_t *aubio_in = new_fvec(setup.block_size);
    fvec_t *aubio_out = new_fvec (1);
    aubio_pitch_t *aubio_pitch = new_aubio_pitch ("default", setup.block_size, 256, setup.sample_rate);
    for (int i=0;i<setup.block_size;i++) {
        aubio_in->data[i] = *(out + i);
    }
    aubio_pitch_do(aubio_pitch, aubio_in, aubio_out);
//    printf("%f\n", aubio_out->data[0]);
    assert (freq*0.99 < aubio_out->data[0]);
    assert (aubio_out->data[0] < freq*1.01);
    del_fvec(aubio_in);
    del_fvec(aubio_out);
    free(out);

    printf("pass: %s\n", setup.plugin_uri);
    return 0;
}
