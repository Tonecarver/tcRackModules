#ifndef _tc_lsystem_term_counter_h_ 
#define _tc_lsystem_term_counter_h_   1

class LSystemTermCounter
{
public:
   virtual void increment(int termType) = 0; 
   virtual int getCount(int termType) = 0; 
   virtual void clearCounts() = 0; 
};

#endif // _tc_lsystem_term_counter_h_ 
