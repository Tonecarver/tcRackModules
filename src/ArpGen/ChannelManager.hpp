#pragma once 
#ifndef _tc_channel_mgr_h_
#define _tc_channel_mgr_h_  1

#include <rack.hpp>
#include <math.hpp>

// TODO: Rename this to OutputChannel

struct ChannelManager { 
    bool  mIsGateOpen = false;
    float mActiveVoltage = 0.f;

    void openGate(float voltage) {
        if (mIsGateOpen == false) {
            mIsGateOpen = true;
            mActiveVoltage = voltage;
        } else {
            // already open, remain open, change voltage 
            mActiveVoltage = voltage;
        }
    }

    void closeGate(float voltage) {
        // If the gate is open, and this closeure is related to the 
        // note/voltage that opened it, then close the gate.
        // Otherwise leave it closed.

        if (mIsGateOpen) {
            if (rack::math::isNear(mActiveVoltage, voltage)) { // epsilon test 
                mIsGateOpen = false;
                mActiveVoltage = 0.f;
            }
        }

        // else ignore close for previos open
        // event with different voltage closed in the meantime
        // i.e., overlap in the DelayList 

    }
};

#endif //  _tc_channel_mgr_h_
