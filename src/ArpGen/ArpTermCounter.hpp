#pragma once
#ifndef _arp_term_counter_h_ 
#define _arp_term_counter_h_   1

#include "LSystem/LSystemTermCounter.hpp"
#include "ArpTerm.hpp"

class ArpTermCounter : public LSystemTermCounter
{
public:
   ArpTermCounter();
   virtual ~ArpTermCounter();
   virtual void increment(int termType) override;
   virtual int getCount(int termType) override; 
   virtual void clearCounts() override; 

   bool hasNoteActions();
   bool hasHarmonyActions(int harmonyId);

private:
   int mCounts[ ArpTermType::kNUM_TERM_TYPES ];

   bool isValidIdx(int idx) const
   {
      return ((0 <= idx) && (idx < ArpTermType::kNUM_TERM_TYPES));
   }
};

#endif // _arp_term_counter_h_ 