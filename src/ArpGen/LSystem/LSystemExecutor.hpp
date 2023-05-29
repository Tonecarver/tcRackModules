#pragma once
#ifndef _tc_lsystem_executor_h_
#define _tc_lsystem_executor_h_ 1

// #include "../../plugin.hpp"
// #include "rack.hpp"

#include "LSystem.hpp"
#include "ExpandedTerm.hpp"
#include "ExpandedTermSequence.hpp"

#include "../../../lib/random/FastRandom.hpp"

typedef DoubleLinkListIteratorBiDirectional<ExpandedTerm> ExpandedTermIterator;

// // TODO: move to own file . OR . make templated bi-directional iterator 
// class ExpandedTermIterator // bi-directional iterator over ExpandedTerm instances in a List<>
// {
// public:
//    ExpandedTermIterator()
//       : mpList(0)
//       , mpNext(0)
//       , mLSystemExecuteDirection(LSystemExecuteDirection::DIRECTION_FORWARD)
//    {
//    }

//    virtual ~ExpandedTermIterator()
//    {
//    }


//    void set(DoubleLinkList<ExpandedTerm> * pList)
//    {
//       mpList = pList;
//       reset();
//    }

//    void set(DoubleLinkList<ExpandedTerm> & list)
//    {
//       mpList = &list;
//       reset();
//    }

//    void setDirectionForward()
//    {
//       mLSystemExecuteDirection = LSystemExecuteDirection::DIRECTION_FORWARD;
//    }

//    void setDirectionBackward()
//    {
//       mLSystemExecuteDirection = LSystemExecuteDirection::DIRECTION_REVERSE;
//    }

//    bool isDirectionForward() const
//    {
//       return (mLSystemExecuteDirection == LSystemExecuteDirection::DIRECTION_FORWARD);
//    }

//    bool isDirectionBackward() const
//    {
//       return (mLSystemExecuteDirection == LSystemExecuteDirection::DIRECTION_REVERSE);
//    }

//    bool hasMore() const
//    {
//       return mpNext != 0;
//    }

//    ExpandedTerm * getNext()
//    {
//       ExpandedTerm * pTerm = mpNext;
//       if (isDirectionForward())
//       {
//          mpNext = (ExpandedTerm*) pTerm->getNextNode();
//       }
//       else
//       {
//          mpNext = (ExpandedTerm*) pTerm->getPrevNode();
//       }
//       return pTerm;
//    }

//    ExpandedTerm * peekNext() const
//    {
//       return mpNext;
//    }

//    void reset()
//    {
//       mpNext = 0; 
//       if (mpList != 0)
//       {
//          if (isDirectionForward())
//          {
//             mpNext = mpList->peekFront();
//          }
//          else
//          {
//             mpNext = mpList->peekTail();
//          }
//       }
//    }

// private:
//    DoubleLinkList<ExpandedTerm> * mpList; // just BORROWING this pointer
//    ExpandedTerm       * mpNext; // just BORROWING this pointer
//    LSystemExecuteDirection mLSystemExecuteDirection;  // traversal direction algorithm 
// };




class LSystemExecutor
{
public:
   LSystemExecutor()
      //: mProbabilityVisitor(mPseudoRandom)
      : mpExpandedSequence(0)
      , mpActiveTerm(0)
      , mExecuteDirection(LSystemExecuteDirection::DIRECTION_FORWARD)
      , mCurrentDirection(LSystemExecuteDirection::DIRECTION_FORWARD)
      , mIsStepComplete(false)
      , mIsEndOfSequence(false)
   {
   }

   virtual ~LSystemExecutor()
   {
   }

   bool isPlayable() const {
      return mpExpandedSequence != 0 && (! mpExpandedSequence->isEmpty()); // TODO: and num playable terms >= 1
   }

   int getNumTerms() const { 
      if (mpExpandedSequence) {
         return mpExpandedSequence->getLength();
      }
      return 0;
   }
   
   void setExpandedTermSequence(ExpandedTermSequence & expandedSequence)
   {
      mpExpandedSequence = &expandedSequence;
      mpActiveTerm = 0;

      // reset the iterator to forward unless specifically configured to be reverse 
      if ((mExecuteDirection == LSystemExecuteDirection::DIRECTION_FORWARD) || 
          (mExecuteDirection == LSystemExecuteDirection::DIRECTION_FORWARD_REVERSE))
      {
         setScanDirectionForward();
      }

//      mProbabilityVisitor.set(expandedTree);
//      recomputeProbabilities();

      if (expandedSequence.isEmpty())
      {
         mExpandedTermIter.set(0);
      }
      else
      {
         mExpandedTermIter.set(expandedSequence.getTerms());
      }

      // inform derived subclass
      onNewSequence();
   }

   // Advance engine until next time-based action is performed
   void step()
   {
      advance(); 
   }

   // Subclasses MUST CALL THIS with a TRUE value to indicate that a step is complete 
   // advancer sets this to False when step begins and stops performing terms when step is complete becomes True  
   void setStepComplete(bool beComplete)
   {
      mIsStepComplete = beComplete;
   }

   bool isStepComplete() const
   {
      return mIsStepComplete;
   }

   // call to subclass to perform the action indicatd by the active term
   // pTerm may be NULL to indicate action could be determined
   virtual void perform(ExpandedTerm * pTerm) = 0;

   // call to subclass to indicate that end of pattern has been reached
   virtual void onEndOfPattern() = 0;

   // call to subclass to indicate that sequence has been replaced 
   virtual void onNewSequence() = 0; 

   // todo: setRandomScaling(float zeroToOne)

   void setExecuteDirection(LSystemExecuteDirection direction)
   {
      if (mExecuteDirection != direction)
      {
         mExecuteDirection = direction;

         if (direction == LSystemExecuteDirection::DIRECTION_FORWARD)
         {
            setScanDirectionForward();
         }
         else if (direction == LSystemExecuteDirection::DIRECTION_REVERSE)
         {
            setScanDirectionReverse();
         }
         // else is FWD_REV, leave iterator and current direction as is
      }
   }

   bool isScanningForward() const
   {
      return (mCurrentDirection == LSystemExecuteDirection::DIRECTION_FORWARD);
   }

   bool isScanningReverse() const
   {
      return (mCurrentDirection == LSystemExecuteDirection::DIRECTION_REVERSE);
   }

   void resetScanToBeginning()
   {
      mExpandedTermIter.reset();
   }

   bool isEndOfSequence() const {
      return mIsEndOfSequence; 
   }

   float getRandomZeroToOne() {
      return mPseudoRandom.generateZeroToOne();
   }
   
private:
   ExpandedTermSequence * mpExpandedSequence; // just BORROWING this pointer 
   ExpandedTermIterator   mExpandedTermIter; 
   ExpandedTerm         * mpActiveTerm;

   LSystemExecuteDirection mExecuteDirection;  // selected direction algorithm 
   LSystemExecuteDirection mCurrentDirection;  // current direction of traversal 

   bool                    mIsStepComplete;  // true when advancer should stop processing terms in current step
   bool                    mIsEndOfSequence; // true when most recently executed term is end-of-sequencce

   FastRandom       mPseudoRandom; 
   //ProbabilityVisitor mProbabilityVisitor;


   void advance()
   {
      //// Continue current node until repeat count is reached 
      //if (mpActiveTerm != 0)
      //{
      //   mpActiveTerm->decrementRepeatCount();
      //   if (mpActiveTerm->getRepeatsRemaining() > 0)
      //   {
      //      perform(mpActiveTerm);
      //      return;
      //   }
      //}

      setStepComplete(false);
      mIsEndOfSequence = false; 

      if ((mpExpandedSequence == 0) || (mpExpandedSequence->isEmpty()))
      {
         DEBUG(( "Abandoning execution .. no terms defined" ));

         mpActiveTerm = 0;
         perform(0);
         return; // empty 
      }

      int numPasses = 0;
      for (;;)
      {
         while (mExpandedTermIter.hasMore())
         {
            ExpandedTerm * pTerm = mExpandedTermIter.getNext();
            assert(pTerm != 0);
            //assert(pTerm->isProbable());

            if (pTerm->isExecutable())
            {
               //DEBUG(( "execute term: %s", pTerm->getThumbprint() ));
               mpActiveTerm = pTerm;
               //mpActiveTerm->decrementRepeatCount();
               perform(pTerm); 

               if (isStepComplete())
               {
                  return;
               }
            }
#ifdef INCLUDE_DEBUG_FUNCTIONS
//            else
//            {
//               DEBUG(( "executor: term: %s is NOT executable", pTerm->getThumbprint() ));
//            }
#endif 
         }

         // have reached end of pattern
         if (numPasses > 1)
         {
            // DEBUG(( "Abandoning execution .. no executable terms found" ));
            return;
         }
         
         numPasses++; 
         
         mIsEndOfSequence = true; 
         onEndOfPattern();
         //recomputeProbabilities();
         mExpandedTermIter.reset();

         if (mExecuteDirection == LSystemExecuteDirection::DIRECTION_FORWARD_REVERSE)
         {
            DEBUG(( "End of pattern : toggle direction" ));
            if (isScanningForward())
            {
               setScanDirectionReverse();
            }
            else // must be in reverse, toggle to forward 
            {
               setScanDirectionForward();
            }

            // Reset iterator to point to the correct end of the list 
            mExpandedTermIter.reset();

            // If the direction changed then the next term is the last (or first)
            // term in the list, which could be the same term as the one that 
            // was executed just prior to switching difrection.
            // 
            // If the algorithm indicates to repeat the term at the end of the 
            // list when switching directions, then do nothing.
            //
            // If the algorithm indicates to perform the end terms only once 
            // when switching direction, then check to see if the next term 
            // matches the recently executed term. If so, advance the iterator 
            // to skip it. 

            ExpandedTerm * pPeekTerm = mExpandedTermIter.peekNext();
            if (pPeekTerm == mpActiveTerm)
            {
               mExpandedTermIter.getNext();
            }
         }

      }

   }

   void setScanDirectionForward()
   {
      //DEBUG(( "Set Scan direction FORWARD" ));
      mExpandedTermIter.setDirectionForward();
      flushAndRepointTermIterator();
      mCurrentDirection = LSystemExecuteDirection::DIRECTION_FORWARD;
   }

   void setScanDirectionReverse()
   {
      //DEBUG(( "Set Scan direction REVERSE" ));
      mExpandedTermIter.setDirectionReverse();
      flushAndRepointTermIterator();
      mCurrentDirection = LSystemExecuteDirection::DIRECTION_REVERSE;
   }

   void flushAndRepointTermIterator()
   {
      // After changing iterator direction,
      // if in the middle of the list, advance the iterator
      // (after setting the direction) to flush and repoint 
      // to the 'next' member in the new direction.
      ExpandedTerm * pTerm = mExpandedTermIter.peekNext();
      if (pTerm != 0)
      {
         if (pTerm->isMiddleNode())
         {
            mExpandedTermIter.getNext();
         }
      }

      pTerm = mExpandedTermIter.peekNext();
      if (pTerm != 0)
      {
         if (pTerm->isMiddleNode())
         {
            mExpandedTermIter.getNext();
         }
      }

   }

   //void recomputeProbabilities()
   //{
   //   mProbabilityVisitor.resetToBeginning();
   //   mProbabilityVisitor.visit(); 

   //   mExpandedTermIter.set(mProbabilityVisitor.getLinearLeafTerms());
   //}


};

#endif // __tc_lsystem_executor_h_
