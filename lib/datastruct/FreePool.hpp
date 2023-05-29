#pragma once
#ifndef _tc_free_pool_h_
#define _tc_free_pool_h_ 1

#include "List.hpp"
#include <cassert>

template <class T>
class FreePool : private DoubleLinkList<T>
{
public:
   FreePool()
   {
   }

   virtual ~FreePool()
   {
      // DoubleLinkList base class peforms deallocation 
   }

   T * allocate()
   {
      T * pItem = this->popFront();
      if (pItem == NULL) {
         pItem = new T();
      }
      return pItem;
   }

   void retire(T * ptr)
   {
      if (ptr != 0)
      {
         this->append(ptr);
      }
   }

   // Transfer all the items in the list to the free pool 
   void retireAll(DoubleLinkList<T> & list)
   {
      retireAll(&list);
   }

   // Transfer all the items in the list to the free pool 
   void retireAll(DoubleLinkList<T> * pList)
   {
      if ((pList != 0) && (pList != this))
      {
         T * pItem; 
         while ((pItem = pList->pop_front()) != 0)
         {
            this->append(pItem);
         }
      }
   }

   int size()
   {
      return this->getNumMembers();
   }

   void dispose()
   {
      this->deleteMembers();
   }

   void ensureMinimumAvailable(int count)
   {
      while (this->getNumMembers() < count)
      {
         T * ptr = new T();
         append(ptr);
      }
   }

   //void reduceToThreshold(int threshold, int maxDeletes = 1)
   //{
   //   if (threshold < 0)
   //   {
   //      threshold = 0; 
   //   }
   //   while ((getNumMembers() > threshold) && (maxDeletes > 0))
   //   {
   //      T * ptr = pop_front();
   //      delete ptr;
   //      maxDeletes--;
   //   }
   //}

};

#endif // _free_pool_h_
