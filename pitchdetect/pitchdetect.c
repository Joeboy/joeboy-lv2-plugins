#include <lv2.h>
#include <aubio/aubio.h>
#include <malloc.h>


#define PITCHDETECTOR_URI       "http://www.joebutton.co.uk/software/lv2/pitchdetect"
#define INPUT       0
#define OUTPUT      1

#define BUFFER_SIZE 2048

static LV2_Descriptor *pitchdetectorDescriptor = NULL;

typedef struct {
    float *input;
    float *output;

    fvec_t *aubio_in; // aubio input structure
    fvec_t *aubio_out; // aubio output
    aubio_pitch_t *aubio_pitch; // aubio pitch structure
} PitchDetector;

static void cleanupPitchDetector(LV2_Handle instance) {
    PitchDetector *plugin_data = (PitchDetector*)instance;
    del_fvec(plugin_data->aubio_in);
    del_fvec(plugin_data->aubio_out);
    del_aubio_pitch(plugin_data->aubio_pitch);
    free(instance);
}

static void connectPortPitchDetector(LV2_Handle instance,
                                     uint32_t port, void *data) {

    PitchDetector* plugin = (PitchDetector *)instance;

    switch (port) {
    case INPUT:
        plugin->input = data;
        break;
    case OUTPUT:
        plugin->output = data;
        break;
    }
}

static LV2_Handle instantiatePitchDetector( const LV2_Descriptor *desc, double sample_rate,
                                       const char *bundle_path, const LV2_Feature * const * features) {

    PitchDetector *plugin_data = (PitchDetector*)malloc(sizeof(PitchDetector));
    plugin_data->aubio_out = new_fvec (1);
    plugin_data->aubio_in = new_fvec(BUFFER_SIZE);
    plugin_data->aubio_pitch = new_aubio_pitch ( (char*)"default",
                                                 plugin_data->aubio_in->length,
                                                 plugin_data->aubio_in->length / 2,
                                                 sample_rate );
    return (LV2_Handle)plugin_data;
}

static void runPitchDetector(LV2_Handle instance, uint32_t sample_count)
{
    PitchDetector *plugin_data = (PitchDetector*)instance;

    static float prev_pitch = 0;
    static uint32_t bufptr = 0;
    uint32_t pos;

    for (pos=0;pos<sample_count;pos++) {
        plugin_data->aubio_in->data[bufptr] = plugin_data->input[pos];
        plugin_data->output[pos] = prev_pitch;
        if (++bufptr > plugin_data->aubio_in->length) {
            bufptr = 0;
            aubio_pitch_do(plugin_data->aubio_pitch, plugin_data->aubio_in, plugin_data->aubio_out);
            prev_pitch = plugin_data->aubio_out->data[0];
        }
    }
}

static void init()
{
    pitchdetectorDescriptor =
     (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));
    pitchdetectorDescriptor->URI = PITCHDETECTOR_URI;
    pitchdetectorDescriptor->activate = NULL;
    pitchdetectorDescriptor->cleanup = cleanupPitchDetector;
    pitchdetectorDescriptor->connect_port = connectPortPitchDetector;
    pitchdetectorDescriptor->deactivate = NULL;
    pitchdetectorDescriptor->instantiate = instantiatePitchDetector;
    pitchdetectorDescriptor->run = runPitchDetector;
    pitchdetectorDescriptor->extension_data = NULL;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (!pitchdetectorDescriptor) init();

    switch (index) {
    case 0:
        return pitchdetectorDescriptor;
    default:
        return NULL;
    }
}
