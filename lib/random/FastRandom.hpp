#ifndef _tc_fast_random_hpp_
#define _tc_fast_random_hpp_

#ifndef M_PI
#  ifndef _USE_MATH_DEFINES
#    define _USE_MATH_DEFINES  // M_PI 
#  endif 
#include <math.h>          // M_PI 
#endif 

#include <cstdint>

struct FastRandom { 
    uint32_t x = 123456789;
    uint32_t y = 362436069;
    uint32_t z = 521288629;
    uint32_t w = 88675123;
    
    float generateZeroToOne() { 
        return (float) xor128() / (float)UINT32_MAX;
    }

    float generatePlusMinusOne() { 
        return (((float) xor128() / (float)UINT32_MAX) - 0.5f) * 2.f;
    }
    
    float generatePlusMinusPi() { 
        return (((float) xor128() / (float)UINT32_MAX) - 0.5f) * 2.f * M_PI;
    }
    
    //float generate() { 
    //    return (float) xor128() / (float)UINT32_MAX;
    //}

    uint32_t xor128(void) {
        uint32_t t;
        t = x ^ (x << 11);   
        x = y; y = z; z = w;   
        return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
    }

    void setSeed(int seed) {
        x = seed >> 1;
        y = seed >> 3;
        z = (seed >> 16) | 10203040;
        w = (seed ^ 0xa5a5a5a5);
    }
};

#endif // _tc_fast_random_hpp_
