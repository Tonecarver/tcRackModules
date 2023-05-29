#ifndef _tc_lsystem_symbol_h_
#define _tc_lsystem_symbol_h_ 1

#include "../../../lib/datastruct/List.hpp"



enum LSystemSymbolType { 
    SYMBOL_TYPE_ACTION,        // built-in action: push, pop, 
    SYMBOL_TYPE_REWRITABLE,    // Rewritable term (to be expanded during expansion)
    SYMBOL_TYPE_USER_DEFINED,  // user-defined symbol type - opaque to the LSystem
};

 
struct LSystemSymbol : ListNode  {
    LSystemSymbolType eSymboltype_;
    

    // Symbol Id
    // Parameter Literal
    // Parameter Expression 
};

#endif // _tc_lsystem_symbol_h_
