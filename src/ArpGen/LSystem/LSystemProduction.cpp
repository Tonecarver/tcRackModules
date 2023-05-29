#include "LSystemProduction.hpp"
#include "LSystemProductionGroup.hpp"
#include <time.h>

#include "../../plugin.hpp"

#include "LSystem.hpp"
#include "LSystemTerm.hpp"
#include "LSystemExpander.hpp"

//#include "debugtrace.h"
#include <assert.h>

const char LSystem::DEPTH_KEYWORD []   = { "DEPTH" };
const char LSystem::BOUNDS_KEYWORD []  = { "BOUNDS" };
const char LSystem::FLOOR_KEYWORD []   = { "FLOOR" };
const char LSystem::CEILING_KEYWORD [] = { "CEILING" };
const char LSystem::INFO_KEYWORD []    = { "INFO" };

const char LSystem::CYCLE_KEYWORD []   = { "CYCLE" };
const char LSystem::REFLECT_KEYWORD [] = { "REFLECT" };


// const char * lsystemBoundsTypeNames[LSystemBoundsType::kNum_BOUNDS_TYPE] = 
// {
//    "Up",       // BOUNDS_CYCLE_UP,         // reset to first note, set direction upward
//    "Down",     // BOUNDS_CYCLE_DOWN,       // reset to last note, set direction downward
//    "Reflect",  // BOUNDS_REFLECT, // reverse direction
//    "Linger",   // BOUNDS_REFLECT_LINGER, // reverse direction
//    "Flip",     // BOUNDS_FLIP,             // reset note to nominal index (0), and reverse direction
//    // "Stay",     // BOUNDS_STAY_AT_EDGE,     // hug the edge until a command arrives to go the other direction
//    "Nominal",  // BOUNDS_RESET_TO_NOMINAL, // reset to nominal value
//    "Discard",  // BOUNDS_DISCARD,          // suppress the out of bounds values 
// };



//////////////////////////////////////////////////////////////////////
//   Term  

bool LSystemTerm::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);

   pWriter->writeString(mName);
   pWriter->writeInt(mUniqueId);
#ifdef INCLUDE_DEBUG_FUNCTIONS
   //SafeString mThumbprint;
#endif 
   pWriter->writeInt(mTermType);
   pWriter->writeBoolean(mIsRewritable);
   pWriter->writeBoolean(mIsExecutable);
   pWriter->writeBoolean(mIsStepTerminator);

   pWriter->writeBoolean(mpExpression != 0);
   if (mpExpression != 0)
   {
      mpExpression->serialize(pWriter);
   }

   pWriter->writeBoolean(mpArgumentExpressionList != 0);
   if (mpArgumentExpressionList != 0)
   {
      mpArgumentExpressionList->serialize(pWriter);
   }

   return true;
}


bool LSystemTerm::deserialize(ByteStreamReader * pReader)
{
   assert(pReader != 0);

   // this REQUIRES a default no-arg ctor so the object can be instantiated 
   // before bing deserialized 

   pReader->readString(mName);
   pReader->readInt(mUniqueId);
#ifdef INCLUDE_DEBUG_FUNCTIONS
   buildThumbprint();
#endif 
   pReader->readInt(mTermType);
   pReader->readBoolean(mIsRewritable);
   pReader->readBoolean(mIsExecutable);
   pReader->readBoolean(mIsStepTerminator);

   bool isPresent;
   pReader->readBoolean(isPresent);
   if (isPresent)
   {
      mpExpression = Expression::unpack(pReader);
      if (mpExpression == 0)
      {
         DEBUG( "LSystemTerm: '%s' error unpacking expression .. ", mName.c_str() );
         return false;
      }
   }

   pReader->readBoolean(isPresent);
   if (isPresent)
   {
      mpArgumentExpressionList = new ArgumentExpressionList();
      bool isOkay = mpArgumentExpressionList->unserialize(pReader);
      if (isOkay == false)
      {
         DEBUG( "LSystemTerm: '%s' error unpacking argument expression list .. ", mName.c_str());
         return false;
      }
   }
   return true;
}

//////////////////////////////////////////////////////////////////////
//   Production 

// Does NOT clear the counter before counting
void LSystemProduction::countTerms(LSystemTermCounter * pCounter)
{
   if (pCounter != 0)
   {
      DoubleLinkListIterator<LSystemTerm> iter(mTerms);
      while (iter.hasMore())
      {
         LSystemTerm * pTerm = iter.getNext();
         pCounter->increment( pTerm->getTermType() );
      }
   }
}


bool LSystemProduction::serialize(ByteStreamWriter * pWriter) const
{
   assert(pWriter != 0);

   pWriter->writeString(mName);
   pWriter->writeInt(mUniqueId);
   pWriter->writeFloat(mProbability); 
   pWriter->writeBoolean(mHasProbability);  // < ? 
   pWriter->writeBoolean(mHasCondition); // << replace with pointer check for arg list and consition 

   pWriter->writeBoolean(mpFormalArgumentList != 0);
   if (mpFormalArgumentList != 0)
   {
      mpFormalArgumentList->serialize(pWriter);
   }

   pWriter->writeBoolean(mpSelectionExpression != 0);
   if (mpSelectionExpression != 0)
   {
      mpSelectionExpression->serialize(pWriter);
   }

   pWriter->writeInt(mTerms.size());
   DoubleLinkListIterator<LSystemTerm> iter(mTerms);   // the successor part 
   while (iter.hasMore())
   {
      LSystemTerm * pTerm = iter.getNext();
      pTerm->serialize(pWriter);
   }
   return true;
}

bool LSystemProduction::deserialize(ByteStreamReader * pReader)
{
   assert(pReader != 0);

   // this REQUIRES a default no-arg ctor so the object can be instantiated 
   // before bing deserialized 

   pReader->readString(mName);
   pReader->readInt(mUniqueId);
   pReader->readFloat(mProbability); 
   pReader->readBoolean(mHasProbability);
   pReader->readBoolean(mHasCondition); // << replace with pointer check for arg list and consition 

   bool isPresent;
   pReader->readBoolean(isPresent);
   if (isPresent)
   {
      mpFormalArgumentList = new FormalArgumentList();
      bool isOkay = mpFormalArgumentList->deserialize(pReader);
      if (isOkay == false)
      {
         DEBUG( "LSystemProduction: '%s' error unpacking formal argument list .. ", mName.c_str() );
         return false;
      }
   }

   pReader->readBoolean(isPresent);
   if (isPresent)
   {
      mpSelectionExpression = Expression::unpack(pReader);
      if (mpSelectionExpression == 0)
      {
         DEBUG( "LSystemProduction: '%s' error unpacking expression .. ", mName.c_str() );
         return false;
      }
   }

   int numTerms = 0;
   pReader->readInt(numTerms);
   for (int i = 0; i < numTerms; i++)
   {
      LSystemTerm * pTerm = new LSystemTerm();
      bool isOkay = pTerm->deserialize(pReader);
      if (isOkay == false)
      {
         DEBUG( "LSystemProduction: '%s' error unpacking term .. ", mName.c_str() );
         delete pTerm;
         return false;
      }
      mTerms.append(pTerm);
   }
   return true;
}

//////////////////////////////////////////////////////////////////////
//   Production Group


bool LSystemProductionGroup::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);

   pWriter->writeString(mName);

   pWriter->writeInt(mAlternateProductions.size());
   DoubleLinkListIterator<LSystemProduction> iter(mAlternateProductions);   // the successor part 
   while (iter.hasMore())
   {
      LSystemProduction * pProduction = iter.getNext();
      pProduction->serialize(pWriter);
   }
   return true;
}

bool LSystemProductionGroup::deserialize(ByteStreamReader * pReader)
{
   assert(pReader != 0);

   // this REQUIRES a default no-arg ctor so the object can be instantiated 
   // before bing deserialized 

   pReader->readString(mName);

   int numProductions = 0;
   pReader->readInt(numProductions);
   for (int i = 0; i < numProductions; i++)
   {
      LSystemProduction * pProduction = new LSystemProduction();
      bool isOkay = pProduction->deserialize(pReader);
      if (isOkay == false)
      {
         DEBUG( "LSystemProductionGroup: '%s' error unpacking production .. ", mName.c_str() );
         delete pProduction; 
         return false;
      }
      mAlternateProductions.append(pProduction);
   }
   return true;
}

//////////////////////////////////////////////////////////////////////
//   LSystem 

bool LSystem::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);

   // attributes 
   // description 
   // .... 

   // todo: 
   //pWriter->writeInt(mUserDefinedSymbols.size());
   //PtrArrayIterator<SymbolDescription> symbolIter(mUserDefinedSymbols); 
   //while (symbolIter.hasMore())
   //{
   //   SymbolDescription * pSymbolDescription = symbolIter.getNext();
   //   pSymbolDescription->serialize(pWriter);
   //}



   pWriter->writeInt(mProductions.size());
   DoubleLinkListIterator<LSystemProductionGroup> iter(mProductions);   // the successor part 
   while (iter.hasMore())
   {
      LSystemProductionGroup * pProductionGroup = iter.getNext();
      pProductionGroup->serialize(pWriter);
   }
   return true;
}

bool LSystem::deserialize(ByteStreamReader * pReader)
{
   assert(pReader != 0);

   // deserializeSymbols(pReader);
   // deserializeGroups(pReader);

   // todo 
   //int numMembers = 0; 
   //pReader->readInt(numMembers);
   //mUserDefinedSymbols.ensureMinimumSize(numMembers);
   //for (int i = 0; i < numMembers; i++)
   //{
   //   // todo: create new symbol
   //   // symbol->deserialize(pReader)
   //   // symbolList.append(symbol)
   //}

   int numProductions = 0;
   pReader->readInt(numProductions);
   for (int i = 0; i < numProductions; i++)
   {
      LSystemProductionGroup * pProductionGroup = new LSystemProductionGroup();
      bool isOkay = pProductionGroup->deserialize(pReader);
      if (isOkay == false)
      {
         DEBUG( "LSystem: error unpacking production group %d .. ", i );
         delete pProductionGroup;
         return false;
      }
      mProductions.append(pProductionGroup);
   }
   return true;
}

//////////////////////////////////////////////////////////////////////
//   ExpandedTermSequence

void ExpandedTermSequence::countTerms(LSystemTermCounter * pCounter) const
{
   if (pCounter != 0)
   {
      DoubleLinkListIterator<ExpandedTerm> iter(mpTerms);
      while (iter.hasMore())
      {
         ExpandedTerm * pExpandedTerm = iter.getNext();
         pCounter->increment( pExpandedTerm->getTermType() );
      }
   }
}


bool ExpandedTermSequence::serialize(ByteStreamWriter * pWriter) const
{
   assert(pWriter != 0);

   if (mpTerms == 0)
   {
      pWriter->writeInt(0);
   }
   else
   {
      pWriter->writeInt(mpTerms->size());
      DoubleLinkListIterator<ExpandedTerm> iter(mpTerms);
      while (iter.hasMore())
      {
         ExpandedTerm * pExpandedTerm = iter.getNext();
         pExpandedTerm->serialize(pWriter);
      }
   }
   return true;
}

bool ExpandedTermSequence::deserialize(ByteStreamReader * pReader, LSystem * pLSystem)
{
   assert(pReader != 0);

   int numTerms = 0;
   pReader->readInt(numTerms);
   if (numTerms == 0)
   {
      mpTerms = 0;
   }
   else
   {
      mpTerms = new DoubleLinkList<ExpandedTerm>();

      for (int i = 0; i < numTerms; i++)
      {
         ExpandedTerm * pExpandedTerm = new ExpandedTerm();  // todo: use free pool for these?
         bool isOkay = pExpandedTerm->deserialize(pReader, pLSystem);
         if (isOkay == false)
         {
            DEBUG( "ExpandedTermSequence: error unpacking term %d .. ", i );
            delete pExpandedTerm;
            return false;
         }
         mpTerms->append(pExpandedTerm);
      }
   }
   return true;
}


//////////////////////////////////////////////////////////////////////
//   ExpandedTerm

bool ExpandedTerm::serialize(ByteStreamWriter * pWriter) const
{
   assert(pWriter != 0);

   pWriter->writeInt(mpTerm->getUniqueId());

   pWriter->writeBoolean(mpParameterValues != 0);
   if (mpParameterValues != 0)
   {
      mpParameterValues->serialize(pWriter);
   }
   return true;
}

bool ExpandedTerm::deserialize(ByteStreamReader * pReader, LSystem * pLSystem)
{
   assert(pReader != 0);

   int uniqueTermId; 
   pReader->readInt(uniqueTermId);

   mpTerm = pLSystem->getTermById(uniqueTermId);
   if (mpTerm == 0)
   {
      DEBUG( "ExpandedTerm: deserialize: cannot find term id %d", uniqueTermId );
      return false;
   }

   bool isPresent;
   pReader->readBoolean(isPresent);
   if (isPresent)
   {
      bool isOkay = mpParameterValues->deserialize(pReader);
      if (isOkay == false)
      {
         DEBUG( "ExpandedTem: '%d' error unpacking parameter values .. ", uniqueTermId );
         return false;
      }
   }

   return true;
}

//void ExpandedTermSequence::createFromProduction(LSystemProduction * pProduction) // << ? move to a builder class ? LSystem factory method?
//{
//   assert(pProduction != 0);
//
//   mTerms.DeleteMembers(); // discard any existing terms
//
//   //setProbability(pProduction->getProbability());
//
//   ListIterator<LSystemTerm> termIter(pProduction->getTerms());
//   while (termIter.hasMore())
//   {
//      LSystemTerm * pTerm = termIter.getNext();
//      ExpandedTerm * pExpandedTerm = new ExpandedTerm((*pTerm)); 
//
//      addTerm(pExpandedTerm);
//   }
//}
