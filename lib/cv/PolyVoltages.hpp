#ifndef _tc_polyvoltages_h_
#define _tc_polyvoltages_h_  1

#include "plugin.hpp"

class PolyVoltages { 
    int mNumChannels; 
    float mVoltages[PORT_MAX_CHANNELS];

public:
    PolyVoltages() : mNumChannels(0) {
        memset(mVoltages, 0, sizeof(mVoltages));
    }

    int getNumChannels() const { return mNumChannels; }
    float const * getVoltages() const { return &mVoltages[0]; }
    float getVoltage(int channelNumber) const { return mVoltages[channelNumber]; }

    /**
     * Compare the number of channels and poly voltage values to the stored values.
     * If different, store the current values and return true, otherwise return false.
    */
    bool process(int numChannels, float * voltages) {
        if ((numChannels != mNumChannels) || (memcmp(voltages, mVoltages, (sizeof(mVoltages[0]) * mNumChannels)) != 0)) {
            mNumChannels = numChannels;
            memset(mVoltages, 0, sizeof(mVoltages));
            memcpy(mVoltages, voltages, (sizeof(mVoltages[0]) * numChannels));
            return true; // values changed 
        }
        return false; // values did not change 
    }

    void reset() { 
        mNumChannels = 0; 
    }

    // void clear() {
    //     mNumChannels = 0;
    //     memset(mVoltages, 0, sizeof(mVoltages));
    // }

}; 

#endif // _tc_polyvoltages_h_