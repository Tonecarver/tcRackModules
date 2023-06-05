#ifndef _tc_polyscale_h_
#define _tc_polyscale_h_ 1

#include "plugin.hpp" // DEBUG 
#include "TwelveToneScale.hpp"
#include "../cv/Voltage.hpp"
#include <cstring>  // memset()

struct PolyScale { 

    enum PolyScaleFormat {
        POLY_EXT_SCALE,
        SORTED_VOCT_SCALE,
        // 
        kNum_POLY_SCALE_FORMAT
    };

    PolyScale(PolyScaleFormat scaleFormat = PolyScaleFormat::SORTED_VOCT_SCALE) 
        : mPolyScaleFormat(scaleFormat)
    {
        clear();
        mTonic = 0;
        mScaleDegrees[0] = true;
        mNumDegrees = 1;
    }

    void setScaleFormat(PolyScaleFormat scaleFormat) {
        mPolyScaleFormat = scaleFormat;
    }

    PolyScaleFormat getScaleFormat() const {
        return mPolyScaleFormat;
    }
    
    // TODO: add get scale type name() 
    // TODO: delete mTonic, or fix up how it is used, setting it to non-zero causes problems

    int getTonic()      const { return mTonic; }
    int getNumDegrees() const { return mNumDegrees; }    
    bool containsDegree(int degree) const { return mScaleDegrees[degree]; }

    void computeScale(int numChannels, float * scaleVoltages) {
        if (mPolyScaleFormat == PolyScaleFormat::POLY_EXT_SCALE) {
            computePolyExtScale(numChannels, scaleVoltages);
        } else if (mPolyScaleFormat == PolyScaleFormat::SORTED_VOCT_SCALE) {
            computeSortedVoctScale(numChannels, scaleVoltages);
        } else {
            DEBUG("Bad scale type: %d", mPolyScaleFormat);
        }
        // DEBUG("Poly Scale:  tonic %d, num degrees %d", mTonic, mNumDegrees);
        // for (int i = 0; i < 12; i++) {
        //     DEBUG("  degree[%2d]  = %s ", i, mScaleDegrees[i] == true ? "ENABLED" : "-");
        // }
    }

    void computePolyExtScale(int numChannels, float * scaleVoltages) {
        if (numChannels > 0) {
            clear();
            numChannels = clamp(numChannels, 1, TwelveToneScale::NUM_DEGREES_PER_SCALE);
            for (int i = 0; i < numChannels; i++) {
                if (scaleVoltages[i] > 0.1f) { 
                    mScaleDegrees[i] = true;
                    // if (scaleVoltages[i] > 8.5f) { 
                    //     mTonic = i;
                    // }
                    mNumDegrees++;
                }
            }
        } else {
            DEBUG("PolyExtScale: bad channel count %d, .. skipping", numChannels);
        }
    }

    /** 
     * Channels carry V/Oct voltage identifying the degrees in the scale
     * Tonic is channel[0]
    */
    void computeSortedVoctScale(int numChannels, float * scaleVoltages) {
        if (numChannels > 0 && numChannels <= PORT_MAX_CHANNELS) {
            clear(); 
            //mTonic = voctToDegreeRelativeToC(scaleVoltages[0]); 
            for (int i = 0; i < numChannels; i++) {
                int degree = voctToDegreeRelativeToC(scaleVoltages[i]);
                if (mScaleDegrees[degree] == false) {
                    mScaleDegrees[degree] = true;
                    mNumDegrees++;
                }
            }
        } else {
            DEBUG("SortedExtScale: bad channel count %d, .. skipping", numChannels);
        }
    }

private:
    int  mTonic;  // Defaults to 0 (C4) if no tonic identified
    bool mScaleDegrees[TwelveToneScale::NUM_DEGREES_PER_SCALE];
    int  mNumDegrees;
    PolyScaleFormat mPolyScaleFormat;

    void clear() {
        mTonic = 0;
        memset(mScaleDegrees, 0, sizeof(mScaleDegrees));
        mNumDegrees = 0;
    }
}; 

#endif //  _tc_polyscale_h_

