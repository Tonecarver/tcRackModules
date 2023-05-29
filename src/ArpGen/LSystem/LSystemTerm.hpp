#pragma once
#ifndef _tc_lsystem_term_h_
#define _tc_lsystem_term_h_ 1

#include "../../plugin.hpp"

#include "../Expression/Expression.hpp"
#include "../Expression/ArgumentList.hpp"

// #include "../Expression/ExpressionContext.hpp"
// #include "LSystemTermCounter.hpp"

#include "../../../lib/datastruct/List.hpp"

// #include "../../../lib/FreePool.hpp"
// #include "../../../lib/FastRandom.hpp"
// #include "../../../lib/io/PrintWriter.hpp"
// #include "../../../lib/io/ByteStream.hpp"
// //#include "ResizableBuffer.h"
// //#include "FastMath.h"
// // #include "debugtrace.h"

#include <string>
#include <cstdio>


// An L-System production term
class LSystemTerm : public ListNode
{
public:
   LSystemTerm(const char * pName, int termType, int uniqueId)
   {
      initialize(pName, termType, uniqueId);
#ifdef INCLUDE_DEBUG_FUNCTIONS
      buildThumbprint();
#endif 
   }

   virtual ~LSystemTerm()
   {
      delete mpExpression;
      delete mpArgumentExpressionList;
   }

   std::string getName() const
   {
      return mName;
   }

   int getUniqueId() 
   {
      return mUniqueId;
   }

#ifdef INCLUDE_DEBUG_FUNCTIONS
   std::string getThumbprint() const
   {
      return mThumbprint;
   }
#endif 

   int getTermType() const
   {
      return mTermType;
   }

   Expression * getCondition() const
   {
      return mpExpression;
   }

   void setCondition(Expression * pExpression)
   {
      delete mpExpression;
      mpExpression = pExpression;
   }

   //float getProbability()
   //{
   //   return mProbability;
   //}

   //void setProbaility(float probability)
   //{
   //   mProbability = FastMath::clamp(probability, 0.0, 1.0);
   //}

   //RepetitionConstraint & getRepetitionConstraint()
   //{
   //   return mRepetitionConstraint;
   //}

   //void addRepetitionValue(int count)
   //{
   //   mRepetitionConstraint.addValue(count);
   //}

   //void addRepetitionRange(int val1, int val2)
   //{
   //   mRepetitionConstraint.addRange(val1, val2);
   //}

   bool isRewritable() const
   {
      return mIsRewritable;
   }

   void setRewritable(bool beRewritable)
   {
      mIsRewritable = beRewritable;
   }

   bool isExecutable() const
   {
      return mIsExecutable;
   }

   void setExecutable(bool beExecutable)
   {
      mIsExecutable = beExecutable;
   }

   bool isStepTerminator() const
   {
      return mIsStepTerminator;
   }

   void setStepTerminator(bool beStepTerminator)
   {
      mIsStepTerminator = beStepTerminator;
   }

   bool hasCondition() const
   {
      return mpExpression != 0;
   }

   void setArgumentExpressionList(ArgumentExpressionList * pArgumentExpressionList)
   {
      mpArgumentExpressionList = pArgumentExpressionList;
   }

   bool hasArgumentExpressionList() const
   {
      return mpArgumentExpressionList != 0;
   }

   ArgumentExpressionList * getArgumentExpressionList() const
   {
      return mpArgumentExpressionList;
   }

   int getNumArguments() const
   {
      if (mpArgumentExpressionList != 0)
      {
         return mpArgumentExpressionList->size();
      }
      return 0;
   }

   void write(PrintWriter & writer)
   {
      //if (mpExpression == 0)
      //{
      //   writer.print(mName.get());
      //}
      //else
      //{
      //   writer.print("{ ");
      //   mpExpression->print(writer);
      //   writer.print(" }");
      //}

      writer.print(mName);
      if (mpExpression != 0)
      {
         mpExpression->print(writer);
      }

      if (mpArgumentExpressionList != 0)
      {
         writer.print("(");

         DoubleLinkListIterator<Expression> iter(mpArgumentExpressionList->getArgumentExpressions());
         while (iter.hasMore())
         {
            Expression * pExpression = iter.getNext();
            pExpression->print(writer);
            if (iter.hasMore())
            {
               writer.print(",");
            }
         }
         writer.print(")");
      }

      // todo: show expression 
      // todo: show argument list 

      //if (mProbability < 1.0)
      //{
      //   writer.write('(');
      //   writer.write(mProbability);
      //   writer.write(')');
      //}
      //mRepetitionConstraint.write(writer); // << causes '..' ranges to be written as x,y,z, discretes 
   }

   bool serialize(ByteStreamWriter * pWriter);
   bool deserialize(ByteStreamReader * pReader);

private:
   std::string mName;
   int        mUniqueId;
#ifdef INCLUDE_DEBUG_FUNCTIONS
   std::string mThumbprint;
#endif 
   int        mTermType;
   bool       mIsRewritable;
   bool       mIsExecutable;
   bool       mIsStepTerminator;
   Expression * mpExpression;     // runtime evaluated condition, optional 

   ArgumentExpressionList * mpArgumentExpressionList; // runtime evaluated argument expressions, optional. 
                                  // argument are resolved to values and "passed" to parameterized productions when expanding

//   float     mProbability; 
//   RepetitionConstraint mRepetitionConstraint;


#ifdef INCLUDE_DEBUG_FUNCTIONS
   void buildThumbprint()
   {
      char buf[48];
      _snprintf(buf, sizeof(buf), "%s_%d", mName.c_str(), mUniqueId);
      buf[ sizeof(buf) - 1] = 0;
      mThumbprint = buf;
   }
#endif 

   // no-ag constructor is reserved for use by the Production when deserializing
   friend class LSystemProduction; 

   LSystemTerm()
   {
      initialize(0,0,0);
   }

   void initialize(const char * pName, int termType, int uniqueId)
   {
      mName = pName;
      mTermType = termType;
      mUniqueId = uniqueId;
      mIsRewritable = false;
      mIsExecutable = true;
      mIsStepTerminator = false;
      mpExpression = 0;
      mpArgumentExpressionList = 0;
   }

};

#endif // __tc_lsystem_term_h_
