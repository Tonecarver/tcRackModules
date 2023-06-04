#ifndef _voltage_hpp__
#define _voltage_hpp__

#include <cmath> // round()

// voct range is -5.f .. 5.f
// pitch range is 0 .. 120 paralleling the MIDI standard

inline int voctToPitch(float voct) {  
	int pitch = round((voct + 5.f) * 12.f); // 0 .. 120 
	return pitch;
}

inline int voctToOctave(float voct) {  
	int octave = round(((voct + 5.f) * 12.f) / 10.f); // 10 octaves in 0 .. 120 
	return octave;
}

inline int voctToDegreeRelativeToC(float voct) {  // degree, relative to C
	int pitch = round((voct + 5.f) * 12.f);
	int degree = pitch % 12;
	return degree;
}

// pitch is 0..120
// voct is -5..0..5 
float pitchToVoct(int pitch) {
	return (float(pitch) / 12.f) - 5.f;
}


// TODO: notyet 

// /** Rescales `x` from the range `[-5, 5]` to `[yMin, yMax]`.
// */
// inline float rescalePlusMinusFive(float x, float yMin, float yMax) {
// 	return yMin + ((x + 5.f) * 0.1f) * (yMax - yMin);
// }

// /** Rescales `x` from the range `[0, 10]` to `[yMin, yMax]`.
// */
// inline float rescaleZeroTen(float x, float yMin, float yMax) {
// 	return yMin + (x * 0.1f) * (yMax - yMin);
// }

#endif // _voltage_hpp__
