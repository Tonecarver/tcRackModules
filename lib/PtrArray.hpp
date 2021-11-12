#ifndef _ptr_array_h_
#define _ptr_array_h_  1

#include <assert.h>

// An array of pointers to objects of type T 
// List is variable length.
// Pointers are just BORROWED/SHARED by this object. 
template <class T>
class PtrArray
{
public:
   PtrArray(int initialSize = 16)
      : mpList(0)
      , mPhysicalCapacity(0)
      , mNumMembers(0)
   {
      ensureCapacity(initialSize);
   }

   ~PtrArray()
   {
      delete mpList;
   }

   inline int getNumMembers() const
   {
      return mNumMembers;
   }

   inline bool contains(T * pCandidate) const
   {
      int index = getIndexOf(pCandidate);
      return (index >= 0);
   }

   int getIndexOf(T * pCandidate)
   {
      for (int i = 0; i < mNumMembers; i++)
      {
         if (mpList[i] == pCandidate)
         {
            return i; // found a match 
         }
      }
      return -1; // not in list 
   }

   inline bool isValidIndex(int idx) const
   {
      return (0 <= idx && idx < mNumMembers);
   }

   inline T * getAt(int idx) const
   {
      assert(0 <= idx && idx < mNumMembers);
      return mpList[idx];
   }

   inline void putAt(int idx, T * pMember)
   {
      assert(pMember != 0);
      ensureCapacity(idx + 1);
      mpList[idx] = pMember;
   }

   // add member as last element 
   void append(T * pMember)
   {
      assert(pMember != 0);
      ensureCapacity(mNumMembers + 1);
      mpList[mNumMembers] = pMember;
      mNumMembers++;
   }

   // add member as first element 
   void insert(T * pMember)
   {
      assert(pMember != 0);
      ensureCapacity(mNumMembers + 1);
      // shift contents up to make room 
      for (int i = mNumMembers; i > 0; i--)
      {
         mpList[i] = mpList[i-1];
      }
      mpList[0] = pMember; // set new member as first 
      mNumMembers++;
   }

   void remove(T * pMember)
   {
      assert(pMember != 0);
      int memberIndex = getIndexOf(pMember);
      if (memberIndex >= 0)
      {
         for (int i = memberIndex; i < mNumMembers; i++)
         {
            mpList[i-1] = mpList[i];
         }
         mNumMembers--;
         mpList[mNumMembers] = 0; // clear the vacated spot at the end of the list 
      }
   }

   T * removeLast()
   {
      T * pMember = 0;
      if (mNumMembers > 0)
      {
         mNumMembers--;
         pMember = mpList[mNumMembers];
         mpList[mNumMembers] = 0;
      }
      return pMember;
   }

   // just discard, do not delete members 
   void clear()
   {
      for (int i = 0; i < mNumMembers; i++)
      {
         mpList[i] = 0;
      }
      mNumMembers = 0;
   }

   // delete the members and clear the list 
   void deleteContents()
   {
      for (int i = 0; i < mNumMembers; i++)
      {
         delete mpList[i];
         mpList[i] = 0;
      }
      mNumMembers = 0;
   }

   // grow the array (if necessary) to hold at least this many members 
   void ensureMinimumSize(int numMembers)
   {
      ensureCapacity(numMembers + 1);
      if (mNumMembers < numMembers)
      {
         mNumMembers = numMembers;
      }
   }

private:
   T ** mpList;
   int  mPhysicalCapacity;
   int  mNumMembers; 

   enum { 
      GROWTH_MARGIN = 8
   };


   void ensureCapacity(int capacity)
   {
      if (mPhysicalCapacity < capacity)
      {
         int newPhysicalCapacity = capacity + GROWTH_MARGIN;
         // allocate an array of pointers to items of type T 
         T ** pNewList = new T*[ newPhysicalCapacity ];
         // copy existing list to new list
         for (int i = 0; i < mNumMembers; i++)
         {
            pNewList[i] = mpList[i];
         }
         // zero the margin/growth area
         for (int i = mNumMembers; i < newPhysicalCapacity; i++)
         {
            pNewList[i] = 0;
         }
         // replace list with new list
         // update physical capacity 
         delete mpList;
         mpList = pNewList;
         mPhysicalCapacity = newPhysicalCapacity;
      }
   }

   // can shrink too, but do not need it so far 

};


#endif // _ptr_array_h_
