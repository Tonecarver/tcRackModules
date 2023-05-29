#pragma once
#ifndef _tc_input_channel_h_ 
#define _tc_input_channel_h_  1 


struct InputChannel {
    bool mIsGateOpen;
    float mActiveVoltage;
    // TODO: this is same struct as OuptputChannel ... combine? 
}; 

struct ChannelState {
    bool mIsGateOpen;
    float mActiveVoltage;
}; 


struct PolyphonicChannel {
    ChannelState channels[16]; 
}; 

...... needs work ... 

#endif //  _tc_input_channel_h_ 