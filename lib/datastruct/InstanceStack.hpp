#ifndef _instance_stack_h_ 
#define _instance_stack_h_  1

#include "ResizableBuffer.hpp"

template <class T>
class InstanceStack
{
public:
   InstanceStack(int initialCapacity = 32)
   {
      mBuffer.ensureCapacity(initialCapacity);
   }

   void push(T val)
   {
      mBuffer.ensureCapacity(mNumMembers + 1);
      mBuffer.putAt(mNumMembers, val);
      mNumMembers++;
   }

   T pop()
   {
      if (mNumMembers > 0)
      {
         mNumMembers--;
         return mBuffer.getAt(mNumMembers);
      }
      return (T) 0;
   }

   T peek() const
   {
      if (mNumMembers > 0)
      {
         return mBuffer.getAt(mNumMembers-1);
      }
      return (T) 0;
   }

   bool isEmpty() const
   {
      return mNumMembers == 0;
   }

   void clear()
   {
      mNumMembers = 0; 
   }

   int getNumMembers() const
   {
      return mNumMembers;
   }

private: 
   ResizableBuffer<T> mBuffer;
   int                mNumMembers;
};


typedef InstanceStack<bool>  BooleanStack;
typedef InstanceStack<int>   IntegerStack;

#endif // _instance_stack_h_ 
