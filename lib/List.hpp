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

    T * peekFront() { 
        return pHead;
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

    T * findMember(T * pItem) { 
        T * pMember = pHead; 
        while (pMember != NULL && pMember != pItem) {
            pMember = (T*) pMember->pNext;
        }
        return pMember; // may be null 
    }

    inline void enqueue(T * pItem) {
        pushTail(pItem);
    }

    inline T * dequeue() {
        return popFront();
    }

};

template <class T>
struct DoubleLinkListIterator {
    DoubleLinkList<T> * pList;
    T * pNext;

    DoubleLinkListIterator(DoubleLinkList<T> & list) {
        pList = &list;
        pNext = list.peekFront();
    }

    bool hasMore() { 
        return pNext != NULL;
    }

    T * getNext() { 
        T * pMember = pNext;
        if (pMember) {
            pNext = (T*) pMember->pNext;
        } else { 
            pNext = NULL;
        }
        return pMember;
    }

    void reset() { // point back to beginning of list 
        pNext = pList->peekFront();
    }

};

struct DelayListNode : ListNode {
    long delay;

    DelayListNode() {
        this->delay = 0;
    }
    
    DelayListNode(long delay) { 
        this->delay = delay;
    }
};


template <class T>
struct DelayList : DoubleLinkList<T> {
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

};


#endif // _ list_hpp_