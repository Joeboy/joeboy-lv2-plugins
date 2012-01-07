#include <lv2plugin.hpp>
#include "visual_compressor.peg"
#include "util.h"
#include <math.h>

#define RMS_BUF_SIZE 512

using namespace LV2;

class VisualCompressor : public Plugin<VisualCompressor> {
public:
  
    VisualCompressor(double rate)
      : Plugin<VisualCompressor>(p_n_ports) {
      sample_rate = rate;
    }

    void activate() {
        rms_samplecount = 0;
        rms_total = 0;
        gain = 1;
        time_step_s = RMS_BUF_SIZE / sample_rate;
    }
  
    void run(uint32_t nframes) {
        threshold = *p(p_threshold);
        ratio = *p(p_ratio);
        input_gain = *p(p_input_gain);
        makeup_gain = *p(p_makeup_gain);
        // Check the values are within acceptable limits
        threshold = threshold < p_ports[p_threshold].min ? p_ports[p_threshold].min : threshold;
        threshold = threshold > p_ports[p_threshold].max ? p_ports[p_threshold].max : threshold;
        ratio = ratio < p_ports[p_ratio].min ? p_ports[p_ratio].min : ratio;
        ratio = ratio > p_ports[p_ratio].max ? p_ports[p_ratio].max : ratio;
        input_gain = input_gain < p_ports[p_input_gain].min ? p_ports[p_input_gain].min : input_gain;
        input_gain = input_gain < p_ports[p_input_gain].min ? p_ports[p_input_gain].min : input_gain;
        makeup_gain = makeup_gain > p_ports[p_makeup_gain].max ? p_ports[p_makeup_gain].max : makeup_gain;
        makeup_gain = makeup_gain > p_ports[p_makeup_gain].max ? p_ports[p_makeup_gain].max : makeup_gain;

        input_gain_coeff = dB_to_coefficient(input_gain);
        makeup_gain_coeff = dB_to_coefficient(makeup_gain);
    
        for (uint32_t i = 0; i < nframes; ++i) {
            // Keep the rms output updated
            rms_total += p(p_input)[i] * p(p_input)[i];
            rms_samplecount++;
            if (rms_samplecount == RMS_BUF_SIZE) {
                rms = input_gain_coeff * sqrt(rms_total/RMS_BUF_SIZE);
                *p(p_rms)=rms;
                rms_samplecount=0;
                rms_total=0;

                // Calculate the desired gain
                threshold_coeff = dB_to_coefficient(threshold);
                if (rms <= threshold_coeff) {
                    // How many times do we get to modify the gain before we hit the target_gain / release_time?
                    gain_chunks = *p(p_release_time)/time_step_s;
                    target_gain = 1;
                } else {
                    gain_chunks = *p(p_attack_time)/time_step_s;
                    target_gain = (threshold_coeff + (rms - threshold_coeff) / ratio) / rms;
                }

                // FIXME: Doing it this way means the time setting isn't quite right:
                gain += (target_gain - gain) / gain_chunks;
                *p(p_gain) = gain;
            }
    
            p(p_output)[i] = makeup_gain_coeff * input_gain_coeff * p(p_input)[i] * gain;
        }
    }

protected:
    float time_step_s; // length of the rms buffer in seconds
    float sample_rate;
    float threshold;
    float ratio;
    float input_gain;
    float makeup_gain;
    float input_gain_coeff;
    float makeup_gain_coeff;
    float threshold_coeff;
    float rms;
    float target_gain;
    float gain;
    float gain_chunks;
    uint32_t rms_samplecount;
    float rms_total;
};


static int _ = VisualCompressor::register_class("http://www.joebutton.co.uk/software/lv2/visual_compressor");

