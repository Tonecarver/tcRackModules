#ifndef _tc_scales_h_
#define _tc_scales_h_ 1

#include <string>
#include "plugin.hpp"

struct ScaleDefinition {
    enum { NUM_DEGREES_PER_SCALE = 12 };

    std::string   name;
    int           numDegrees;
    unsigned char degrees[NUM_DEGREES_PER_SCALE];

    int getNumDegrees() const {
        return numDegrees;
    }

    int getDegree(int idx) const {
        return degrees[idx];
    }

    void clear() {
        name = "";
        numDegrees = 0;
        for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++) {
            degrees[i] = 0; 
        }
    }

    bool containsDegree(int degree) {
        for (int i = 0; i < numDegrees; i++) {
            if (degrees[i] == degree) {
                return true;
            }
        }
        return false;
    }

    void addPitch(int midiPitch) { // 0..120 
        // Silently ignores duplicate degree values -- each degree may be specified only once
        int degree_relative_to_c = midiPitch % 12;

        // DEBUG("Scales: adding pitch %d, degree rel to C %d, numPitches %d (already contained? %s)", midiPitch, degree_relative_to_c, numDegrees, containsDegree(degree_relative_to_c) ? "YES" : "no");

        if ((numDegrees < NUM_DEGREES_PER_SCALE) && (! containsDegree(degree_relative_to_c))) {
            degrees[numDegrees] = degree_relative_to_c;
            numDegrees++;
        }
    }

};

struct ScaleTable {
    enum { NUM_SCALES = 79 };

    static const ScaleDefinition scaleTable[NUM_SCALES];

    static bool isValidIndex(int idx) {
        return idx >= 0 && idx < NUM_SCALES;
    }

    const ScaleDefinition * findScaleByName(std::string name);
};
#endif // _tc_scales_h_
