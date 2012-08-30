#include <stdlib.h>

#include "lv2.h"

static LV2_Descriptor *shimmerDescriptor = NULL;

typedef struct {
    float *input;
    float *output;
    float *shimmer_rate;
    float *shimmer_depth;
    float shimmer_phase;
    double sample_rate;
    int going_up;
} Plugin;

static void cleanupShimmer(LV2_Handle instance) {
    free(instance);
}

static void connectPortShimmer(LV2_Handle instance, uint32_t port, void *data) {
    Plugin *plugin = (Plugin *)instance;

    switch (port) {
    case 0:
        plugin->input = data;
        break;
    case 1:
        plugin->output = data;
        break;
    case 2:
        plugin->shimmer_rate = data;
        break;
    case 3:
        plugin->shimmer_depth = data;
    }
}

static LV2_Handle instantiateShimmer(const LV2_Descriptor *descriptor,
        double s_rate, const char *path, const LV2_Feature * const* features) {

    Plugin *plugin = (Plugin *)malloc(sizeof(Plugin));
    plugin->sample_rate = s_rate;
    plugin->shimmer_phase = 0;   // from 0-1 (1=360 degrees)
    plugin->going_up = 1;

    return (LV2_Handle)plugin;
}


static void runShimmer(LV2_Handle instance, uint32_t sample_count)
{
    Plugin *plugin = (Plugin *)instance;
    float shimmer_rate = *(plugin->shimmer_rate);
    float shimmer_depth = *(plugin->shimmer_depth);
    float *input = plugin->input;
    float *output = plugin->output;
    shimmer_rate = shimmer_rate < 0 ? 0 : shimmer_rate;
    shimmer_rate = shimmer_rate > 50 ? 50 : shimmer_rate;
    shimmer_depth = shimmer_depth < 0 ? 0 : shimmer_depth;
    shimmer_depth = shimmer_depth > 1 ? 1 : shimmer_depth;

    // Really dumb triangle wave
    for (uint32_t i = 0; i < sample_count; i++) {
        output[i] = input[i] - (input[i] * shimmer_depth * plugin->shimmer_phase);
        if (plugin->going_up) {
            plugin->shimmer_phase += shimmer_rate / plugin->sample_rate;
            if (plugin->shimmer_phase > 1) {
                plugin->shimmer_phase = 1;
                plugin->going_up = 0;
            }
        } else {
            plugin->shimmer_phase -= shimmer_rate / plugin->sample_rate;
            if (plugin->shimmer_phase < 0) {
                plugin->shimmer_phase = 0;
                plugin->going_up = 1;
            }
        }
    }
}

static void init()
{
    shimmerDescriptor =
     (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

    shimmerDescriptor->URI = "http://www.joebutton.co.uk/software/lv2/shimmer";
    shimmerDescriptor->activate = NULL;
    shimmerDescriptor->cleanup = cleanupShimmer;
    shimmerDescriptor->connect_port = connectPortShimmer;
    shimmerDescriptor->deactivate = NULL;
    shimmerDescriptor->instantiate = instantiateShimmer;
    shimmerDescriptor->run = runShimmer;
    shimmerDescriptor->extension_data = NULL;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (!shimmerDescriptor) init();

    switch (index) {
    case 0:
        return shimmerDescriptor;
    default:
        return NULL;
    }
}
