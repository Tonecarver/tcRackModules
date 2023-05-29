#pragma once
#ifndef _tc_lsystem_production_h_
#define _tc_lsystem_production_h_ 1

#include "../../plugin.hpp"

#include "../Expression/Expression.hpp"
#include "../Expression/ExpressionContext.hpp"
#include "../Expression/ArgumentList.hpp"

#include "LSystemTerm.hpp"
#include "LSystemTermCounter.hpp"

#include "../../../lib/datastruct/List.hpp"
#include "../../../lib/datastruct/FreePool.hpp"
#include "../../../lib/random/FastRandom.hpp"
#include "../../../lib/io/PrintWriter.hpp"
#include "../../../lib/io/ByteStream.hpp"

//#include "ResizableBuffer.h"
//#include "FastMath.h"
// #include "debugtrace.h"

#include <string>
#include <cstdio>

//template <class T>
//class ResizableArray
//{
//public:
//   ResizableArray()
//      : mSize(0)
//   {
//   }
//
//   // TODO: dtor 
//
//   T getAt(int idx)
//   {
//      assert((0 <= idx) && (idx < mSize));
//
//      return mBackingArray.getAt(idx);
//   }
//
//   void putAt(int idx, T & value)
//   {
//      assert((0 <= idx) && (idx < mSize));
//
//      mBackingArray.putAt(idx, value);
//   }
//
//   void append(T & value)
//   {
//      mBackingArray.ensureCapacity(mSize + 1);
//      mBackingArray.putAt(mSize, value);
//      mSize++;
//   }
//
//   int size() 
//   {
//      return mSize;
//   }
//
//   void clear()
//   {
//      mSize = 0; 
//      mBackingArray.dispose();
//   }
//
//private:
//   ResizableBuffer<T> mBackingArray;
//   int mSize;
//};
//
//class DiscreteIntegers // : public Node
//{
//public:
//   DiscreteIntegers()
//   {
//   }
//   
//   // dtor not needed
//   
//   void addValue(int value)
//   {
//      if (contains(value) == false)
//      {
//         mDiscretes.append(value);
//      }
//   }
//
//   void addRange(int val1, int val2)
//   {
//      int minVal = (val1 < val2 ? val1 : val2);
//      int maxVal = (val1 < val2 ? val2 : val1);
//
//      for (int v = minVal; v <= maxVal; v++)
//      {
//         addValue(v);
//      }
//   }
//
//   int size()
//   {
//      return mDiscretes.size();
//   }
//
//   int getAt(int idx)
//   {
//      // assert valid range 
//      return mDiscretes.getAt(idx);
//   }
//
//   void write(Writer & writer)
//   {
//      int numDiscretes = mDiscretes.size(); 
//      for (int i = 0; i < numDiscretes; i++)
//      {
//         if (i > 0)
//         {
//            writer.write(',');
//         }
//         writer.write( mDiscretes.getAt(i) );
//      }
//   }
//
//private:
//   ResizableArray<int> mDiscretes;
//
//   bool contains(int value)
//   {
//      int length = mDiscretes.size();
//      for (int i = 0; i < length; i++)
//      {
//         if (mDiscretes.getAt(i) == value)
//         {
//            return true;
//         }
//      }
//      return false; 
//   }
//};
//
////class IntegerRange : public Node
////{
////public:
////   IntegerRange(int minValue, int maxValue)
////      : mMinValue(minValue < maxValue ? minValue : maxValue)
////      , mMaxValue(minValue < maxValue ? maxValue : minValue)
////   {
////      mSpan = (mMaxValue - mMinValue) + 1;
////   }
////
////   bool isDiscrete() { return mSpan == 1; } 
////
////   int selectRandom(PseudoRandom & randomNumberGenerator)
////   {
////      if (mSpan == 1)
////      {
////         return mMinValue; 
////      }
////      else
////      {
////         float rand = randomNumberGenerator.genZeroOne();
////         int offset = int( (float(mSpan) * rand) + 0.5 );
////         return mMinValue + offset;
////      }
////   }
////
////private:
////   int mMinValue;
////   int mMaxValue;
////   int mSpan;
////};
//
//class RepetitionConstraint // : public Node
//{
//public:
//   RepetitionConstraint() 
//   {}
//
//   ~RepetitionConstraint() 
//   {}
//
//   void addValue(int value)
//   {
//      mDiscreteValues.addValue(value);
//   }
//
//   void addRange(int val1, int val2)
//   {
//      mDiscreteValues.addRange(val1,val2);
//   }
//
//   int selectRandomValue(PseudoRandom & randomNumberGenerator)
//   {
//      int size = mDiscreteValues.size();
//      if (size == 0)
//      {
//         return 1; // default to one repetition 
//      }
//
//      int index = 0;
//      if (size > 1)
//      {
//         float rand = randomNumberGenerator.genZeroOne();
//         index = int( float(size) * rand );
//         if (index == size)
//         {
//            index = size - 1;
//         }
//      }
//      return mDiscreteValues.getAt(index);
//   }
//
//   void write(Writer & writer)
//   {
//      mDiscreteValues.write(writer);
//   }
//
//   int isRandomizable()
//   {
//      return mDiscreteValues.size() > 1;
//   }
//
//private:
//   DiscreteIntegers mDiscreteValues; 
//
//};
//


// An L-System production 
class LSystemProduction : public ListNode
{
public:
   LSystemProduction(const char * pName, int uniqueId = 0)
   {
      initialize(pName, uniqueId);
   }

   virtual ~LSystemProduction()
   {
      delete mpFormalArgumentList;
      delete mpSelectionExpression;
   }

   std::string getName() const
   {
      return mName;
   }

   float getProbability() const
   {
      return mProbability;
   }

   void setProbaility(float probability)
   {
      mProbability = clamp(probability, 0.0, 1.0);
      mHasProbability = true;
   }

   DoubleLinkList<LSystemTerm> & getTerms()
   {
      return mTerms;
   }

   void addTerm(LSystemTerm * pTerm)
   {
      if (pTerm != 0)
      {
         mTerms.append(pTerm);
      }
   }   

   LSystemTerm * getTermById(int id)
   {
      DoubleLinkListIterator<LSystemTerm> iter(mTerms);
      while (iter.hasMore())
      {
         LSystemTerm * pTerm = iter.getNext();
         if (pTerm->getUniqueId() == id)
         {
            return pTerm;
         }
      }
      return 0; // not found 
   }

   // Does NOT clear the counter before counting
   void countTerms(LSystemTermCounter * pCounter);

   bool isSelectRandom() const
   {
      return mHasProbability;
   }

   bool isSelectParameterized() const
   {
      return isParameterized();
   }

   void setFormalArgumentList(FormalArgumentList * pFormalArgumentList)
   {
      delete mpFormalArgumentList;
      mpFormalArgumentList = pFormalArgumentList;
   }

   FormalArgumentList * getFormalArgumentList() const
   {
      return mpFormalArgumentList;
   }

   bool hasFormalArguments() const
   {
      return (getNumFormalArguments() > 0);
   }

   bool isParameterized() const
   {
      return mpFormalArgumentList != 0;
   }

   int getNumFormalArguments() const
   {
      if (mpFormalArgumentList != 0)
      {
         return mpFormalArgumentList->size();
      }
      return 0;
   }

   std::string getFormalArgumentName(int idx) const
   {
      if (mpFormalArgumentList != 0)
      {
         return mpFormalArgumentList->getNameAt(idx);
      }
      return "<indx-out-of-range>";
   }

   void setSelectionExpression(Expression * pExpression)
   {
      delete mpSelectionExpression;
      mpSelectionExpression = pExpression;
   }

   Expression * getSelectionExpresssion() const
   {
      return mpSelectionExpression;
   }

   void write(PrintWriter & writer)
   {
      writer.print(mName);
      if (mProbability < 1.0)
      {
        writer.print("(%lf)", mProbability);
      }
     writeFormalArguments(writer);
     writeSelectionExpression(writer);
     writer.print(" = " );
     DoubleLinkListIterator<LSystemTerm> iter(mTerms);
     while (iter.hasMore())
     {
        LSystemTerm * pTerm = iter.getNext();
        pTerm->write(writer);
        writer.print(" ");
     }
   }

   bool serialize(ByteStreamWriter * pWriter) const;
   bool deserialize(ByteStreamReader * pReader);

private:
   std::string mName;           // the predecessor part 
   int        mUniqueId;

   float      mProbability; 
   bool       mHasProbability;

   bool       mHasCondition; // << replace with pointer check for arg list and consition 

   FormalArgumentList * mpFormalArgumentList;  // ptr to formal argument names if production is parameterized, optional 
   Expression         * mpSelectionExpression; // ptr to conditional expression used to select parameterized productions, optional 

   mutable DoubleLinkList<LSystemTerm> mTerms;   // the successor part 


   void writeFormalArguments(PrintWriter & writer)
   {
      if (mpFormalArgumentList != 0)
      {
         writer.print("(");
         int numArguments = mpFormalArgumentList->size();
         for (int i = 0; i < numArguments; i++)
         {
            if (i > 0)
            {
               writer.print(",");
            }
            std::string name = mpFormalArgumentList->getNameAt(i);
            writer.print(name);
         }
         writer.print(")");
     }
   }

   void writeSelectionExpression(PrintWriter & writer)
   {
      if (mpSelectionExpression != 0)
      {
         writer.print(":");
         mpSelectionExpression->print(writer);
     }
   }

   // No-arg constructor for deserialization
   friend class LSystemProductionGroup;
   LSystemProduction()
   {
      initialize(0,0);
   }

   void initialize(const char * pName, int uniqueId)
   {
      mName = pName;
      mUniqueId = uniqueId;
      mProbability = 1.0;
      mHasProbability = false;
      mHasCondition = false;
      mpFormalArgumentList = 0;
      mpSelectionExpression = 0;
   }

};

#endif // __tc_lsystem_production_h_
