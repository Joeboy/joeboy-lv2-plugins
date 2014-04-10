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
#include <string.h>
#include <assert.h>
#include "../lv2_test.h"


int main(int argc, char** argv) {
    uint32_t block_size = 512;
    float* drive_in = (float*)malloc(block_size * sizeof(float));
    Lv2PortBufData drive_in_buf = {0, (void*)drive_in};
    float* thresh_in = (float*)malloc(sizeof(float));
    Lv2PortBufData thresh_in_buf = {1, (void*)thresh_in};
    float* attack_in = (float*)malloc(sizeof(float));
    Lv2PortBufData attack_in_buf = {2, (void*)attack_in};
    float* decay_in = (float*)malloc(sizeof(float));
    Lv2PortBufData decay_in_buf = {3, (void*)decay_in};
    float* sustain_in = (float*)malloc(sizeof(float));
    Lv2PortBufData sustain_in_buf = {4, (void*)sustain_in};
    float* release_in = (float*)malloc(sizeof(float));
    Lv2PortBufData release_in_buf = {5, (void*)release_in};
    float* env_out = (float*)malloc(block_size * sizeof(float));
    Lv2PortBufData env_out_buf = {6, (void*)env_out};

    memset(drive_in, 0, block_size * sizeof(float));
    *thresh_in=0.1;
    *attack_in=0.1;
    *decay_in=0.1;
    *sustain_in=0.1;
    *release_in=0.1;

    Lv2PortBufData* bufs[] = {
        &drive_in_buf,
        &thresh_in_buf,
        &attack_in_buf,
        &decay_in_buf,
        &sustain_in_buf,
        &release_in_buf,
        &env_out_buf,
        NULL
    };

    Lv2TestSetup setup = {
        "http://drobilla.net/plugins/blop/adsr",
        block_size,
        48000,
        (Lv2PortBufData**)&bufs
    };

    // Run the plugin
    assert(run_plugin(setup));

    //for (int i=0;i<setup.block_size;i++) {
    //    printf("%f\n", *(out + i));
   // }
    free(drive_in);
    free(thresh_in);
    free(attack_in);
    free(sustain_in);
    free(decay_in);
    free(release_in);
    free(env_out);

    printf("pass: %s\n", setup.plugin_uri);
    return 0;
}
