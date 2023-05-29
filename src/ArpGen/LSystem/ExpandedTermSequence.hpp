#pragma once
#ifndef _tc_expanded_term_sequence_h_
#define _tc_expanded_term_sequence_h_ 1

#include "../../plugin.hpp"

#include "ExpandedTerm.hpp"
#include "LSystem.hpp"
#include "LSystemTerm.hpp"
#include "LSystemTermCounter.hpp"

#include "../../../lib/datastruct/List.hpp"

#include <string>

class ExpandedTermSequence //: public Node
{
public:
   ExpandedTermSequence()
      : mpTerms(0)
      //: mProbability(1.0)
   {
   }

   virtual ~ExpandedTermSequence()
   {
      delete mpTerms;  /// << todo: use external freePool for this, siphon off a few every processDoubleReplacing ... 
   }

   bool isEmpty() const
   {
      return (mpTerms == 0) || (mpTerms->size() == 0); 
   }

   void setTerms(DoubleLinkList<ExpandedTerm> * pTerms)
   {
      mpTerms = pTerms;  
   }

   DoubleLinkList<ExpandedTerm> & getTerms() const
   {
      assert(mpTerms != 0);
      return (*mpTerms);  
   }

   int getLength() const 
   {
      if (mpTerms != 0)
      {
         return mpTerms->size();
      }
      return 0;
   }

   // Does NOT clear the counter before counting
   void countTerms(LSystemTermCounter * pCounter) const;

   bool serialize(ByteStreamWriter * pWriter) const;
   bool deserialize(ByteStreamReader * pReader, LSystem * pLSystem);

private:
   mutable DoubleLinkList<ExpandedTerm> * mpTerms;  
};


#endif // __tc_expanded_term_sequence_h_
