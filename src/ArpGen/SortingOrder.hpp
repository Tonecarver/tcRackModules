#ifndef _tc_sorting_order_h_
#define _tc_sorting_order_h_  1

enum SortingOrder { 
   SORT_ORDER_AS_RECEIVED, // sort notes in order received 
   SORT_ORDER_LOW_TO_HIGH, // sort notes in low to high order 
   SORT_ORDER_HIGH_TO_LOW, // sort notes in high to low order 
   // SORT_ORDER_RANDOM,      // sort notes in random order 
   //
   kNUM_SORTING_ORDER
}; 
// extern const char * sortingOrderNames[SortingOrder::kNUM_SORTING_ORDER]; 
extern const char * getSortingOrderName(SortingOrder sortingOrder); 
extern SortingOrder getSortingOrderByName(const char * pName); 

// TODO: maybe rename ? 
// SortingOrder_getName(val)
// SortingOrder_getVal(name)

#endif // _tc_sorting_order_h_
