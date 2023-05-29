#include "ArpTermCounter.hpp"
#include <string.h>  // for memset 

ArpTermCounter::ArpTermCounter()
{
   clearCounts();
}

ArpTermCounter::~ArpTermCounter()
{
}

void ArpTermCounter::increment(int termType)
{
   if (isValidIdx(termType))
   {
      mCounts[ termType ]++;
   }
}

int ArpTermCounter::getCount(int termType)
{
   if (isValidIdx(termType))
   {
      return mCounts[ termType ];
   }
   return 0;
}

void ArpTermCounter::clearCounts()
{
   memset(mCounts, 0 , sizeof mCounts);
}

bool ArpTermCounter::hasNoteActions()
{
   // TODO: optimize: do this sum once 
   
   int count = 
      mCounts[ ArpTermType::TERM_NOTE ];
      
      // +  mCounts[ ArpTermType::TERM_CHORD ]; 
      //  + mCounts[ ArpTermType::TERM_GLISS ];

   return (count > 0);
}

bool ArpTermCounter::hasHarmonyActions(int harmonyId)
{
   int count = 0;

#if ARP_PLAYER_NUM_HARMONIES >= 1
   if (harmonyId == 0)
   {
      count = 
         mCounts[ ArpTermType::TERM_HARMONY_1_ASSIGN ] +
         mCounts[ ArpTermType::TERM_HARMONY_1_UP ] +
         mCounts[ ArpTermType::TERM_HARMONY_1_DOWN ] +
         mCounts[ ArpTermType::TERM_HARMONY_1_NOMINAL ] +
         mCounts[ ArpTermType::TERM_HARMONY_1_RANDOM ];
   }
#endif 

#if ARP_PLAYER_NUM_HARMONIES >= 2
   else if (harmonyId == 1)
   {
      count = 
         mCounts[ ArpTermType::TERM_HARMONY_2_ASSIGN ] +
         mCounts[ ArpTermType::TERM_HARMONY_2_UP ] +
         mCounts[ ArpTermType::TERM_HARMONY_2_DOWN ] +
         mCounts[ ArpTermType::TERM_HARMONY_2_NOMINAL ] +
         mCounts[ ArpTermType::TERM_HARMONY_2_RANDOM ];
   }
#endif 

#if ARP_PLAYER_NUM_HARMONIES >= 3
   else if (harmonyId == 2)
   {
      count = 
         mCounts[ ArpTermType::TERM_HARMONY_3_ASSIGN ] +
         mCounts[ ArpTermType::TERM_HARMONY_3_UP ] +
         mCounts[ ArpTermType::TERM_HARMONY_3_DOWN ] +
         mCounts[ ArpTermType::TERM_HARMONY_3_NOMINAL ] +
         mCounts[ ArpTermType::TERM_HARMONY_3_RANDOM ];
   }
#endif 

#if ARP_PLAYER_NUM_HARMONIES > 3
  Unsupported number of Harmonies
#endif 

  return (count > 0);
}

