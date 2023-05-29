#pragma once
#ifndef _tc_lsystem_expander_h_
#define _tc_lsystem_expander_h_ 1

#include "../../plugin.hpp"

// #include "../Expression/Expression.hpp"
// #include "../Expression/ExpressionContext.hpp"
// #include "../Expression/ArgumentList.hpp"

#include "LSystem.hpp"
#include "ExpandedTerm.hpp"
#include "ExpandedTermSequence.hpp"

// #include "LSystemTermCounter.hpp"

// #include "../../../lib/List.hpp"
// #include "../../../lib/FreePool.hpp"
// #include "../../../lib/FastRandom.hpp"

// #include "../../../lib/io/PrintWriter.hpp"
// #include "../../../lib/io/ByteStream.hpp"

// //#include "ResizableBuffer.h"
// //#include "FastMath.h"
// // #include "debugtrace.h"

// #include <string>
// #include <cstdio>


class LSystemMemoryRecycler
{
public:
   LSystemMemoryRecycler(int freeTermThreshold = 32, int freeArglistThreshold = 32, int maxDeletesPerSiphon = 1)
      : mExpandedTermThreshold(freeTermThreshold)
      , mArgumentValueThreshold(freeArglistThreshold)
      , mMaxDeletesPerSiphon(maxDeletesPerSiphon) 
   {
   }

   virtual ~LSystemMemoryRecycler()
   {
   }

   ExpandedTerm * allocateExpandedTerm(LSystemTerm * pTerm)
   {
      ExpandedTerm * pExpandedTerm = mExpandedTermFreePool.allocate();
      pExpandedTerm->initialize();
      pExpandedTerm->setTerm(pTerm);
      return pExpandedTerm;
   }

   void retireExpandedTerm(ExpandedTerm * pExpandedTerm)
   {
      if (pExpandedTerm != 0)
      {
         ArgumentValuesList * pArgumentValuesList = pExpandedTerm->getActualParameterValues();
         if (pArgumentValuesList != 0)
         {
            mArgumentValueFreePool.retire(pArgumentValuesList);
         }
         pExpandedTerm->setActualParameterValues(0);
         mExpandedTermFreePool.retire(pExpandedTerm);
      }
   }

   ArgumentValuesList * allocateArgumentValuesList()
   {
      ArgumentValuesList * pArgumentValuesList = mArgumentValueFreePool.allocate();
      pArgumentValuesList->clear();
      return pArgumentValuesList;
   }

   void retireArgumentValuesList(ArgumentValuesList * pArgumentValuesList)
   {
      if (pArgumentValuesList != 0)
      {
         mArgumentValueFreePool.retire(pArgumentValuesList);
      }
   }

   void siphonExcessFreeInstances()
   {
      //mExpandedTermFreePool.reduceToThreshold(mExpandedTermThreshold, mMaxDeletesPerSiphon);
      //mArgumentValueFreePool.reduceToThreshold(mArgumentValueThreshold, mMaxDeletesPerSiphon);
   }

private:
   FreePool<ExpandedTerm> mExpandedTermFreePool;
   int mExpandedTermThreshold; 

   FreePool<ArgumentValuesList> mArgumentValueFreePool;
   int mArgumentValueThreshold; 

   int mMaxDeletesPerSiphon; 
};



class LSystemExpander
{
public:
   LSystemExpander(const LSystem & lSystem, LSystemMemoryRecycler & memoryRecycler, ExpressionContext & expressionContext)
      : mLSystem(lSystem)
      , mMemoryRecycler(memoryRecycler)
      , mExpressionContext(expressionContext)
   {
   }

   virtual ~LSystemExpander()
   {
   }

   enum { 
       MAX_EXPANDED_LENGTH = 7500  // stop expanding after this number of terms 
   };

   ExpandedTermSequence * expand(const char * startRuleName, int depth);

private:
   const LSystem         & mLSystem;
   LSystemMemoryRecycler & mMemoryRecycler; // free pools and allocate/retire methods 
   ExpressionContext     & mExpressionContext; // context for evaluating expressions 

   FastRandom         mRandomNumberGenerator; // todo: pass this in as optional 

   DoubleLinkList<ExpandedTerm>  * mpConstructed;
   DoubleLinkList<ExpandedTerm>  * mpUnderConstruction;
   
   void appendTerms(LSystemProduction * pProduction);
   void expandTerm(ExpandedTerm * pExpandedTerm);
   LSystemProduction * selectProduction(ExpandedTerm * pExpandedTerm);
   LSystemProduction * selectProductionParameterized(LSystemProductionGroup * pGroup, ExpandedTerm * pExpandedTerm);
   void resolveArgumentExpressions(ExpandedTerm * pExpandedTerm);

};


#endif // __tc_lsystem_expander_h_
