#ifndef _sweep_selector_
#define _sweep_selector_

#include "../lib/FastRandom.hpp"

enum SweepAlgorithmEnum { 
    SWEEP_UP, 
    SWEEP_DOWN,
    SWEEP_ZIG_ZAG,
    SWEEP_RANDOM,
	NUM_SWEEP_ALGO
};

template<int SIZE>
struct SweepSelector {
	bool enabled[SIZE];  // true == step enabled, false == step disabled
	int numEnabled;      // number of 'true' entries in the enabled[] array 

	int lowIndex;   // index of first entry
	int highIndex;  // index of last entry to sweep across (may be active or inactive)

    int selection;  // index of the selected step

    bool amTravelingUp; // direction 
    SweepAlgorithmEnum algorithm;
	FastRandom random; 

    SweepSelector() {
        for (int k = 0; k < SIZE; k++) {
			enabled[k] = true;
		}
		numEnabled = SIZE;
		lowIndex = 0; 
		highIndex = SIZE -1;
        selection = 0; 
        algorithm = SWEEP_UP;
        amTravelingUp = true;
    }

	void enableAllSteps() {
        for (int k = 0; k < SIZE; k++) {
			enabled[k] = true;
		}
		countEnabled();
	}

	void setStepEnabled(int idx, bool beEnabled) {
		enabled[idx] = beEnabled;
		countEnabled();
	}

	bool isStepEnabled(int idx) {
		return enabled[idx];
	}

	bool getNumEnabled(int idx) {
		return numEnabled;
	}

	void setFirstIndex(int firstIdx) {
		setRange(firstIdx, highIndex);
	}

	void setLastIndex(int lastIdx) {
		setRange(lowIndex, lastIdx);
	}
	
	void setRange(int firstIdx, int lastIdx) {
		memset(enabled, 0, sizeof(enabled));
		for (int k = firstIdx; k <= lastIdx; k++) {
			enabled[k] = true;
		}
		countEnabled();
	}

    void setToFirst() { 
        selection = lowIndex;
    }
    
    void setToLast() { 
        selection = highIndex;
    }
    
    void setAlgorithm(SweepAlgorithmEnum algo) {
        algorithm = algo;
    }

	int getLowIndex() { 
		return lowIndex;
	}

	int getHighIndex() { 
		return highIndex;
	}
	
	int getSelection() { 
		return selection;
	}

    int select() {
		if (numEnabled > 0) {
			switch (algorithm) {
				case SWEEP_UP:
					selectUp();
					break;
				case SWEEP_DOWN:
					selectDown();
					break;
				case SWEEP_ZIG_ZAG:
					selectZigZag();
					break;
				case SWEEP_RANDOM:
					selectRandom();
					break;
				default:
					DEBUG("SweepSelector::select()  BAD ALGO ..%d", algorithm);
					break;
			}
			selection = clamp(selection, lowIndex, highIndex);
		}
		return selection;
    }

protected:
	// TODO: include option to SKIP non-active when making selections
	//   or make that part of a different logic
    void selectUp() {
        amTravelingUp = true;
		while (true) {
	        selection ++;
			if (selection > highIndex) {
				selection = lowIndex;
				return;
			}
			if (enabled[selection]) {
				return;
			}
		}
    }
 
    void selectDown() {
        amTravelingUp = false;
		while (true) {
	        selection --;
			if (selection < lowIndex) {
				selection = highIndex;
				return;
			}
			if (enabled[selection]) {
				return;
			}
		}
    }
 
    void selectZigZag() {
        if (amTravelingUp) {
			while (true) {
				selection ++;
				if (selection > highIndex) {
	                amTravelingUp = false;
    	            selection = highIndex - 1;
					return;
				}
				if (enabled[selection]) {
					return;
				}
			}
        } else {
			while (true) {
				selection --;
				if (selection < lowIndex) {
					amTravelingUp = true;
					selection = lowIndex + 1;
					return;
				}
				if (enabled[selection]) {
					return;
				}
			}
        }
    }

    void selectRandom() {
		int newSelection = lowIndex;
        int count = int(random.generateZeroToOne() * float(numEnabled - 1));
		while (count > 0) {
			if (enabled[newSelection]) {
				count--;
			}
			newSelection++;
		}

        if (newSelection > selection) {
            amTravelingUp = true;
        } else if (newSelection < selection) {
            amTravelingUp = false;
        }
        selection = newSelection; 
    }

	void countEnabled() {
		numEnabled = 0;
		lowIndex = 0;
		highIndex = 0; 
		bool lowFound = false;
		for (int k = 0; k < SIZE; k++) {
			if (enabled[k]) {
				numEnabled++;
				if (! lowFound) {
					lowIndex = k;
					lowFound = true;
				}
				highIndex = k;
			}
		}
		if (numEnabled > 0) {
		    selection = clamp(selection, lowIndex, highIndex);
		} else {
			selection = -1; // no selection 
		}
	}

};

#endif // _sweep_selector_