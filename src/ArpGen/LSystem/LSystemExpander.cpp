#include "LSystemProduction.hpp"
#include "LSystemProductionGroup.hpp"
#include <time.h>

#include "../../plugin.hpp"  // for DEBUG 

#include "LSystemExpander.hpp"

//#include "debugtrace.h"
#include <assert.h>

ExpandedTermSequence * LSystemExpander::expand(const char * startRuleName, int depth)
{
   time_t now;
   time(&now);
   if (now != 0)
   {
      mRandomNumberGenerator.setSeed( int(now) );
   }

   LSystemProductionGroup * pStartGroup = mLSystem.getProductionGroup(startRuleName);
   if (pStartGroup == 0)
   {
      DEBUG( "Expander: cannot find start rule: '%s'", startRuleName == 0 ? "<null>" : startRuleName );
      return 0;
   }

   LSystemProduction * pProduction = pStartGroup->getProductions().peekFront();
   if (pProduction == 0)
   {
      DEBUG( "Expander: start group is empty: '%s'", startRuleName == 0 ? "<null>" : startRuleName );
      return 0;
   }

   mpConstructed = new DoubleLinkList<ExpandedTerm>(); 
   mpUnderConstruction = new DoubleLinkList<ExpandedTerm>(); 

   appendTerms(pProduction);
   DoubleLinkList<ExpandedTerm> * pTemp = mpConstructed;
   mpConstructed = mpUnderConstruction; 
   mpUnderConstruction = pTemp;

 // DEBUG( "Expander: root, length = %d", mpConstructed->size() );

   for (int i = 1; i < depth; i++)
   {
      if (mpConstructed->size() >= MAX_EXPANDED_LENGTH)
      {
         DEBUG( "Expander: depth %d, length = %d .. max length reached, breaking early", i, mpConstructed->size() );
        break;
      }

      while (mpConstructed->isEmpty() == false)
      {
         ExpandedTerm * pTerm = mpConstructed->popFront(); 
         expandTerm(pTerm); 
      }

      DoubleLinkList<ExpandedTerm> * pTemp = mpConstructed;
      mpConstructed = mpUnderConstruction; 
      mpUnderConstruction = pTemp;

    // DEBUG( "Expander: depth %d, length = %d", i, mpConstructed->size() );
   }

   // todo: make ExpandedTermSequence a SUBCLASS of List<ExpandedTerm>
   //   or have ot hold/own a ptr to a list
   ExpandedTermSequence * pExpandedSequence = new ExpandedTermSequence();
   pExpandedSequence->setTerms(mpConstructed);
   mpConstructed = 0;

   delete mpUnderConstruction;

   // DEBUG("Expanding complete: depth = %d", depth);

   return pExpandedSequence;
}

void LSystemExpander::appendTerms(LSystemProduction * pProduction)
{
   assert(pProduction != 0);

   DoubleLinkListIterator<LSystemTerm> termIter(pProduction->getTerms());
   while (termIter.hasMore())
   {
      LSystemTerm * pTerm = termIter.getNext();

      ExpandedTerm * pExpandedTerm = mMemoryRecycler.allocateExpandedTerm(pTerm);

      if (pTerm->getNumArguments() > 0)
      {
         // resolve expressions to actual 
         resolveArgumentExpressions(pExpandedTerm);
      }

      mpUnderConstruction->append(pExpandedTerm);
   }

}

void LSystemExpander::expandTerm(ExpandedTerm * pExpandedTerm)
{
   if (pExpandedTerm->isRewritable())
   {
      // set (optional) local param values 
      ArgumentValuesList * pActualParameterList = pExpandedTerm->getActualParameterValues();
      mExpressionContext.setLocalParameterValues(pActualParameterList); 

      LSystemProduction * pProduction = selectProduction(pExpandedTerm);
      if (pProduction != 0)
      {
         appendTerms(pProduction);
         mMemoryRecycler.retireExpandedTerm(pExpandedTerm); // retire AFTER appending so that borrowed local parameter array remains intact 
      }
      else
      {
         mpUnderConstruction->append(pExpandedTerm);
      }

      mExpressionContext.setLocalParameterValues(0); // remove local param values 
   }
   else
   {
      mpUnderConstruction->append(pExpandedTerm);
   }
}

LSystemProduction * LSystemExpander::selectProduction(ExpandedTerm * pExpandedTerm)
{
   std::string name = pExpandedTerm->getName();

   LSystemProductionGroup * pGroup = mLSystem.getProductionGroup(name);
   if (pGroup == 0)
   {
      return 0;
   }

   LSystemProduction * pProduction = 0;

   if (pGroup->isEmpty())
   {
      pProduction = 0; // just in case 
   }
   //else if (select by random index)
   //{
   //   double rand = mRandomNumberGenerator.genZeroOne();
   //   double maxIndex = pGroup->getNumProductions() - 1;
   //   int index = int( (rand * maxIndex) + 0.5 ); 
   //   DEBUG((" Production %s, RANDOM rand=%lf, numProd=%d, selected %d", pGroup->getName().get(), rand, pGroup->getNumProductions(),index ));
   //   assert((0 <= index) && (index < pGroup->getNumProductions()));
   //   pProduction = pGroup->getAt(index);
   //}

   else if (pGroup->isSelectRandom())
   {
      float rand = mRandomNumberGenerator.generateZeroToOne();
      pProduction = pGroup->selectRandom(rand); 
      // todo: history.record(pProduction);
   }
   else if (pGroup->isSelectParameterized())
   {
      pProduction = selectProductionParameterized(pGroup, pExpandedTerm);
   }
   else
   {
      pProduction = pGroup->getAt(0);
   }

   return pProduction;
}

LSystemProduction * LSystemExpander::selectProductionParameterized(LSystemProductionGroup * pGroup, ExpandedTerm * pExpandedTerm)
{
   DoubleLinkListIterator<LSystemProduction> iter(pGroup->getProductions());
   while (iter.hasMore())
   {
      LSystemProduction * pProduction = iter.getNext();
      Expression * pExpression = pProduction->getSelectionExpresssion();
      if (pExpression == 0)
      {
         // no condition -- assume always true (match everything)
         return pProduction;
      }

      if (pExpression->isTrue( & mExpressionContext ))
      {
         return pProduction; // expression condition satisfied 
      }
   }
   return 0; // no match 
}

void LSystemExpander::resolveArgumentExpressions(ExpandedTerm * pExpandedTerm)
{
   // for each argument expression
   //  resolve to constant value 
   //  store as arg value 

   ArgumentExpressionList * pArgumentExpressionList = pExpandedTerm->getArgumentList();  // expressions 
   if (pArgumentExpressionList == 0)
   {
      return;
   }

   if (pArgumentExpressionList->size() == 0)
   {
      return;
   }

   ArgumentValuesList * pArgumentValuesList = mMemoryRecycler.allocateArgumentValuesList();

   DoubleLinkListIterator<Expression> iter(pArgumentExpressionList->getArgumentExpressions());
   while (iter.hasMore())
   {
      Expression * pExpression = iter.getNext();

      float value = pExpression->getValue( & mExpressionContext );
      pArgumentValuesList->addParameterValue(value);
   }

   pExpandedTerm->setActualParameterValues(pArgumentValuesList);
}
