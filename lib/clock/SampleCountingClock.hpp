#ifndef _tc_sample_counting_clock_h_ 
#define _tc_sample_counting_clock_h_  1

#include "../lib/FloatCounter.hpp"


// Usage: 
// 
// 		roverStepClock.setStepSize( getSamplesPerBeat() );
//		roverStepClock.sync();   // begin cycle now
//		roverStepClock.setStepSize( activeSamplesPerBeat ); 
//		if (isConnectedToExternalClock()) {
//			roverStepClock.sync();
//
// process()
//      roverStepClock.tick();
// 		if (roverStepClock.isLeadingEdge()) {
//          .....
// 		if (roverStepClock.isTrailingEdge()) {
//          .....






struct SampleCountingClock {
	FloatCounter sampleCounter;
	float stepSize;    // number of ticks per cycle
	float pulseWidth;  // 0.5 = even high,low, 0.66 = swing
	float highSideSamples; // number of samples in the High portion of the cycle 
	bool isHigh;       // true while clock is in high portion
	bool leadingEdge;  // true when clock goes high (begin new step)
	bool trailingEdge; // true when clock goes low ("half-way" point based on pulseWidth)

	SampleCountingClock() {
		stepSize = 44100.f;
		pulseWidth = 0.5f;
		highSideSamples = stepSize * pulseWidth;
		isHigh = false;
		leadingEdge = true;
		trailingEdge = false;
		computePosition();
	}

	void setStepSize(float stepSize) {
		if (this->stepSize != stepSize) {
			this->stepSize = stepSize;
			computePosition();
		}
	}

	// sets % of time for the high side of the cycle
	// values of 0 and 1 are nonsensical
	//   0.5 is even igh/low sides 
	//   0.66666 is swing division (2/3)
	void setPulseWidth(float pulseWidth) {
		if (this->pulseWidth != pulseWidth) {
			this->pulseWidth = pulseWidth;
			computePosition();
		}
	}

	bool isLeadingEdge() const {
		return leadingEdge;
	}

	bool isTrailingEdge() const {
		return trailingEdge;
	}

	bool isEdge() const {
		return leadingEdge || trailingEdge;
	}

	float getHighSideLength() const {
		return highSideSamples;
	}

	float getLowSideLength() const {
		return stepSize - highSideSamples;
	}

	void sync() {  // begin new cycle
		sampleCounter.reset();
		isHigh = true;
		leadingEdge = true;
		trailingEdge = false;
	}

	bool tick() { 
		leadingEdge = sampleCounter.tick();
		trailingEdge = isHigh && (sampleCounter.getCount() >= highSideSamples);
		if (leadingEdge) {
			isHigh = true;
		}
		else if (trailingEdge) {
			isHigh = false;
		}
		return leadingEdge;
	}

	void computePosition() {
		sampleCounter.setTarget(stepSize);
		float sampleCount = sampleCounter.getCount();
		highSideSamples = stepSize * pulseWidth;
		leadingEdge = (sampleCount >= stepSize);
		trailingEdge = isHigh && (sampleCount >= highSideSamples);
		if (leadingEdge) {
			sampleCount = 0;
			isHigh = true;
		}
		else if (trailingEdge) {
			isHigh = false;
		}
	}

	void reset() { 
		sync();
	}
};

#endif // _tc_sample_counting_clock_h_ 
