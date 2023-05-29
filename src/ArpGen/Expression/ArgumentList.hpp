#ifndef _argument_list_h_
#define _argument_list_h_  1

#include "Expression.hpp" 
#include "../../../lib/datastruct/List.hpp"
#include "../../../lib/datastruct/PtrArray.hpp"
#include "../../../lib/io/ByteStream.hpp"

//#include "FixedLengthArray.h"
//#include "SafeString.h"
//#include "debugtrace.h"

#include <string>
#include <cassert>

#define MAX_ARGUMENT_COUNT (4)

class Expression;

// todo: PtrArray<SafeString> wastes a lot of space .. the capacity_overhead is 32 
//  if the max is fixed, allocate an exact size array.

class ArgumentExpressionList
{
public:
   ArgumentExpressionList()
   {
   }

   virtual ~ArgumentExpressionList()
   {
   }

   int size() const
   {
      return mArgumentExpressions.size();
   }

   bool isEmpty() const
   {
      return mArgumentExpressions.isEmpty();
   }

   bool isFull()
   {
      return mArgumentExpressions.size() == MAX_ARGUMENT_COUNT;
   }

   void addArgument(Expression * pExpression)
   {
      if (pExpression == 0)
      {
         // todo: log
         return;
      }

      if (isFull())
      {
         // todo: log
         return;
      }

      // todo: check already contains?
      mArgumentExpressions.append(pExpression);
   }

   DoubleLinkList<Expression> & getArgumentExpressions()
   {
      return mArgumentExpressions;
   }

   bool serialize(ByteStreamWriter * pWriter);

   bool unserialize(ByteStreamReader * pReader);

private:
   DoubleLinkList<Expression> mArgumentExpressions;
};

// array of formal argument names 
class FormalArgumentList 
{
public:
   FormalArgumentList()
      : mNumMembers(0)
   {
   }

   virtual ~FormalArgumentList()
   {
   }

   int size() const
   {
      return mNumMembers;
   }

   bool isEmpty() const
   {
      return mNumMembers == 0;
   }

   bool isFull() const
   {
      return mNumMembers == MAX_ARGUMENT_COUNT;
   }
   
   bool isValidIndex(int idx) const
   {
      return ((0 <= idx) && (idx < mNumMembers));
   }

   void addParameterName(const char * pName)
   {
      if (pName == 0)
      {
         // todo:log 
         return;
      }

      // todo: should check for all blank 

      if (pName[0] == 0)
      {
         // todo:log 
         return;
      }

      if (isFull())
      {
         // todo: log
         return;
      }

      // todo: check contains?

      mArgumentNames[ mNumMembers ] = pName;
      mNumMembers++;
   }

   bool contains(const char * pName) const
   {
      for (int i = 0; i < mNumMembers; i++) {
         if (mArgumentNames[i] == pName) {
            return true;
         }
      }
      return false; 
   }

   std::string getNameAt(int idx) const 
   {
      if (isValidIndex(idx))
      {
         return mArgumentNames[idx];
      }
      return "<bad-index>";
   }

   bool serialize(ByteStreamWriter * pWriter);
   bool deserialize(ByteStreamReader * pReader);

private:
   std::string mArgumentNames[MAX_ARGUMENT_COUNT];
   int         mNumMembers;
};



// array of actual argument values 
class ArgumentValuesList : public ListNode
{
public:
   ArgumentValuesList()
      : mNumMembers(0)
   {
   }

   virtual ~ArgumentValuesList()
   {
   }

   int size() const
   {
      return mNumMembers;
   }

   bool isEmpty() const
   {
      return mNumMembers == 0;
   }

   bool isFull() const
   {
      return mNumMembers == MAX_ARGUMENT_COUNT;
   }

   bool isValidIndex(int idx) const
   {
      return ((0 <= idx) && (idx < mNumMembers));
   }

   void addParameterValue(float value)
   {
      if (isFull())
      {
         // todo: log
         return;
      }
      mArgumentValues[ mNumMembers ] = value;
      mNumMembers++;
   }

   float getValueAt(int idx) const
   {
      if (isValidIndex(idx))
      {
         return mArgumentValues[ idx ];
      }
      return 0.0;
   }

   void setValueAt(int idx, float value)
   {
      if (isValidIndex(idx))
      {
         mArgumentValues[ idx ] = value;
      }
   }

   void clear()
   {
      mNumMembers = 0;
   }

   bool serialize(ByteStreamWriter * pWriter);
   bool deserialize(ByteStreamReader * pReader);

private:
   float mArgumentValues[MAX_ARGUMENT_COUNT];
   int    mNumMembers;
};



#endif // _argument_list_h_