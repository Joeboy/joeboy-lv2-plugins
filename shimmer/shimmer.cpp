#include <lv2plugin.hpp>


using namespace LV2;


class Shimmer : public Plugin<Shimmer> {
public:
  
  Shimmer(double rate)
    : Plugin<Shimmer>(4) {
    sample_rate = rate;
    shimmer_phase = 0;   // from 0-1 (1=360 degrees)
    going_up = true;
  }
  
  void run(uint32_t nframes) {
    shimmer_rate = *p(2);
    shimmer_depth = *p(3);
    shimmer_rate = shimmer_rate < 0 ? 0 : shimmer_rate;
    shimmer_rate = shimmer_rate > 50 ? 50 : shimmer_rate;
    shimmer_depth = shimmer_depth < 0 ? 0 : shimmer_depth;
    shimmer_depth = shimmer_depth > 1 ? 1 : shimmer_depth;

    // Really dumb triangle wave
    for (uint32_t i = 0; i < nframes; ++i) {
        p(1)[i] = p(0)[i] - (p(0)[i] * shimmer_depth * shimmer_phase);
        if (going_up) {
            shimmer_phase += shimmer_rate / sample_rate;
            if (shimmer_phase > 1) {
                shimmer_phase = 1;
                going_up = false;
            }
        } else {
            shimmer_phase -= shimmer_rate / sample_rate;
            if (shimmer_phase < 0) {
                shimmer_phase = 0;
                going_up = true;
            }
        }
    }
  }

protected:
    float sample_rate;
    float shimmer_phase;
    float shimmer_rate;
    float shimmer_depth;
    bool going_up; // or down if false
};


static int _ = Shimmer::register_class("http://www.joebutton.co.uk/software/lv2/shimmer");

