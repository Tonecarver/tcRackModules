#ifndef _tc_stack_hpp_
#define _tc_stack_hpp_


#include "List.hpp"

template <class T>
struct Stack {
    T * pHead;
    T * pTail;
    int numMembers;

    Stack() {
        pHead = NULL;
        pTail = NULL;
        numMembers = 0;
    }

    ~Stack() {
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

    T * bottom() const { 
        return pHead;
    }
    
    T * top() const { 
        return pTail;
    }

    void push(T * pItem) { 
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

    T * pop() { 
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

};


// Iterate stack from bottom (oldest) to top (newest)
template <class T>
struct StackIterator {
    Stack<T> * pStack;
    T * pNext;

    StackIterator(Stack<T> & stack) {
        pStack = &stack;
        pNext = stack.bottom();
    }

    bool hasMore() const { 
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

    void reset() { // point back to bottom of stack 
        pNext = pStack->bottom();
    }

};



#endif // _tc_stack_hpp_