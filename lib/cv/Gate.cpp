#include "Gate.hpp"

Gate::GateState Gate::gateTransitionTable[2][Gate::kNum_GATE_STATES] = 
{
	// incoming 
	// voltage     // ------------------- previous gate state ---------------------------------------
	// state       // GATE_CLOSED         GATE_LEADING        GATE_OPEN           GATE_TRAILING
    /* 0 CLOSED */   { Gate::GATE_CLOSED,       Gate::GATE_TRAILING_EDGE, Gate::GATE_TRAILING_EDGE, Gate::GATE_CLOSED       },
    /* 1 OPEN   */   { Gate::GATE_LEADING_EDGE, Gate::GATE_OPEN,          Gate::GATE_OPEN,          Gate::GATE_LEADING_EDGE },
}; 

