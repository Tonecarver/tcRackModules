#ifndef _tc_clock_watcher_h_
#define _tc_clock_watcher_h_ 1

#include "plugin.hpp"
#include "SampleRateCalculator.hpp"

// Usage: 
// 		clock.setSampleRate(APP->engine->getSampleRate());
//		clock.setBeatsPerMinute(DFLT_BPM);
//		activeBpm = DFLT_BPM;
//
// onReset()
// 		clock.reset();
//
// process() 
//      if (inputs[CLOCK_CV_INPUT].isConnected()) {
//          clock.tick_external(inputs[CLOCK_CV_INPUT].getVoltage());
//      } else {
//          clock.click_internal();	

struct ClockWatcher {	
	enum ClockState {
		UNKNOWN,
		LOW,
		HIGH,
		//
		kNum_ClockStates
	};

	enum ClockTransition { 
		STEADY,
		LEADING_EDGE,
		TRAILING_EDGE
	};

	ClockState mClockState; 
	ClockTransition mClockTransition;
	SampleRateCalculator  mSampleRateCalculator;
	bool  mExternalClockConnected; 
	float mSamplesSinceExternalLeadingEdge;
	float mSamplesSinceClockPulse;

	ClockWatcher() {
		mClockState = ClockState::UNKNOWN;
		mClockTransition = ClockTransition::STEADY;
		mExternalClockConnected = false;
		mSamplesSinceExternalLeadingEdge = 0.f;
		mSamplesSinceClockPulse = 0.f;
	}

	void setBeatsPerMinute(float beatsPerMinute) {
		mSampleRateCalculator.setBpm(beatsPerMinute);
	}

	void setSampleRate(float sampleRate) {
		mSampleRateCalculator.setSampleRate(sampleRate);
	}

	float getBeatsPerMinute() const {
		return mSampleRateCalculator.bpm;
	}

	float getSamplesPerBeat() const {
		return mSampleRateCalculator.getSamplesPerBeat();
	}

	float getSampleRate() const { 
		return mSampleRateCalculator.sampleRate; // TODO: get this from rack() 
	}

	bool isConnectedToExternalClock() const {
		return mExternalClockConnected;
	}

	void reset() { 
/////		clockTrigger.reset();  // TODO: remove this method?
	}

	bool isLeadingEdge() const {
		return mClockTransition == ClockTransition::LEADING_EDGE; 
	}

	bool isTrailingEdge() const {
		return mClockTransition == ClockTransition::TRAILING_EDGE; 
	}

	bool isHigh() const {
		return mClockState == ClockState::HIGH;
	}

	bool isLow() const {
		return mClockState == ClockState::LOW;
	}

	// Call this if external clock is connected 
	void tick_external(float clock_cv) {

		// Update BPM based on leading edge of external clock pulse 
		mSamplesSinceClockPulse ++;
		mSamplesSinceExternalLeadingEdge ++;

		ClockState state = ClockState::UNKNOWN;
		if (clock_cv > 9.5) {
			state = ClockState::HIGH;
		} else if (clock_cv < 5.0) {
			state = ClockState::LOW;
		}

		mClockTransition = transitionTable[mClockState][state];

		if (mClockTransition == ClockTransition::LEADING_EDGE) {
			// DEBUG("ClockWatcher: External Clock EDGE DETECTED: clock_cv %f, samples since external leading edge %f", clock_cv, mSamplesSinceExternalLeadingEdge);
			// Transition low to high 
			if (mExternalClockConnected) { // have detected at least one full cycle 
				// DEBUG("ClockWatcher: External Clock SET BPM clock_cv %f, samples since %f", clock_cv, mSamplesSinceExternalLeadingEdge);
				mSampleRateCalculator.setBpmFromNumSamples(mSamplesSinceExternalLeadingEdge);
				mSamplesSinceClockPulse = mSampleRateCalculator.getSamplesPerBeat();
				// DEBUG("ClockWatcher: Computed BPM as %f, samplesPerBeat %f", sampleRateCalculator.bpm, sampleRateCalculator.getSamplesPerBeat());
				// DEBUG("   samples since external leading edge = %f", mSamplesSinceExternalLeadingEdge);
			}
			mSamplesSinceExternalLeadingEdge = 0.f;
		}

		mClockState = state; 
		mExternalClockConnected = true;
	
		float overshoot = mSamplesSinceClockPulse - mSampleRateCalculator.getSamplesPerBeat();
		if (overshoot >= 0.f) {
			// DEBUG("Tick external: Overshoot %f", overshoot);
			mSamplesSinceClockPulse = overshoot; // capture the overshoot 
		}
	}

	// Call this if external clock is not connected 
	void tick_internal() {
		mExternalClockConnected = false;
		mSamplesSinceClockPulse += 1.f;
		mClockTransition = ClockTransition::STEADY;
		float overshoot = mSamplesSinceClockPulse - (mSampleRateCalculator.getSamplesPerBeat() * 0.5);
		if (overshoot >= 0.f) {
			ClockState state = (mClockState == ClockState::LOW ? ClockState::HIGH : ClockState::LOW);
			mClockTransition = transitionTable[ mClockState ][ state ];
			mClockState = state;
			// DEBUG("Tick internal: Overshoot %f", overshoot);
			// DEBUG("Tick internal: samples since clock pulse %f, clock state %d, transition %d", samplesSinceClockPulse, mClockState, mClockTransition);
			mSamplesSinceClockPulse = overshoot; // capture the overshoot 
		}
	}

private:
	const ClockTransition transitionTable[ClockState::kNum_ClockStates][ClockState::kNum_ClockStates] = {
		/* -------          UNKNOWN,                     LOW,                         HIGH */
		/* UNKNOWN */ { ClockTransition::STEADY, ClockTransition::STEADY, ClockTransition::STEADY },
		/* LOW     */ { ClockTransition::STEADY, ClockTransition::STEADY, ClockTransition::LEADING_EDGE },
		/* HIGH    */ { ClockTransition::STEADY, ClockTransition::TRAILING_EDGE, ClockTransition::STEADY },
	};

};

#endif //  _tc_clock_watcher_h_
