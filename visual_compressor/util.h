#include <math.h>

static inline float dB_to_coefficient (float dB) {
    return dB > -318.8f ? pow (10.0f, dB * 0.05f) : 0.0f;
}

static inline float coefficient_to_dB (float coeff) {
    return 20.0f * log10 (coeff);
}
