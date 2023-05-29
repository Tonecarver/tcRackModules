#pragma once
#ifndef _list_hpp_
#define _list_hpp_

#ifndef NULL
#define NULL (0)
#endif 

struct ListNode {
    struct ListNode * pNext;
    struct ListNode * pPrev;

    ListNode() : 
        pNext(NULL), 
        pPrev(NULL)
    {
    }

    void detach() {
        pNext = NULL;
        pPrev = NULL;
    }

    ListNode * getNextNode() const {
        return pNext;
    }

    ListNode * getPrevNode() const {
        return pPrev;
    }

    bool hasNextNode() const { 
        return pNext != 0;
    }

    bool hasPrevNode() const { 
        return pPrev != 0;
    }
    
    bool isFirstNode() const {
        return pPrev == NULL && pNext != NULL;
    }

    bool isMiddleNode() const {
        return pPrev != NULL && pNext != NULL;
    }
    
    bool isLastNode() const {
        return pPrev != NULL && pNext == NULL;
    }
    
    bool isDetached() const {
        return pPrev == NULL && pNext == NULL;
    }

};


template <class T>
struct DoubleLinkList {
    T * pHead;
    T * pTail;
    int numMembers;

    DoubleLinkList() {
        pHead = NULL;
        pTail = NULL;
        numMembers = 0;
    }

    ~DoubleLinkList() {
        deleteMembers();
    }

    int size() const { return numMembers; }

    bool isEmpty() const { return pHead == NULL; }

    void deleteMembers() { 
        while (pHead != NULL) {
            T * pItem = pHead;
            pHead = (T*) pHead->pNext;
            delete pItem;
        }
        pHead = NULL;
        pTail = NULL;
        numMembers = 0;
    }

    T * peekFront() const { 
        return pHead;
    }

    T * peekTail() const { 
        return pTail;
    }

    T * popFront() { 
        if (pHead != NULL) {
            T * pItem = pHead;
            pHead = (T*) pItem->pNext;
            if (pHead != NULL) {
                pHead->pPrev = NULL;
            } else {
                pTail = NULL;
            }
            numMembers--;
            pItem->detach();
            return pItem;
        }
        return NULL;
    }

    T * popTail() { 
        if (pTail != NULL) {
            T * pItem = pTail;
            pTail = (T*) pItem->pPrev;
            if (pTail != NULL) {
                pTail->pNext = NULL;
            } else {
                pHead = NULL;
            }
            numMembers--;
            pItem->detach();
            return pItem;
        }
        return NULL;
    }

    void pushFront(T * pItem) { 
        if (pHead == NULL) {
            pHead = pItem;
            pTail = pItem;
            pItem->detach();
        }
        else // pHead != null
        {
            pItem->pPrev = NULL;
            pItem->pNext = pHead;
            pHead->pPrev = pItem;
            pHead = pItem;
        }
        numMembers++;
    }

    void pushTail(T * pItem) { 
        if (pTail == NULL) {
            pHead = pItem;
            pTail = pItem;
            pItem->detach();
        }
        else // pTail != null
        {
            pTail->pNext = pItem;
            pItem->pPrev = pTail;
            pItem->pNext = NULL;
            pTail = pItem;
        }
        numMembers++;
    }

    void insertBefore(T * pMember, T * pItem) {       
        if (pHead == NULL || pHead == pMember) {
            pushFront(pItem);
        } else {
            pItem->pPrev = pMember->pPrev; 
            pItem->pNext = pMember;
            pMember->pPrev->pNext = pItem;
            pMember->pPrev = pItem;
            numMembers++;
        }
    }

    T * remove(T * pItem) {
        T * pMember = findMember(pItem);
        if (pMember) {

            if (pHead == pMember) {
                pHead = (T*) pMember->pNext;
            }

            if (pTail == pMember) {
                pTail = (T*) pMember->pPrev;
            }

            if (pMember->pPrev) {
                pMember->pPrev->pNext = (T*) pMember->pNext;
            }

            if (pMember->pNext) {
                pMember->pNext->pPrev = (T*) pMember->pPrev;
            }
            numMembers--;
            pMember->detach();
        }
        return pMember;
    }

    T * findMember(T * pItem) const { 
        T * pMember = pHead; 
        while (pMember != NULL && pMember != pItem) {
            pMember = (T*) pMember->pNext;
        }
        return pMember; // may be null 
    }

    // -----------------------------------------
    // get the N'th member. Slow traversal
    // -----------------------------------------

    T * getAt(int idx) const { 
        if (idx >= 0 && idx < numMembers) {
            T * pMember = pHead;
            while (idx > 0) {
                idx--; 
                pMember = (T*) pMember->pNext; 
            }
            return pMember;
        }
        return 0; // bad index 
    }

    // -----------------------------------------
    // Queue-like operations
    // -----------------------------------------

    inline void enqueue(T * pItem) {
        pushTail(pItem);
    }

    inline T * dequeue() {
        return popFront();
    }

    // -----------------------------------------
    // Double-ended-list operations 
    // -----------------------------------------

    inline void append(T * pItem) {
        pushTail(pItem);
    }

    inline T * prepend() {
        return pushFront();
    }
};

template <class T>
struct DoubleLinkListIterator {
    const DoubleLinkList<T> * mpList;
    T * mpNext;

    DoubleLinkListIterator(DoubleLinkList<T> const& list) {
        mpList = &list;
        reset();
    }

    DoubleLinkListIterator(const DoubleLinkList<T> * pList) {
        mpList = pList;
        reset();
    }

    void set(DoubleLinkList<T> * pList) {
        mpList = pList;
        reset();
    }

    void set(DoubleLinkList<T> & list) {
        mpList = &list;
        reset();
    }

    bool hasMore() const { 
        return mpNext != NULL;
    }

    T * getNext() { 
        T * pMember = mpNext;
        if (pMember) {
            mpNext = (T*) pMember->pNext;
        } else { 
            mpNext = NULL;
        }
        return pMember;
    }

    T * peekNext() const {
        return mpNext;
    }
    
    void reset() { // point back to beginning of list 
        mpNext = 0;
        if (mpList) {
            mpNext = mpList->peekFront();
        }
    }
};

template <class T>
struct DoubleLinkListIteratorBiDirectional {
    const DoubleLinkList<T> * mpList;
    T * mpNext;
    bool mIsForward;

    DoubleLinkListIteratorBiDirectional(const DoubleLinkList<T> * pList = 0, bool goForward = true) {
        mpList = pList;
        mIsForward = goForward;
        reset();
    }

    DoubleLinkListIteratorBiDirectional(DoubleLinkList<T> const & list, bool goForward) {
        mpList = &list;
        mIsForward = goForward;
        reset();
    }

    void set(DoubleLinkList<T> * pList) {
        mpList = pList;
        reset();
    }

    void set(DoubleLinkList<T> & list) {
        mpList = &list;
        reset();
    }

    bool hasMore() const { 
        return mpNext != NULL;
    }

    bool isForward() const { 
        return mIsForward == true; 
    }

    bool isReversed() const { 
        return mIsForward == false; 
    }

    void setDirectionForward() { 
        mIsForward = true;
    }

    void setDirectionReverse() { 
        mIsForward = false;
    }
    
    void toggleDirection() { 
        mIsForward = (! mIsForward);
    }
    
    T * getNext() { 
        T * pMember = mpNext;
        if (pMember) {
            if (mIsForward) {
                mpNext = (T*) pMember->pNext;
            } else {
                mpNext = (T*) pMember->pPrev;
            }
        } else { 
            mpNext = NULL;
        }
        return pMember;
    }

    T * peekNext() const {
        return mpNext;
    }

    void reset() { // point back to beginning of list 
        mpNext = 0;
        if (mpList) {
            if (mIsForward) {
                mpNext = mpList->peekFront();
            } else { 
                mpNext = mpList->peekTail();
            }
        }
    }
};





// struct DelayListNode : ListNode {
//     long delay;

//     DelayListNode() {
//         this->delay = 0;
//     }
    
//     DelayListNode(long delay) { 
//         this->delay = delay;
//     }
// };


// template <class T>
// struct DelayList : DoubleLinkList<T> {
//     DelayList() {
//     }

//     ~DelayList() {
//     }

//     void insertTimeOrdered(T * pItem) {
//         long delayRemaining = pItem->delay;

//         T * pMember = DoubleLinkList<T>::peekFront(); 
//         while (pMember != NULL && delayRemaining > pMember->delay) {
//             delayRemaining -= pMember->delay;
//             pMember = (T*) pMember->pNext;
//         }

//         pItem->delay = delayRemaining; 

//         if (pMember != NULL) {
//             pMember->delay -= delayRemaining;
//             DoubleLinkList<T>::insertBefore(pMember, pItem);
//         } else {
//             DoubleLinkList<T>::pushTail(pItem);
//         }
//     }

// };


#endif // _ list_hpp_