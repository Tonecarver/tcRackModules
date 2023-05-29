#pragma once
#ifndef _delay_list_hpp_
#define _delay_list_hpp_  1

#include "List.hpp"

struct DelayListNode : ListNode {
    long delay;

    DelayListNode() {
        this->delay = 0;
    }
    
    DelayListNode(long delay) { 
        this->delay = delay;
    }
};


// Class T should be qa subclass of the  DelayListNode
template <class T>
struct DelayList : public DoubleLinkList<T> {
    DelayList() {
    }

    ~DelayList() {
    }

    void insertTimeOrdered(T * pItem) {
        long delayRemaining = pItem->delay;

        T * pMember = DoubleLinkList<T>::peekFront(); 
        while (pMember != NULL && delayRemaining > pMember->delay) {
            delayRemaining -= pMember->delay;
            pMember = (T*) pMember->pNext;
        }

        pItem->delay = delayRemaining; 

        if (pMember != NULL) {
            pMember->delay -= delayRemaining;
            DoubleLinkList<T>::insertBefore(pMember, pItem);
        } else {
            DoubleLinkList<T>::pushTail(pItem);
        }
    }

    bool hasExpiredItems() const { 
        T * pMember = DoubleLinkList<T>::peekFront(); 
        return pMember && pMember->delay <= 0;
    }

    T * getNextExpired() {
        T * pMember = DoubleLinkList<T>::peekFront(); 
        if (pMember && pMember->delay <= 0) {
            return DoubleLinkList<T>::popFront(); 
        }
        return NULL; // empty list, or item(s) not expired yet 
    }

    void passTime(long count) { 
        T * pMember = DoubleLinkList<T>::peekFront(); 
        if (pMember) {
            pMember->delay -= count;
        }
    }

};


#endif // _delay_list_hpp_ 
