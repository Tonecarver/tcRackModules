#ifndef _sample_rate_calculator_
#define _sample_rate_calculator_

struct SampleRateCalculator {
    float sampleRate;
    float bpm;
	float samplesPerBeat; 

    SampleRateCalculator() {
        sampleRate = 44100.0;
        bpm = 120.f;
		computeSamplesPerBeat();
    }

    void setSampleRate(float sampleRate) {
		if (this->sampleRate != sampleRate) {
	        this->sampleRate = sampleRate;
			computeSamplesPerBeat();
		}
    }

    void setBpm(float bpm) {
        if (this->bpm != bpm) {
			this->bpm = bpm;
			computeSamplesPerBeat();
		}
    }

    void setBpmFromNumSamples(float numSamples) {
        this->bpm = samplesToBpm(numSamples);
		computeSamplesPerBeat();
    }

	float getSamplesPerBeat() const {
		return samplesPerBeat; 
	}
	
    float computeSamplesForBeats(float numBeats) {
        float numSamples = samplesPerBeat * numBeats;
		return numSamples;
    }

    float computeSamplesForMilliseconds(float numMillis) {
        float numSamples = millisToSamples(numMillis);
		return numSamples;
    }

	float bpmToSamples(float bpm) const {
		if (bpm > 0.f) {
			return (sampleRate * 60.f) / bpm;
		}
		return 0.f; 
	}

	float millisToSamples(float millis) const {
		if (millis > 0.f) {
			return (sampleRate * 0.001f) * millis; 
		}
		return 0.;
	}

	float samplesToBpm(float numSamples) const {
		if (numSamples > 0.f) { 
			return (sampleRate * 60.f) / numSamples;
		}
		return 0.f;
	}

private:
	void computeSamplesPerBeat() {
		samplesPerBeat = bpmToSamples(bpm);
	}
};


#endif // _sample_rate_calculator_
