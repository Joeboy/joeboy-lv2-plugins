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

#ifndef LV2_TEST_H
#define LV2_TEST_H

#include <stdint.h>

typedef struct {
    uint32_t index;
    void* data;
} Lv2PortBufData;

typedef struct {
	char* plugin_uri;
    uint32_t block_size;
    uint32_t sample_rate;
    Lv2PortBufData** lv2_port_buffers;
} Lv2TestSetup;

int run_plugin(Lv2TestSetup setup);

#endif  /* LV2_TEST_H */
