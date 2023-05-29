#pragma once
#ifndef _tc_cv_event_h_
#define _tc_cv_event_h_ 1

#include "../datastruct/DelayList.hpp"

enum CvEventType { 
    CV_EVENT_TRIGGER,
    CV_EVENT_LEADING_EDGE,
    CV_EVENT_TRAILING_EDGE,
    CV_EVENT_NON_SPECIFIC,
};

struct CvEvent : public DelayListNode { 
    int   channel; 
    float voltage;
    CvEventType eventType;

    CvEvent() :
        DelayListNode(0), channel(0), voltage(0.f), eventType(CV_EVENT_NON_SPECIFIC)
    {
    }

    // CvEvent(long delay, int channel, float voltage, CvEventType eventType = CV_EVENT_NON_SPECIFIC) :
    //     DelayListNode(delay), channel(channel), voltage(voltage), eventType(eventType)
    // {
    // }

    void set(long delay, int channel, float voltage, CvEventType eventType) {
        this->delay = delay;
        this->channel = channel;
        this->voltage = voltage;
        this->eventType = eventType;
    }

    bool isTrigger() const { 
        return eventType == CV_EVENT_TRIGGER;
    }

    bool isLeadingEdge() const { 
        return eventType == CV_EVENT_LEADING_EDGE;
    }

    bool isTrailingEdge() const { 
        return eventType == CV_EVENT_TRAILING_EDGE;
    }

    bool isNonSpecific() const { 
        return eventType == CV_EVENT_NON_SPECIFIC;
    }
}; 

#endif // _tc_cv_event_h_
