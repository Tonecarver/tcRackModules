#include "SortingOrder.hpp"
#include "../../lib/string/StringUtil.hpp"

// Must match enum order in SortingOrder 
const char * sortingOrderNames[SortingOrder::kNUM_SORTING_ORDER] = {
   "AsReceived",  // SORT_ORDER_AS_RECEIVED, // sort notes in order received 
   "LowToHigh",   // SORT_ORDER_LOW_TO_HIGH, // sort notes in low to high order 
   "HighToLow",   // SORT_ORDER_HIGH_TO_LOW, // sort notes in high to low order 
   // "Random",      // SORT_ORDER_RANDOM,      // sort notes in random order 
}; 

const char * getSortingOrderName(SortingOrder sortingOrder) {
   return (sortingOrder >= 0 && sortingOrder < SortingOrder::kNUM_SORTING_ORDER) ? sortingOrderNames[sortingOrder] : "";
}

SortingOrder getSortingOrderByName(const char * pName) {
   for (int i = 0; i < SortingOrder::kNUM_SORTING_ORDER; i++) {
      if (startsWithIgnoreCase(sortingOrderNames[i], pName) >= 2) { 
         return (SortingOrder) i;
      }
   }
   return (SortingOrder) -1; // no match 
}
