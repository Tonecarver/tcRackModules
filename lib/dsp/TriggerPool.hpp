#ifndef _tc_triggerpool_h_
#define _tc_triggerpool_h_ 1

#include "plugin.hpp"

template <int COUNT>
struct TriggerPool {
    dsp::SchmittTrigger mTriggers[COUNT];

    void reset() {
        for (int i = 0; i < COUNT; i++) { 
            mTriggers[i].reset();
        }
    }

    bool process(int idx, float value) {
        return mTriggers[idx].process(value);
    }

};

#endif // _tc_triggerpool_h_
