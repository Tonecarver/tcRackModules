#ifndef _tc_ppolygatedcv_h_
#define _tc_ppolygatedcv_h_ 1

#include "Gate.hpp"
#include "PolyVoltages.hpp"

struct PolyGatedCv {
	PolyVoltages mControlVoltages;
	PolyVoltages mGateVoltages;
	Gate         mGates[PORT_MAX_CHANNELS];

	bool process(int numCvChannels, float * cvVoltages, int numGateChannels, float * gateVoltages) {
		bool changeDetected = false;
		if (mControlVoltages.process(numCvChannels, cvVoltages)) {
			changeDetected = true;
		}
		if (mGateVoltages.process(numGateChannels, gateVoltages)) {
			changeDetected = true;
		}
		if (changeDetected) {
			for (int c = 0; c < numCvChannels; c++) { // limit/stretch gates to number of CV channels
				float gateVoltage = (c >= numGateChannels ? gateVoltages[0] : gateVoltages[c]); // spread mono gates to all cv channels
				mGates[c].setOpen( gateVoltage >= 9.f );
			}
			for (int c = numCvChannels; c < PORT_MAX_CHANNELS; c++) {
				mGates[c].close();
			}
		}
		return changeDetected;
	}
	
	float getControlVoltage(int channelNumber) {
		return mControlVoltages.getVoltage(channelNumber);
	}

	float getGateVoltage(int channelNumber) { 
		return mGateVoltages.getVoltage(channelNumber); 
	}

	float getPolyGateVoltage(int channelNumber) { 
		return (channelNumber >= mGateVoltages.getNumChannels() ? mGateVoltages.getVoltage(0) : mGateVoltages.getVoltage(channelNumber)); 
	}

	bool isGateOpen(int channelNumber) const {
		return mGates[channelNumber].isOpen();
	}

	bool isGateLeadingEdge(int channelNumber) const {
		return mGates[channelNumber].isLeadingEdge();
	}

	bool isGateTrailingEdge(int channelNumber) const {
		return mGates[channelNumber].isTrailingEdge();
	}

	bool isGateClosed(int channelNumber) const {
		return mGates[channelNumber].isClosed();
	}
};

#endif //  _tc_ppolygatedcv_h_
