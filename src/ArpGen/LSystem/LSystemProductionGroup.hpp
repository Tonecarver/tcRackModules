#pragma once
#ifndef _tc_lsystem_production_group_h_
#define _tc_lsystem_production_group_h_ 1

#include "../../plugin.hpp"

#include "LSystemProduction.hpp"

#include "../../../lib/datastruct/List.hpp"
#include "../../../lib/io/PrintWriter.hpp"
#include "../../../lib/io/ByteStream.hpp"

// #include "../Expression/Expression.hpp"
// #include "../Expression/ExpressionContext.hpp"
// #include "../Expression/ArgumentList.hpp"

// #include "LSystemTermCounter.hpp"

// #include "../../../lib/FreePool.hpp"
// #include "../../../lib/FastRandom.hpp"

// #include "../../../lib/io/PrintWriter.hpp"
// #include "../../../lib/io/ByteStream.hpp"

// //#include "ResizableBuffer.h"
// //#include "FastMath.h"
// // #include "debugtrace.h"

#include <string>
#include <cstdio>




// An L-System production group
// contains one or more alternatives, each with probability
class LSystemProductionGroup : public ListNode
{
public:
   LSystemProductionGroup(std::string name)
      : mName(name)
   {
   }

   virtual ~LSystemProductionGroup()
   {
   }

   std::string getName() const
   {
      return mName;
   }

   void addProduction(LSystemProduction * pProduction)
   {
      // check already contains ? 
      // assert prod.name == name 
      assert(pProduction != 0);

      mAlternateProductions.append(pProduction);
   }

   DoubleLinkList<LSystemProduction> & getProductions()
   {
      return mAlternateProductions;
   }
   
   bool isEmpty() const
   {
      return getNumProductions() == 0; 
   }

   int getNumProductions() const
   {
      return mAlternateProductions.size();
   }

   bool isSelectRandom() const
   {
      return isEmpty() || mAlternateProductions.peekFront()->isSelectRandom();
   }

   bool isSelectParameterized() const
   {
      return isEmpty() || mAlternateProductions.peekFront()->isSelectParameterized();
   }

   int getNumFormalParameters() const
   {
      if (isEmpty())
      {
         return 0;
      }

      return mAlternateProductions.peekFront()->getNumFormalArguments();
   }

   LSystemProduction * getAt(int idx) const
   {
      return mAlternateProductions.getAt(idx);
   }
   
   LSystemProduction * selectRandom(float probabilityThreshold)
   {
      float accumulatedProbability = 0.0;
      DoubleLinkListIterator<LSystemProduction> iter(mAlternateProductions);
      while (iter.hasMore())
      {
         LSystemProduction * pProduction = iter.getNext();
         accumulatedProbability += pProduction->getProbability();
         if (accumulatedProbability >= probabilityThreshold)
         {
            return pProduction;
         }
      }
      return 0;
   }

   LSystemTerm * getTermById(int id)
   {
      DoubleLinkListIterator<LSystemProduction> iter(mAlternateProductions);
      while (iter.hasMore())
      {
         LSystemProduction * pProduction = iter.getNext();
         LSystemTerm * pTerm = pProduction->getTermById(id);
         if (pTerm != 0)
         {
            return pTerm;
         }
      }
      return 0; // not found 
   }

   void write(PrintWriter & writer)
   {
      DoubleLinkListIterator<LSystemProduction> iter(mAlternateProductions);
      while (iter.hasMore())
      {
         LSystemProduction * pProduction = iter.getNext();
         pProduction->write(writer);
         writer.println();
      }
   }

   bool serialize(ByteStreamWriter * pWriter);
   bool deserialize(ByteStreamReader * pReader);

private:
   std::string mName; 
   DoubleLinkList<LSystemProduction> mAlternateProductions;

   // No-arg constructor for deserialization
   friend class LSystem;
   LSystemProductionGroup()
   {
   }

};

#endif // __tc_lsystem_production_group_h_
