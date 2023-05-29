#pragma once
#ifndef _tc_expanded_term_h_
#define _tc_expanded_term_h_ 1

#include "../../plugin.hpp"

#include "LSystem.hpp"
#include "LSystemTerm.hpp"
#include "LSystemTermCounter.hpp"

#include "../../../lib/datastruct/List.hpp"

#include <string>

class ExpandedTerm : public ListNode
{
public:
   ExpandedTerm()
      : mpTerm(0)
      , mpParameterValues(0)
   {
   }

   virtual ~ExpandedTerm()
   {
      delete mpParameterValues;
   }

   // reset to initial state after removing from free pool 
   void initialize()
   {
      if (mpParameterValues != 0)
      {
         mpParameterValues->clear();
      }
      mpTerm = 0; 
   }

   void setTerm(LSystemTerm * pTerm)
   {
      assert(pTerm != 0);
      mpTerm = pTerm;
   }

   std::string getName() const
   {
      assert(mpTerm != 0);
      return mpTerm->getName();
   }

   int getTermType() const
   {
      return mpTerm->getTermType();
   }

   bool isRewritable()  const
   {
      return mpTerm->isRewritable();
   }

   bool isExecutable()  const
   {
      return mpTerm->isExecutable();
   }

   bool isStepTerminator() const
   {
      return mpTerm->isStepTerminator();
   }

   bool hasCondition() const
   {
      return mpTerm->hasCondition();
   }

   Expression * getCondition() const
   {
      return mpTerm->getCondition();
   }

   bool hasArgumentList() const
   {
      return mpTerm->hasArgumentExpressionList();
   }

   ArgumentExpressionList * getArgumentList() const
   {
      return mpTerm->getArgumentExpressionList();
   }

   int getNumActualArguments() const
   {
      if (mpParameterValues != 0)
      {
         return mpParameterValues->size();
      }
      return 0;
   }

   ArgumentValuesList * getActualParameterValues() const
   {
      return mpParameterValues;
   }

   void setActualParameterValues(ArgumentValuesList * pArgumentValuesList)
   {
      mpParameterValues = pArgumentValuesList;
   }

   //void addActualParameterValue(float value)
   //{
   //   if (mpParameterValues == 0)
   //   {
   //      mpParameterValues = new ArgumentValuesList(); // todo: change this to just get/set, allocate/retire in expander 
   //   }
   //  
   //   if (mpParameterValues->isFull() == false)
   //   {
   //      mpParameterValues->addParameterValue(value);
   //   }
   //}

   //void setActualParameterValue(int idx, float value)
   //{
   //   // todo: check idx >= 0

   //   if (mpParameterValues == 0)
   //   {
   //      mpParameterValues = new ArgumentValuesList();
   //   }
   //   mpParameterValues->setValueAt(idx,value);
   //}

   //int getRepeatsRemaining()
   //{
   //   return mRepetitionsRemaining;
   //}

   //void decrementRepeatCount()
   //{
   //   mRepetitionsRemaining--;
   //}

   //bool isLeaf() 
   //{
   //   return mAlternateExpansions.getNumMembers() == 0;
   //}

   //bool isExpanded() 
   //{
   //   return mAlternateExpansions.getNumMembers() > 0;
   //}

   //float getProbability()
   //{
   //   return mpTerm->getProbability();
   //}

   //bool isRandomizable()
   //{
   //   return mIsRandomizable; 
   //}

   //bool isProbable()
   //{
   //   return mIsProbable; // has met probability ctriterea
   //}

   //List<ExpandedTermSequence> & getAlternateExpansions()
   //{
   //   return mAlternateExpansions;
   //}

   //bool isActiveSequence(ExpandedTermSequence * pSequence)
   //{
   //   assert(pSequence != 0);
   //   return (mpActiveExpansion == pSequence);
   //}

   //void applyProbability(PseudoRandom & randomNumberGenerator)
   //{
   //   //mRepetitionsRemaining = 1;

   //   //if (mIsRandomizable == false)
   //   //{
   //   //   return;
   //   //}

   //   //float rand = randomNumberGenerator.genZeroOne();
   //   //if (isExpanded())
   //   //{
   //   //   mIsProbable = true;

   //   //   //float accumProb = 0;

   //   //   //mpActiveExpansion = 0;

   //   //   //DoubleLinkListIterator<ExpandedTermSequence> iter(mAlternateExpansions);
   //   //   //while (iter.hasMore())
   //   //   //{
   //   //   //   ExpandedTermSequence * pSequence = iter.getnext();
   //   //   //   pSequence->setProbable(false);
   //   //   //   accumProb += pSequence->getProbability();
   //   //   //   if (accumProb >= rand)
   //   //   //   {
   //   //   //      if (mpActiveExpansion == 0)
   //   //   //      {
   //   //   //         mpActiveExpansion = pSequence;
   //   //   //         pSequence->setProbable(true);
   //   //   //      }
   //   //   //   }
   //   //   //}
   //   //}
   //   //else // if (isLeaf()) || isExpanded() == false 
   //   //{
   //   //   //if (mpTerm->getProbability() >= rand)
   //   //   //{
   //   //   //   mIsProbable = true;
   //   //   //}
   //   //   //else
   //   //   //{
   //   //   //   mIsProbable = false;
   //   //   //}

   //   //   //mRepetitionsRemaining = mpTerm->getRepetitionConstraint().selectRandomValue(randomNumberGenerator);
   //   //   //if (mRepetitionsRemaining == 0)
   //   //   //{
   //   //   //   mIsProbable = false;
   //   //   //}

   //   //   mIsProbable = true;
   //   //}
   //}

   bool serialize(ByteStreamWriter * pWriter) const;
   bool deserialize(ByteStreamReader * pReader, LSystem * pLSystem);

#ifdef INCLUDE_DEBUG_FUNCTIONS
   std::string getThumbprint() const
   {
      return mpTerm->getThumbprint();
   }
#endif 

private:
   LSystemTerm       * mpTerm;  // the term declaration that this is expanded from


   // If the Term (that this expanded term is expanded from) has formal arguments,
   // then the argument expressions held by the term are evaluated into actual values
   // and stored in the mParameterValues array.
   ArgumentValuesList * mpParameterValues;   // optional 

   //int    mRepetitionsRemaining;  // runtime: number of repetitions remaining 
};


#endif // _tc_expanded_term_h_
