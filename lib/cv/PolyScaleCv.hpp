#ifndef _tc_polyscalecv_h_
#define _tc_polyscalecv_h_ 1


// TODO: not yet 
// #include "PolyVoltages.hpp"
// #include "../scale/PolyScale.hpp"

// struct PolyScaleCv {
// 	PolyVoltages    mPolyScaleVoltages;
// 	PolyScale       mPolyScale;

//     bool process(int numChannels, float * voltages) {
//         if (mPolyScaleVoltages.process(numChannels, voltages)) {
//             mPolyScale.computeScale(numChannels, voltages);
//             return true; // change detected
//         }
//         return false; // no change
//     }

//     bool processScaleButtons(float tonic, bool * pScaleButons) {
//         mPolyScaleVoltages.reset(); 

//     }

//     // set scale format
//     // get scale format
//     bool containsDegree(int degree) const { 
//         return mPolyScale.containsDegree(degree);
//     }


// }; 

#endif // _tc_polyscalecv_h_
