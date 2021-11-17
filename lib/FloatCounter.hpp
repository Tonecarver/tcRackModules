#ifndef _tc_float_counter_h_ 
#define _tc_float_counter_h_ 

#include "plugin.hpp"

struct FloatCounter {
	float count;
	float target; 
    bool onTarget; 
    //int ticks;   // DEBUG 

	FloatCounter() { 
		count = 0.f; 
		target = 100.f;
        onTarget = false;
        //ticks = 0;
	}

    float getCount() const {
        return count;
    }

    float getTarget() const {
        return target;
    }
    
    bool isAtTarget() const {
        return onTarget;
    }

	void setTarget(float target) {
		this->target = target;
	}

	void reset() { 
		count = 0; 
	}

	bool tick() {
        //ticks++;
        onTarget = false;
		count += 1.0; 
		if (count >= target) {
            onTarget = true;
			count = count - target; // overshoot 
            //DEBUG("FloatCounter: LeadingEdge: count %f, target %f, overshoot %f  (ticks = %d)", count, target, count - target, ticks);
            // ticks = 0;
		}
        return onTarget;
	}

};

#endif // _tc_float_counter_h_ 
