#ifndef _tc_gate_h_
#define _tc_gate_h_ 1

struct Gate {
	enum GateState { 
		GATE_CLOSED, 
		GATE_LEADING_EDGE,
		GATE_OPEN,
		GATE_TRAILING_EDGE,
		//
		kNum_GATE_STATES
	}; 

	Gate(GateState state = GATE_CLOSED) : mState(state) {}

	bool isOpen()         const { return mState == GateState::GATE_OPEN || mState == GateState::GATE_LEADING_EDGE; }
	bool isClosed()       const { return mState == GateState::GATE_CLOSED || mState == GateState::GATE_TRAILING_EDGE; }
	bool isLeadingEdge()  const { return mState == GateState::GATE_LEADING_EDGE; }
	bool isTrailingEdge() const { return mState == GateState::GATE_TRAILING_EDGE; }

	GateState getState()  const { return mState; }

	void setOpen(bool beOpen) {
		mState = gateTransitionTable[ beOpen ? 1 : 0 ][ mState ];
	}

	void close() {
		mState = gateTransitionTable[ 0 ][ mState ];
	}

private:
	GateState mState; 
	static GateState gateTransitionTable[2][GateState::kNum_GATE_STATES];
}; 

#endif // _tc_gate_h_
