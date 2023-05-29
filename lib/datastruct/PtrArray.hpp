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
      mpList = 0;
   }

   inline bool isEmpty() const {
      return mNumMembers == 0;
   }

   inline int size() const
   {
      return mNumMembers;
   }

   // TODO: delete this method, use size() instead
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
      if (mNumMembers <= idx)
      {
         mNumMembers = idx + 1;
      }
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
      insertAt(0);
   }

   void insertAt(int idx, T * pMember)
   {
      assert(pMember != 0);
      ensureCapacity(mNumMembers + 1);
      // shift contents up to make room 
      for (int i = mNumMembers; i > idx; i--)
      {
         mpList[i] = mpList[i-1];
      }
      mpList[idx] = pMember; // set new member at selected position
      mNumMembers++;
   }

   void remove(T * pMember)
   {
      assert(pMember != 0);
      int memberIndex = getIndexOf(pMember);
      if (memberIndex >= 0)
      {
         removeAt(memberIndex);
      }
   }

   T * removeFirst() 
   {
      return removeAt(0);
   }

   T * removeAt(int idx)
   {
      T * pMember = 0;
      if (isValidIndex(idx))
      {
         pMember = mpList[idx];

         for (int i = idx + 1; i < mNumMembers; i++)
         {
            mpList[i - 1] = mpList[i];
         }
         mNumMembers--;
         mpList[mNumMembers] = 0; // clear the vacated spot at the end of the list 
      }
      return pMember; 
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
      memset(mpList, 0, sizeof mpList[0] * mNumMembers);
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

template <class T>
class PtrArrayIterator
{
public:
	PtrArrayIterator( const PtrArray<T> &ptrArray )  
   { 
      mpPtrArray = &ptrArray; 
      mIndex = 0;
      getMember();
   }

	PtrArrayIterator( const PtrArray<T> * ptrArray )
   { 
      mpPtrArray = ptrArray; 
      mIndex = 0;
      getMember();
   }

	bool hasMore() const { return mpNext != 0; }
	
	T * getNext() // advance before returning
	{
		T * ptr = mpNext;
		if (mpNext != 0) { 
         mIndex++;
			getMember();
		}
      return ptr;
	}

	void reset() 
   { 
      mIndex = 0; 
      getMember(); 
   } 

private:
   const PtrArray<T> * mpPtrArray;
   int           mIndex;
	T           * mpNext;

   void getMember()
   {
      mpNext = (T*) (mIndex < mpPtrArray->getNumMembers()) ? mpPtrArray->getAt(mIndex) : 0 ;
   }
};


// REVERSE iterator:  iterate over array from last element to first 
template <class T>
class PtrArrayReverseIterator
{
public:
	PtrArrayReverseIterator( PtrArray<T> &ptrArray )  
   { 
      mpPtrArray = &ptrArray; 
      mIndex = ptrArray.getNumMembers() - 1;
      getMember();
   }

	PtrArrayReverseIterator( PtrArray<T> * ptrArray )
   { 
      mpPtrArray = ptrArray; 
      mIndex = ptrArray->getNumMembers() - 1;
      getMember();
   }

	bool hasMore() const { return mpNext != 0; }
	
	T * getNext() // advance before returning
	{
		T * ptr = mpNext;
		if (mpNext != 0) { 
         mIndex--;
			getMember();
		}
      return ptr;
	}

	void reset() 
   { 
      mIndex = mpPtrArray->getNumMembers() - 1;
      getMember();
   } 

private:
   PtrArray<T> * mpPtrArray;
   int           mIndex;
	T           * mpNext;

   void getMember()
   {
      mpNext = (T*) (mpPtrArray->isValidIndex(mIndex) ? mpPtrArray->getAt(mIndex) : 0);
   }
};

#endif // _ptr_array_h_
