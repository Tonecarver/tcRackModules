#ifndef _thread_safe_list_hpp_
#define _thread_safe_list_hpp_

#include <mutex>

#ifndef NULL
#define NULL (0)
#endif 


// Type T shoud be a subclass of ListNode (see List.hpp)
template <class T>
struct ThreadSafeDoubleLinkList {
    T * pHead;
    T * pTail;
    int numMembers;
    std::mutex list_mutex;

    ThreadSafeDoubleLinkList() {
        pHead = NULL;
        pTail = NULL;
        numMembers = 0;
    }

    ~ThreadSafeDoubleLinkList() {
        deleteMembers();
    }

    int size() const { return numMembers; }

    bool isEmpty() const { 
        const std::lock_guard<std::mutex> lock(list_mutex);
        return pHead == NULL; 
    }

    void deleteMembers() { 
        const std::lock_guard<std::mutex> lock(list_mutex);
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
        const std::lock_guard<std::mutex> lock(list_mutex);
        return pHead;
    }

    T * popFront() { 
        const std::lock_guard<std::mutex> lock(list_mutex);
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
        const std::lock_guard<std::mutex> lock(list_mutex);
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
        const std::lock_guard<std::mutex> lock(list_mutex);
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
        const std::lock_guard<std::mutex> lock(list_mutex);
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
        const std::lock_guard<std::mutex> lock(list_mutex);
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

    inline void enqueue(T * pItem) {
        pushTail(pItem);
    }

    inline T * dequeue() {
        return popFront();
    }

};


#endif // _thread_safe_list_hpp_