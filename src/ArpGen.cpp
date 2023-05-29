#include "plugin.hpp"
#include <osdialog.h>

// #define INCLUDE_DEBUG_FUNCTIONS 1 

#include "ArpGen/parser/SpecParser.hpp"
#include "../lib/io/AsciiFileReader.hpp"

#include "ArpGen/LSystem/LSystem.hpp"
#include "ArpGen/LSystem/LSystemExpander.hpp"
#include "ArpGen/LSystem/LSystemExecutor.hpp"
#include "Arpgen/ArpPlayer.hpp"
#include "ArpGen/NoteTable.hpp"
#include "ArpGen/ChannelManager.hpp"
#include "ArpGen/ArpTermCounter.hpp"
#include "../lib/scale/Scales.hpp"
#include "../lib/scale/TwelveToneScale.hpp"
#include "../lib/datastruct/DelayList.hpp"
#include "../lib/cv/CvEvent.hpp"
#include "../lib/datastruct/FreePool.hpp"
#include "../lib/clock/ClockWatcher.hpp"
#include "../lib/arpeggiator/Stepper.hpp"

const float DFLT_BPM = 120.f;

#define MIN_EXPANSION_DEPTH (2)
#define MAX_EXPANSION_DEPTH (200)
#define DFLT_EXPANSION_DEPTH (10)


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static bool verbose = false;
#define MYTRACE(x) if (verbose) { DEBUG x; }

static std::string arprulesDir;

class DebugPrintWriter : public PrintWriter
{
public:
	DebugPrintWriter()
	{
		buf[0] = 0;
	}

	virtual ~DebugPrintWriter()
	{
	}

	virtual void printChars(const char * pStr) override
	{
		if (pStr) {
			strcat(buf, pStr);
		}
	}

	virtual void printlnChars(const char * pStr) override
	{
		if (pStr) {
			strcat(buf, pStr ? pStr : "<~NULL~>");
		}
		DEBUG("%s", buf);
		buf[0] = 0;
	}

private:
	char buf[1000];
}; 


// void log_lsystem(LSystem * pLsystem, const char * pHint) {
// 	errno = 0;
// 	// FILE *pFile = fopen("c:\\Bill\\arpgen.log", "a");
// 	// if (pFile) {
// 	// 	fprintf(pFile, "-- Lsystem: %s\n", pHint != 0 ? pHint : "");
// 	// 	FilePrintWriter writer(pFile);
// 	// 	pLsystem->write(writer);
// 	// 	fclose(pFile);
// 	// }
// 	DEBUG("------------------------------------- Lsystem: %s", pHint ? pHint : "");
// 	DebugPrintWriter writer;
// 	pLsystem->write(writer);
// }

// voct range is -5.f .. 5.f
int voctToPitch(float voct) {  
	int pitch = round((voct + 5.f) * 12.f); // 0 .. 120 
	return pitch;
}

int voctToOctave(float voct) {  
	int octave = round(((voct + 5.f) * 12.f) / 10.f); // 10 octaves in 0 .. 120 
	return octave;
}

int voctToDegreeRelativeToC(float voct) {  // degree, relative to C
	int pitch = round((voct + 5.f) * 12.f);
	int degree = pitch % 12;
	return degree;
}

// pitch is 0..120
// voct is -5..0..5 
float pitchToVoct(int pitch) {
	return (float(pitch) / 12.f) - 5.f;
}

// NOT USED YET 
// float modulationVoltageToZeroToOne(float plusMinusFiveValue) {
// 	return (plusMinusFiveValue + 5.f) * 0.1f;
// }
// float modulationVoltageToPlusMinusOne(float plusMinusFiveValue) {
// 	return plusMinusFiveValue * 0.2f; // divide by 5
// }
// float modulationVoltageToZeroTen(float plusMinusFiveValue) {
// 	return plusMinusFiveValue + 5.f;
// }
// 


template <class T>
class OutputNoteWriter : public ArpNoteWriter {
public:
	T * pReceiver; 


// TODO: take timing aspects out of the ArpPlayer 
//       have player report delays and/or note length as percentage of step time

	// virtual void noteOn(int channelNumber, int noteNumber, int velocity, int lengthSamples, int delaySamples) override {
	// 	DEBUG("NoteWriter: channel %d, note %d, velocity %d, lengthSamples %d, dddelaySamples %d", channelNumber, noteNumber, velocity, lengthSamples, delaySamples);
	// 	pReceiver->noteOn(channelNumber, noteNumber, velocity, lengthSamples, delaySamples);
	// }
	virtual void noteOn(int channelNumber, int noteNumber, int velocity) override {
		// DEBUG("NoteWriter: channel %d, note %d, velocity %d", channelNumber, noteNumber, velocity);
		pReceiver->noteOn(channelNumber, noteNumber, velocity);
	}
}; 


struct TcArpGen : Module {
	enum ParamId {
		ARP_ALGORITHM_PARAM,
		ARP_DELAY_0_PARAM,
		ARP_DELAY_1_PARAM,
		ARP_DELAY_2_PARAM,
		ARP_DELAY_3_PARAM,
		ARP_OCTAVE_MAX_PARAM,
		ARP_OCTAVE_MIN_PARAM,
		ARP_PROBABILITY_PARAM,
		DELAY_TIMING_PARAM,
		DOUBLE_TIME_BUTTON_PARAM,
		DOUBLE_TIME_COUNT_PARAM,
		EXPANSION_DEPTH_PARAM,
		HARMONY_1_DELAY_0_PARAM,
		HARMONY_1_DELAY_1_PARAM,
		HARMONY_1_DELAY_2_PARAM,
		HARMONY_1_DELAY_3_PARAM,
		HARMONY_2_DELAY_0_PARAM,
		HARMONY_2_DELAY_1_PARAM,
		HARMONY_2_DELAY_2_PARAM,
		HARMONY_2_DELAY_3_PARAM,
		HARMONY_3_DELAY_0_PARAM,
		HARMONY_3_DELAY_1_PARAM,
		HARMONY_3_DELAY_2_PARAM,
		HARMONY_3_DELAY_3_PARAM,
		HARMONY_OCTAVE_MAX_PARAM,
		HARMONY_OCTAVE_MIN_PARAM,
		HARMONY_OVERSHOOT_STRATEGY_PARAM,
		HARMONY_PROBABILITY_PARAM,
		INTERVAL_STEP_SIZE_PARAM,
		KEY_ORDER_BUTTON_PARAM,
		KEY_STRIDE_PARAM,
		MAINTAIN_CONTEXT_BUTTON_PARAM,
		OBEY_PARAM,
		OCTAVE_PERFECT_BUTTON_PARAM,
		PITCH_A_PARAM,
		PITCH_A_SHARP_PARAM,
		PITCH_B_PARAM,
		PITCH_C_PARAM,
		PITCH_C_SHARP_PARAM,
		PITCH_D_PARAM,
		PITCH_D_SHARP_PARAM,
		PITCH_E_PARAM,
		PITCH_F_PARAM,
		PITCH_F_SHARP_PARAM,
		PITCH_G_PARAM,
		PITCH_G_SHARP_PARAM,
		PLAY_DIRECTION_PARAM,
		RANDOMIZE_NOTES_BUTTON_PARAM,
		RESET_BUTTON_PARAM,
		ROOT_PITCH_PARAM,
		RUN_BUTTON_PARAM,
		SEMITONE_OVERSHOOT_STRATEGY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ARP_ALGORITHM_CV_INPUT,
		ARP_DELAY_CV_INPUT,
		ARP_OCTAVE_MAX_CV_INPUT,
		ARP_OCTAVE_MIN_CV_INPUT,
		ARP_PROBABILITY_CV_INPUT,
		CLOCK_CV_INPUT,
		DELAY_TIMING_CV_INPUT,
		DOUBLE_TIME_CV_INPUT,
		HARMONY_1_DELAY_CV_INPUT,
		HARMONY_2_DELAY_CV_INPUT,
		HARMONY_3_DELAY_CV_INPUT,
		HARMONY_OCTAVE_MAX_CV_INPUT,
		HARMONY_OCTAVE_MIN_CV_INPUT,
		HARMONY_OVERSHOOT_STRATEGY_CV_INPUT,
		HARMONY_PROBABILITY_CV_INPUT,
		INTERVAL_STEP_SIZE_CV_INPUT,
		KEY_STRIDE_CV_INPUT,
		OBEY_CV_INPUT,
		OCTAVE_PERFECT_CV_INPUT,
		PLAY_DIRECTION_CV_INPUT,
		POLY_GATE_IN_INPUT,
		POLY_IN_CV_INPUT,
		POLY_SCALE_CV_INPUT,
		RANDOMIZE_NOTES_CV_INPUT,
		RESET_CV_INPUT,
		ROOT_PITCH_CV_INPUT,
		RUN_CV_INPUT,
		SEMITONE_OVERSHOOT_STRATEGY_CV_INPUT,
		KEY_ORDER_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		CLOCK_OUTPUT,
		END_OF_CYCLE_OUTPUT,
		GATE_FOUR_OUTPUT,
		GATE_ONE_OUTPUT,
		GATE_POLY_OUTPUT,
		GATE_THREE_OUTPUT,
		GATE_TWO_OUTPUT,
		RUN_OUTPUT,
		VOCT_FOUR_OUTPUT,
		VOCT_ONE_OUTPUT,
		VOCT_POLY_OUTPUT,
		VOCT_THREE_OUTPUT,
		VOCT_TWO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ARP_DELAY_0_LED_LIGHT,
		ARP_DELAY_1_LED_LIGHT,
		ARP_DELAY_2_LED_LIGHT,
		ARP_DELAY_3_LED_LIGHT,
		DIRECTION_FORWARD_LED_LIGHT,
		DIRECTION_REVERSE_LED_LIGHT,
		DOUBLE_TIME_LED_LIGHT,
		// HARMONY_1_DELAY_0_LED_LIGHT,
		// HARMONY_1_DELAY_1_LED_LIGHT,
		// HARMONY_1_DELAY_2_LED_LIGHT,
		// HARMONY_1_DELAY_3_LED_LIGHT,
		// HARMONY_2_DELAY_0_LED_LIGHT,
		// HARMONY_2_DELAY_1_LED_LIGHT,
		// HARMONY_2_DELAY_2_LED_LIGHT,
		// HARMONY_2_DELAY_3_LED_LIGHT,
		// HARMONY_3_DELAY_0_LED_LIGHT,
		// HARMONY_3_DELAY_1_LED_LIGHT,
		// HARMONY_3_DELAY_2_LED_LIGHT,
		// HARMONY_3_DELAY_3_LED_LIGHT,
		KEY_ORDER_AS_RECEIVED_LED_LIGHT,
		KEY_ORDER_HIGH_TO_LOW_LED_LIGHT,
		KEY_ORDER_LOW_TO_HIGH_LED_LIGHT,
		MAINTAIN_CONTEXT_LED_LIGHT,
		OCTAVE_PERFECT_LED_LIGHT,
		PITCH_A_LIGHT,
		PITCH_A_SHARP_LIGHT,
		PITCH_B_LIGHT,
		PITCH_C_LIGHT,
		PITCH_C_SHARP_LIGHT,
		PITCH_D_LIGHT,
		PITCH_D_SHARP_LIGHT,
		PITCH_E_LIGHT,
		PITCH_F_LIGHT,
		PITCH_F_SHARP_LIGHT,
		PITCH_G_LIGHT,
		PITCH_G_SHARP_LIGHT,
		RANDOMIZE_NOTES_LED_LIGHT,
		RESET_LED_LIGHT,
		ENUMS(RULES_VALID_LED_LIGHT,2),
		RUN_LED_LIGHT,
		LIGHTS_LEN
	};


	std::string mRulesFilepath = "click to select file"; 

	// -- Scale --
	int activeScaleIdx = 0;
	ScaleDefinition customScaleDefinition;
	bool scaleRedrawRequired = true;

	struct ScaleButton {
		int mParamIdx;
		int mLightIdx;
	}; 

	ScaleButton scaleButtons[12] = {
		{ PITCH_C_PARAM,       PITCH_C_LIGHT       },
		{ PITCH_C_SHARP_PARAM, PITCH_C_SHARP_LIGHT },
		{ PITCH_D_PARAM,       PITCH_D_LIGHT       },
		{ PITCH_D_SHARP_PARAM, PITCH_D_SHARP_LIGHT },
		{ PITCH_E_PARAM,       PITCH_E_LIGHT       },
		{ PITCH_F_PARAM,       PITCH_F_LIGHT       },
		{ PITCH_F_SHARP_PARAM, PITCH_F_SHARP_LIGHT },
		{ PITCH_G_PARAM,       PITCH_G_LIGHT       },
		{ PITCH_G_SHARP_PARAM, PITCH_G_SHARP_LIGHT },
		{ PITCH_A_PARAM,       PITCH_A_LIGHT       },
		{ PITCH_A_SHARP_PARAM, PITCH_A_SHARP_LIGHT },
		{ PITCH_B_PARAM,       PITCH_B_LIGHT       },
	};

	dsp::SchmittTrigger scaleButtonTriggers[12];
	bool scaleButtonEnabled[12];

	// -- random -- 
	FastRandom mRandom;
	dsp::SchmittTrigger  mRandomizeHarmoniesTrigger; 

	// -- clock --
	ClockWatcher clock;
	dsp::SchmittTrigger	mResetButtonTrigger; 
	dsp::SchmittTrigger	mResetTrigger; 
	dsp::PulseGenerator mResetPulse;

	// -- inputs --
	NoteTable noteTable;  // Input Notes 

	// -- L-System --
	// Double-buffered to allow import of new rules without affecting 
	// currently active rules. Also, if new import fails the active
	// rules remain intact. 
	LSystem               mLSystem[2]; // L-System rules
	LSystem             * mpLSystemActive  = &mLSystem[0];
	LSystem             * mpLSystemPending = &mLSystem[1];
	LSystemMemoryRecycler mMemoryRecycler;                // TODO: << siphon this every few cycles

	// -- arp player -- 
	ExpandedTermSequence  * mpExpandedTermSequence = 0; // the expanded L-System: list of ExpandedTerms
	ArpTermCounter          mTermCounter;
	ArpPlayer               mArpPlayer;
	OutputNoteWriter<TcArpGen> mOutputNoteWriter;
	dsp::PulseGenerator        mEocPulseGenerator;
	float               mNoteProbability = 1.f;
	float               mHarmonyProbability = 1.f;
	int                 mExpansionDepth = DFLT_EXPANSION_DEPTH;
	StepperAlgo         mArpPlayerArpStyle = StepperAlgo::STEPPER_ALGO_CYCLE_UP;
	StepperAlgo         mHarmonyIntervalStepperStrategy = StepperAlgo::STEPPER_ALGO_CYCLE_UP;
	StepperAlgo         mSemitoneIntervalStepperStrategy = StepperAlgo::STEPPER_ALGO_CYCLE_UP;

	SortingOrder        mArpPlayerSortingOrder = SortingOrder::SORT_ORDER_LOW_TO_HIGH; 
	dsp::SchmittTrigger mSortingOrderTrigger;

	ScaleOutlierStrategy mOutlierStrategy = ScaleOutlierStrategy::OUTLIER_SNAP_TO_SCALE; // TODO: not set by UI, remove 

	int                  mArpOctaveMin = 0;
	int                  mArpOctaveMax = 0;

	int                  mHarmonyOctaveMin = 0;
	int                  mHarmonyOctaveMax = 0;

	dsp::SchmittTrigger mPerfectOctaveTrigger;
	bool                mOctaveIncludesPerfect = false;

	int 			    mIntervalStepSize = 1;

	LSystemExecuteDirection mPlayDirection = LSystemExecuteDirection::DIRECTION_FORWARD; 

	bool                arpPlayerRedrawRequired = true;  // UI refresh requested

	KeysOffStrategy     mKeysOffStrategy = KeysOffStrategy::KEYS_OFF_MAINTAIN_CONTEXT;
	dsp::SchmittTrigger mKeysOffTrigger;
	bool keysOffStrategyRedrawRequired = true;
	// todo; delete 
	// int keysOffStrategiesLightIds[KeysOffStrategy::kNUM_KEYS_OFF_STRATEGIES] = {
	// 	IDLE_MAINTAIN_LED_LIGHT, 
	// 	IDLE_2_LED_LIGHT, 
	// };

	int mStride = 1; // number of key positions to move on an increment or decrement operation

	enum InterlockState {
		INTERLOCK_OFF,
		INTERLOCK_NORMALED,
		INTERLOCK_NORMALED_INVERTED,
		//
		kNUM_INTERLOCK_STATES
	}; 
	InterlockState mOctaveInterlockRoot = InterlockState::INTERLOCK_OFF;
	InterlockState mOctaveInterlockHarmony = InterlockState::INTERLOCK_OFF;
	InterlockState mOctaveInterlockBoth = InterlockState::INTERLOCK_OFF;

	InterlockState selectNextInterlockState(InterlockState in) {
		int intval = in + 1;
		if (intval >= InterlockState::kNUM_INTERLOCK_STATES) { 
			intval = 0;
		}
		return (InterlockState) intval;
	}

	// bool mOctaveInterlockBoth = false; 
	// bool mOctaveInterlockRoot = false; 
	// bool mOctaveInterlockHarmony = false; 
	dsp::SchmittTrigger mOctaveInterlockBothTrigger; 
	dsp::SchmittTrigger mOctaveInterlockRootTrigger; 
	dsp::SchmittTrigger mOctaveInterlockHarmonyTrigger; 

	bool                mRunning = true; 
	dsp::SchmittTrigger mRunTrigger; 
	dsp::SchmittTrigger mRunButtonTrigger; 

/** experiment */
	int mTwinkleSteps = 3;  // number of steps to take when activated
	int mTwinkleCount = 0;  // number of active steps pending 
	const float minTwinkleSteps = 3.f; 
	const float maxTwinkleSteps = 32.f;
	dsp::SchmittTrigger mTwinkleTrigger; // TODO: add trigger to reset 
	float mTwinklePercent = 0.5f;

	bool mIsDoubleTime = false; // true if outputting at 2x clock 
/* end */

	// -- Delays -- 

	enum { 
		NUM_DELAYS = 4,
		NUM_DELAY_SLOTS = 4
	};

	float delayPercentage[4] = { // TODO: const
		0.f, 0.f, 0.f, 0.f
	}; 

	int delaySelected[4] = { // TODO: const
		0, 0, 0, 0 
	};

	float mSwingAmount = 0.f;

	float delayPercentageOptions[NUM_DELAY_SLOTS] = { 
  		0.f, 0.25f, 0.5f, 0.75f
	}; 

	void applySwing(float swingAmount) {
		mSwingAmount = swingAmount;

		delayPercentageOptions[0] = 0.f; 
		delayPercentageOptions[1] = 0.25f + (swingAmount * ((1.f/3.f) - 0.25f)); 
		delayPercentageOptions[2] = 0.50f + (swingAmount * ((2.f/3.f) - 0.50f)); 
		delayPercentageOptions[3] = 0.75f + (swingAmount * ((3.f/3.f) - 0.75f)); // TODO: wrap to 0 ? 

		for (int i = 0; i < 4; i++) {
			delayPercentage[i] = delayPercentageOptions[ delaySelected[i] ];
		}
	}

	bool delayRedrawRequired = false;

	void setDelaySelection(int delayIdx, int selectionIdx) {
		delaySelected[delayIdx] = selectionIdx;
		delayPercentage[delayIdx] = delayPercentageOptions[selectionIdx];
		delayRedrawRequired = true;
	}


	dsp::SchmittTrigger delayTriggers[4][NUM_DELAY_SLOTS]; // TODO const // cannot compile this into struct ?

	struct DelaySelector { 
		int delayIdx;
		int inputId; // cv_port 
		int delaySwitchIds[NUM_DELAY_SLOTS];  // TODO: const 
	}; 

	DelaySelector delaySelectors[4] = // TODO: const
	{
	    { 0, // delayIdx
		  ARP_DELAY_CV_INPUT,
		  {
			ARP_DELAY_0_PARAM,
			ARP_DELAY_1_PARAM,
			ARP_DELAY_2_PARAM,
			ARP_DELAY_3_PARAM,
		  }
		},
		{ 1, // delayIdx
		  HARMONY_1_DELAY_CV_INPUT, 
		  {
			HARMONY_1_DELAY_0_PARAM,
			HARMONY_1_DELAY_1_PARAM,
			HARMONY_1_DELAY_2_PARAM,
			HARMONY_1_DELAY_3_PARAM,
		  }
		},
		{ 2, // delayIdx
		  HARMONY_2_DELAY_CV_INPUT, 
		  {
			HARMONY_2_DELAY_0_PARAM,
			HARMONY_2_DELAY_1_PARAM,
			HARMONY_2_DELAY_2_PARAM,
			HARMONY_2_DELAY_3_PARAM,
		  }
		},
		{ 3, // delayIdx
		  HARMONY_3_DELAY_CV_INPUT, 
		  {
			HARMONY_3_DELAY_0_PARAM,
			HARMONY_3_DELAY_1_PARAM,
			HARMONY_3_DELAY_2_PARAM,
			HARMONY_3_DELAY_3_PARAM,
		  }
		}
	};

	// -- output --
	ChannelManager      polyChannelManager[4]; // TODO const
	DelayList<CvEvent>  outputQueue;
	FreePool<CvEvent>   cvEventPool;

	struct OutputPort {
		int gateId; 
		int portId;
	}; 

	OutputPort outputPorts[4]  = {
		{ GATE_ONE_OUTPUT,   VOCT_ONE_OUTPUT   },
		{ GATE_TWO_OUTPUT,   VOCT_TWO_OUTPUT   },
		{ GATE_THREE_OUTPUT, VOCT_THREE_OUTPUT },
		{ GATE_FOUR_OUTPUT,  VOCT_FOUR_OUTPUT  },
	}; 

	// -- parameter processing counter --
	int mParameterPollCount = 0;  // number of samples to wait to collect parameters 
	const int PARAMETER_POLL_SAMPLES = 8;

	// -- -- 
	TcArpGen() {

		DEBUG("ARP GEN: constructor() .. ");

		struct ArpStyleQuantity : ParamQuantity {
			char formattedValue[24];

			std::string getDisplayValueString() override { // TODO: add method to class, range check index 
				sprintf(formattedValue, "%s", stepperAlgoNames[ ((TcArpGen*)module)->mArpPlayerArpStyle ]);
				return formattedValue;
			}
		};
		struct SemitoneStrategyQuantity : ParamQuantity {
			char formattedValue[24];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%s", stepperAlgoNames[ ((TcArpGen*)module)->mSemitoneIntervalStepperStrategy ]);
				return formattedValue;
			}
		};
		struct HarmonyStrategyQuantity : ParamQuantity {
			char formattedValue[24];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%s", stepperAlgoNames[ ((TcArpGen*)module)->mHarmonyIntervalStepperStrategy ]);
				return formattedValue;
			}
		};
		struct OutlierStrategyQuantity : ParamQuantity {
			char formattedValue[24];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%s", scaleOutlierStrategyNames[ ((TcArpGen*)module)->mOutlierStrategy ]);
				return formattedValue;
			}
		};
		struct StrideQuantity : ParamQuantity {
			char formattedValue[8];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%d", ((TcArpGen*)module)->mStride);
				return formattedValue;
			}
		};
		struct RootPitchQuantity : ParamQuantity {
			char formattedValue[16];

			std::string getDisplayValueString() override {
				int rootPitch = ((TcArpGen*)module)->mArpPlayer.getRootPitch();
				sprintf(formattedValue, "%s%d (%d)", TwelveToneScale::noteName(rootPitch), TwelveToneScale::midiPitchToOctave(rootPitch), rootPitch);
				return formattedValue;
			}
		};
		struct TwinkleQuantity : ParamQuantity {
			char formattedValue[16];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%d", ((TcArpGen*)module)->mTwinkleSteps);
				return formattedValue;
			}
		};

		struct KeyOrderQuantity : ParamQuantity {
			char formattedValue[16];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%s", getSortingOrderName( ((TcArpGen*)module)->mArpPlayerSortingOrder ));
				return formattedValue;
			}

			void setDisplayValueString(std::string s) override {
				int order = getSortingOrderByName(s.c_str());
				if (order >= 0) {
					setValue(float(order) / float(SortingOrder::kNUM_SORTING_ORDER - 1)); // as 0..1 to match param config 
// 					((TcArpGen*)module)->mArpPlayerSortingOrder = order;
				}
			}
		};

		struct ArpOctaveMinQuantity : ParamQuantity {
			char formattedValue[8];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%d", ((TcArpGen*)module)->mArpOctaveMin);
				return formattedValue;
			}
		};

		struct ArpOctaveMaxQuantity : ParamQuantity {
			char formattedValue[8];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%d", ((TcArpGen*)module)->mArpOctaveMax); // TODO: use dynamic_cast<>()
				return formattedValue;
			}
		};

		struct HarmonyOctaveMinQuantity : ParamQuantity {
			char formattedValue[16];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%d", ((TcArpGen*)module)->mHarmonyOctaveMin);
				return formattedValue;
			}
		};

		struct HarmonyOctaveMaxQuantity : ParamQuantity {
			char formattedValue[16];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%d", ((TcArpGen*)module)->mHarmonyOctaveMax);
				return formattedValue;
			}
		};

		struct IntervalStepSizeQuantity : ParamQuantity {
			char formattedValue[8];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%d", ((TcArpGen*)module)->mIntervalStepSize);
				return formattedValue;
			}
		};

		struct PlayDirectionQuantity : ParamQuantity {
			char formattedValue[16];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%s", lsystemExecuteDirectionNames[ ((TcArpGen*)module)->mPlayDirection ]);
				return formattedValue;
			}
		};

		struct ExpansionDepthQuantity : ParamQuantity {
			char formattedValue[8];

			std::string getDisplayValueString() override {
				sprintf(formattedValue, "%d", ((TcArpGen*)module)->mExpansionDepth);
				return formattedValue;
			}
		};

		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam<ArpStyleQuantity>(ARP_ALGORITHM_PARAM, 0.f, StepperAlgo::kNum_STEPPER_ALGOS - 1, StepperAlgo::STEPPER_ALGO_CYCLE_UP, "Note Arp Strategy");
		getParamQuantity(ARP_ALGORITHM_PARAM)->snapEnabled = true;
		configParam(ARP_DELAY_0_PARAM, 0.f, 1.f, 0.f, "Arp Delay 0");
		configParam(ARP_DELAY_1_PARAM, 0.f, 1.f, 0.f, "Arp Delay 1");
		configParam(ARP_DELAY_2_PARAM, 0.f, 1.f, 0.f, "Arp Delay 2");
		configParam(ARP_DELAY_3_PARAM, 0.f, 1.f, 0.f, "Arp Delay 3");
		configParam<ArpOctaveMaxQuantity>(ARP_OCTAVE_MAX_PARAM, -1.f, 1.f, 0.f, "Arp Octave Max");
		configParam<ArpOctaveMinQuantity>(ARP_OCTAVE_MIN_PARAM, -1.f, 1.f, 0.f, "Arp Octave Min");
		configParam(ARP_PROBABILITY_PARAM, 0.f, 1.f, 1.f, "Arp Probability");
		configParam(DELAY_TIMING_PARAM, 0.f, 1.f, 0.f, "Delay Swing");
		configParam(DOUBLE_TIME_BUTTON_PARAM, 0.f, 1.f, 0.f, "Double Time");
		configParam<TwinkleQuantity>(DOUBLE_TIME_COUNT_PARAM, 0.f, 1.f, 0.f, "Double Time Burst Count");
		configParam<ExpansionDepthQuantity>(EXPANSION_DEPTH_PARAM, MIN_EXPANSION_DEPTH, MAX_EXPANSION_DEPTH, DFLT_EXPANSION_DEPTH, "Expansion Depth");
		configParam(HARMONY_1_DELAY_0_PARAM, 0.f, 1.f, 0.f, "Harm 1 Delay 0");
		configParam(HARMONY_1_DELAY_1_PARAM, 0.f, 1.f, 0.f, "Harm 1 Delay 1");
		configParam(HARMONY_1_DELAY_2_PARAM, 0.f, 1.f, 0.f, "Harm 1 Delay 2");
		configParam(HARMONY_1_DELAY_3_PARAM, 0.f, 1.f, 0.f, "Harm 1 Delay 3");
		configParam(HARMONY_2_DELAY_0_PARAM, 0.f, 1.f, 0.f, "Harm 2 Delay 0");
		configParam(HARMONY_2_DELAY_1_PARAM, 0.f, 1.f, 0.f, "Harm 2 Delay 1");
		configParam(HARMONY_2_DELAY_2_PARAM, 0.f, 1.f, 0.f, "Harm 2 Delay 2");
		configParam(HARMONY_2_DELAY_3_PARAM, 0.f, 1.f, 0.f, "Harm 2 Delay 3");
		configParam(HARMONY_3_DELAY_0_PARAM, 0.f, 1.f, 0.f, "Harm 3 Delay 0");
		configParam(HARMONY_3_DELAY_1_PARAM, 0.f, 1.f, 0.f, "Harm 3 Delay 1");
		configParam(HARMONY_3_DELAY_2_PARAM, 0.f, 1.f, 0.f, "Harm 3 Delay 2");
		configParam(HARMONY_3_DELAY_3_PARAM, 0.f, 1.f, 0.f, "Harm 3 Delay 3");
		configParam<HarmonyOctaveMaxQuantity>(HARMONY_OCTAVE_MAX_PARAM, -1.f, 1.f, 0.f, "Harmony Octave Max");
		configParam<HarmonyOctaveMinQuantity>(HARMONY_OCTAVE_MIN_PARAM, -1.f, 1.f, 0.f, "Harmony Octave Min");
		configParam<HarmonyStrategyQuantity>(HARMONY_OVERSHOOT_STRATEGY_PARAM, 0.f, StepperAlgo::kNum_STEPPER_ALGOS - 1, StepperAlgo::STEPPER_ALGO_CYCLE_UP, "Harmony Overshoot"); 
		getParamQuantity(HARMONY_OVERSHOOT_STRATEGY_PARAM)->snapEnabled = true;
		configParam(HARMONY_PROBABILITY_PARAM, 0.f, 1.f, 1.f, "Harmony Probability");
		configParam<IntervalStepSizeQuantity>(INTERVAL_STEP_SIZE_PARAM, 1.f, 8.f, 1.f, "Interval Step Size");
		getParamQuantity(INTERVAL_STEP_SIZE_PARAM)->snapEnabled = true;
		configParam<KeyOrderQuantity>(KEY_ORDER_BUTTON_PARAM, 0.f, 1.f, 0.f, "Note Sorting Order");
		configParam<StrideQuantity>(KEY_STRIDE_PARAM, 1.f, 8.f, 1.f, "Stride");
		getParamQuantity(KEY_STRIDE_PARAM)->snapEnabled = true;
		configParam(MAINTAIN_CONTEXT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Keys Off Mode");
		configParam(OBEY_PARAM, 0.f, 1.f, 1.f, "Obey");
		configParam(OCTAVE_PERFECT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Octave Includes Perfect");
		configParam(PITCH_A_PARAM, 0.f, 1.f, 0.f, "A");
		configParam(PITCH_A_SHARP_PARAM, 0.f, 1.f, 0.f, "A#");
		configParam(PITCH_B_PARAM, 0.f, 1.f, 0.f, "B");
		configParam(PITCH_C_PARAM, 0.f, 1.f, 0.f, "C");
		configParam(PITCH_C_SHARP_PARAM, 0.f, 1.f, 0.f, "C#");
		configParam(PITCH_D_PARAM, 0.f, 1.f, 0.f, "D");
		configParam(PITCH_D_SHARP_PARAM, 0.f, 1.f, 0.f, "D#");
		configParam(PITCH_E_PARAM, 0.f, 1.f, 0.f, "E");
		configParam(PITCH_F_PARAM, 0.f, 1.f, 0.f, "F");
		configParam(PITCH_F_SHARP_PARAM, 0.f, 1.f, 0.f, "F#");
		configParam(PITCH_G_PARAM, 0.f, 1.f, 0.f, "G");
		configParam(PITCH_G_SHARP_PARAM, 0.f, 1.f, 0.f, "G#");
		configParam<PlayDirectionQuantity>(PLAY_DIRECTION_PARAM, 0.f, LSystemExecuteDirection::kNum_EXECUTE_DIRECTIONS - 1, LSystemExecuteDirection::DIRECTION_FORWARD, "Play Direction");
		getParamQuantity(PLAY_DIRECTION_PARAM)->snapEnabled = true;
		configParam(RANDOMIZE_NOTES_BUTTON_PARAM, 0.f, 1.f, 0.f, "Randomize Notes");
		configParam(RESET_BUTTON_PARAM, 0.f, 1.f, 0.f, "Reset");
		configParam<RootPitchQuantity>(ROOT_PITCH_PARAM, 0.f, 120.f, 60.f, "Root Pitch");  // midi range, 0..120, C4 = 60
		getParamQuantity(ROOT_PITCH_PARAM)->snapEnabled = true;
		configParam(RUN_BUTTON_PARAM, 0.f, 1.f, 0.f, "Run"); // TODO: see Foundation LFO.cpp offset. configSwitch()
		configParam<SemitoneStrategyQuantity>(SEMITONE_OVERSHOOT_STRATEGY_PARAM, 0.f, StepperAlgo::kNum_STEPPER_ALGOS - 1, StepperAlgo::STEPPER_ALGO_CYCLE_UP, "Semitone Overshoot"); // TODO: remane SemitomeBoundsStrategy 
		getParamQuantity(SEMITONE_OVERSHOOT_STRATEGY_PARAM)->snapEnabled = true;
		configInput(ARP_ALGORITHM_CV_INPUT, "Arp Algo");
		configInput(ARP_DELAY_CV_INPUT, "Arp Delay");
		configInput(ARP_OCTAVE_MAX_CV_INPUT, "Arp Octave Max");
		configInput(ARP_OCTAVE_MIN_CV_INPUT, "Arp Octave Min");
		configInput(ARP_PROBABILITY_CV_INPUT, "Arp Probability");
		configInput(CLOCK_CV_INPUT, "Clock");
		configInput(DELAY_TIMING_CV_INPUT, "Delay Swing");
		configInput(DOUBLE_TIME_CV_INPUT, "Double Time");
		configInput(HARMONY_1_DELAY_CV_INPUT, "Harmony 1 Delay");
		configInput(HARMONY_2_DELAY_CV_INPUT, "Harmony 2 Delay");
		configInput(HARMONY_3_DELAY_CV_INPUT, "Harmony 3 Delay");
		configInput(HARMONY_OCTAVE_MAX_CV_INPUT, "Harmony Octave Max");
		configInput(HARMONY_OCTAVE_MIN_CV_INPUT, "Harmony Octave Min");
		configInput(HARMONY_OVERSHOOT_STRATEGY_CV_INPUT, "Harmony Algo");
		configInput(HARMONY_PROBABILITY_CV_INPUT, "Harmony Probability");
		configInput(INTERVAL_STEP_SIZE_CV_INPUT, "Harmony Interval Step Size");
		configInput(KEY_ORDER_CV_INPUT, "Input Note Order");
		configInput(KEY_STRIDE_CV_INPUT, "Key Stride");
		configInput(OBEY_CV_INPUT, "Obey Probability");
		configInput(OCTAVE_PERFECT_CV_INPUT, "Octave Includes Perfect");
		configInput(PLAY_DIRECTION_CV_INPUT, "Play Direction");
		configInput(POLY_GATE_IN_INPUT, "Poly Gate");
		configInput(POLY_IN_CV_INPUT, "Poly V/Oct");
		configInput(POLY_SCALE_CV_INPUT, "Poly Scale");
		configInput(RANDOMIZE_NOTES_CV_INPUT, "Randomize Notes");
		configInput(RESET_CV_INPUT, "Reset");
		configInput(ROOT_PITCH_CV_INPUT, "Root Pitch");
		configInput(RUN_CV_INPUT, "Run");
		configInput(SEMITONE_OVERSHOOT_STRATEGY_CV_INPUT, "Semitone Algo");
		configOutput(CLOCK_OUTPUT, "Clock");
		configOutput(END_OF_CYCLE_OUTPUT, "End of Cycle");
		configOutput(GATE_FOUR_OUTPUT, "Harmony 3 Gate");
		configOutput(GATE_ONE_OUTPUT, "Arp Note Gate");
		configOutput(GATE_POLY_OUTPUT, "Poly Gate");
		configOutput(GATE_TWO_OUTPUT, "Harmony 1 Gate");
		configOutput(GATE_THREE_OUTPUT, "Harmony 2 Gate");
		configOutput(RUN_OUTPUT, "Run");
		configOutput(VOCT_FOUR_OUTPUT, "Harmony 3 V/Oct");
		configOutput(VOCT_ONE_OUTPUT, "Arp Note V/Oct");
		configOutput(VOCT_POLY_OUTPUT, "Poly V/Oct");
		configOutput(VOCT_THREE_OUTPUT, "Harmony 2 V/Oct");
		configOutput(VOCT_TWO_OUTPUT, "Harmony 1 V/Oct");
		configBypass(POLY_IN_CV_INPUT, VOCT_POLY_OUTPUT); // TODO: does not seem to pass poly signals in bypass ?? 
		configBypass(POLY_GATE_IN_INPUT, GATE_POLY_OUTPUT); // TODO: does not seem to pass poly signals in bypass ?? 

		clock.setSampleRate(APP->engine->getSampleRate());
		clock.setBeatsPerMinute(DFLT_BPM);

		mArpPlayer.setRootPitch(60); // TODO: const C4
		setActiveScale(0); // major // TODO: find scale by name 
		captureScaleEnableStates();

		mOutputNoteWriter.pReceiver = this;


		delayRedrawRequired = true;
		scaleRedrawRequired = true;
		arpPlayerRedrawRequired = true;		
	}

	// void test_stepper(StepperAlgo algo, int minVal, int maxVal) {
	// 	DEBUG("Stepper Test: ---------------------------  algo %s (%d), min %d, max %d", stepperAlgoNames[algo], algo, minVal, maxVal);
	// 	Stepper stepper(minVal, maxVal);
	// 	stepper.setAlgorithm(algo);
	// 	DEBUG("  initial value is: %d", stepper.getValue());
	// 	int numSteps = (maxVal - minVal) + 1;
	// 	numSteps *= 2; 
	// 	for (int i = 0; i < numSteps; i++) {
	// 		DEBUG("  forward-1 [%2d]   val = %d", i, stepper.forward());
	// 	}
	// 	for (int i = 0; i < numSteps; i++) {
	// 		DEBUG("  reverse [%2d]   val = %d", i, stepper.reverse());
	// 	}
	// 	for (int i = 0; i < numSteps; i++) {
	// 		DEBUG("  forward-2 [%2d]   val = %d", i, stepper.forward());
	// 	}
	// }

	// TODO: add destructor ?? or dtor-like method to drain the memory pools on exit


	void process(const ProcessArgs& args) override {

// static bool isFirst = true; 
// if (isFirst) { // TODO: remove 
// 	DEBUG("ARP GEN: in process() .. ");
// 	// DEBUG("systemDir is %s", asset::systemDir.c_str());
// 	// DEBUG("userDir is %s", asset::userDir.c_str());
// 	// DEBUG("asset::system is: %s", asset::system("sys.txt").c_str());
// 	// DEBUG("asset::user is: %s", asset::user("use.txt").c_str());
// 	// DEBUG("asset::pluginInstance is: %s", asset::plugin(pluginInstance, "arprules").c_str());

// 		// STEPPER test
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_CYCLE_UP, 1,5);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_CYCLE_DOWN, 1,5);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_REFLECT, 1, 5);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_REFLECT_LINGER, 1, 5);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_STAIRS_UP, 1, 5);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_STAIRS_DOWN, 1, 5);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_CONVERGE, 3, 8);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_DIVERGE, 3, 8);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_DIVERGE, 4, 8);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_PINKY_UP, 3, 8);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_PINKY_DOWN, 3, 8);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_THUMB_UP, 3, 8);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_THUMB_DOWN, 3, 8);
// 		// test_stepper(StepperAlgo::STEPPER_ALGO_RANDOM, -24, 24);
// 		isFirst = false;
// 	}

		// Reset
		if (mResetButtonTrigger.process(params[RESET_BUTTON_PARAM].getValue()) | mResetTrigger.process(inputs[RESET_CV_INPUT].getVoltage(), 0.1f, 2.f)) {
			mResetPulse.trigger(1e-3f);
			mArpPlayer.resetScanToBeginning();
		}

		bool resetGate = mResetPulse.process(args.sampleTime);

		// Clock
		// Ignore the external clock just after a reset occurred 
		if (inputs[CLOCK_CV_INPUT].isConnected() && (! resetGate)) {
			clock.tick_external(inputs[CLOCK_CV_INPUT].getVoltage());
		} else {
			clock.tick_internal();
		}

		if (inputs[RUN_CV_INPUT].isConnected()) {
			mRunning = inputs[RUN_CV_INPUT].getVoltage() > 0.5f;			
		} else { 
			if (mRunButtonTrigger.process(params[RUN_BUTTON_PARAM].getValue())) {
				mRunning ^= true;
				// runPulse.trigger(1e-3f);  // TODO  see Fundamental/SEQ.cpp 
			}
		}

		// TODO: this widget acts a bit different than the rest
		// the others accept the button input only if the CV is not connected
		// i.e. cv connection disables ui button push -- ok to diverge from that strategy here?
		if (mKeysOffTrigger.process(params[MAINTAIN_CONTEXT_BUTTON_PARAM].getValue())) {
			int strat = mKeysOffStrategy; 
			strat = (strat + 1) % KeysOffStrategy::kNUM_KEYS_OFF_STRATEGIES;
			mKeysOffStrategy = (KeysOffStrategy) strat;
			keysOffStrategyRedrawRequired = true;
		}

		mIsDoubleTime = (mTwinkleCount > 0 || (inputs[DOUBLE_TIME_CV_INPUT].getVoltage() > 0.f));

		// time to produce output? 
		bool emitNotes = (clock.isLeadingEdge() || (mIsDoubleTime && clock.isTrailingEdge())); 

		mParameterPollCount--;
		if (emitNotes || mParameterPollCount < 0) {
			mParameterPollCount = PARAMETER_POLL_SAMPLES; // TODO: adjust this for time according to the current sample rate 

			// TODO: move to processDoubleTimeChanges()
			mTwinkleSteps = minTwinkleSteps + round(params[DOUBLE_TIME_COUNT_PARAM].getValue() * (maxTwinkleSteps - minTwinkleSteps));
			if (mTwinkleTrigger.process(params[DOUBLE_TIME_BUTTON_PARAM].getValue() + inputs[DOUBLE_TIME_CV_INPUT].getVoltage())) {
				if (mTwinkleCount == 0) {
					mTwinkleCount = mTwinkleSteps;
				}
			}

			processStrideChanges();
			processScaleChanges();
			processDelayChanges();
			processLSystemExpansion();
			processOctaveRangeChanges(); 
			processPlayerConfigurationChanges();
			processHarmonyRandomization();
			receiveInputs();
		}

		if (emitNotes) {
			if (mRunning) {
				stepArpPlayer();
				if (mArpPlayer.isEndOfSequence()) {
					mEocPulseGenerator.trigger(1e-3f);
				}
			} 
			if (mTwinkleCount > 0) {
				mTwinkleCount--;
			}
		}

		produceOutput();

		outputs[END_OF_CYCLE_OUTPUT].setVoltage(mEocPulseGenerator.process(args.sampleTime) ? 10.f : 0.f);
		outputs[CLOCK_OUTPUT].setVoltage(clock.isHigh() ? 10.f : 0.f);

		// always draw or refresh the delay selector switches 
		updateDelaySwitches();

		if (scaleRedrawRequired) {

			// DEBUG("==== Scale Redraw:");
			// showScaleEnabledStates();

			for (int i = 0; i < 12; i++) {  // TODO const 
				// DEBUG("Scale redraw: button %2d, enabled = %d, scale.degree enabled %d, scale.rel to C %d", 
				// 	i, 
				// 	scaleButtonEnabled[i],
				// 	arpPlayer.getTwelveToneScale().isPitchEnabled(i),
				// 	arpPlayer.getTwelveToneScale().isDregreeRelativeToCEnabled(i));
				// DEBUG("  setting scale light %d, (led idx %d) to %d", i, scaleButtons[i].mLightIdx, arpPlayer.getTwelveToneScale().isDregreeRelativeToCEnabled(i));

				lights[ scaleButtons[i].mLightIdx ].setBrightness(scaleButtonEnabled[i]);
			}
			scaleRedrawRequired = false;
		}

		if (arpPlayerRedrawRequired) {
			// DEBUG("ArpPlayer needs redraw .. STUBBED .. fix me"); // TODO: implement this
			arpPlayerRedrawRequired ^= true;
		}

		lights[DOUBLE_TIME_LED_LIGHT].setBrightness(mIsDoubleTime);
		lights[OCTAVE_PERFECT_LED_LIGHT].setBrightness(mOctaveIncludesPerfect);
		lights[RESET_LED_LIGHT].setSmoothBrightness(resetGate, args.sampleTime);
		lights[RUN_LED_LIGHT].setBrightness(mRunning);
		lights[DIRECTION_FORWARD_LED_LIGHT].setBrightness(mArpPlayer.isScanningForward());
		lights[DIRECTION_REVERSE_LED_LIGHT].setBrightness(mArpPlayer.isScanningReverse());
		lights[MAINTAIN_CONTEXT_LED_LIGHT].setBrightness(mKeysOffStrategy == KeysOffStrategy::KEYS_OFF_MAINTAIN_CONTEXT);
		
		lights[RANDOMIZE_NOTES_LED_LIGHT].setSmoothBrightness(mRandomizeHarmoniesTrigger.isHigh(), args.sampleRate / 32); // TODO: const
		lights[KEY_ORDER_LOW_TO_HIGH_LED_LIGHT].setBrightness( mArpPlayerSortingOrder == SortingOrder::SORT_ORDER_LOW_TO_HIGH);
		lights[KEY_ORDER_HIGH_TO_LOW_LED_LIGHT].setBrightness( mArpPlayerSortingOrder == SortingOrder::SORT_ORDER_HIGH_TO_LOW);
		lights[KEY_ORDER_AS_RECEIVED_LED_LIGHT].setBrightness( mArpPlayerSortingOrder == SortingOrder::SORT_ORDER_AS_RECEIVED);

		bool isPlayable = mpExpandedTermSequence != 0 && mTermCounter.hasNoteActions();
		lights[RULES_VALID_LED_LIGHT + 0].setBrightness(isPlayable);
		lights[RULES_VALID_LED_LIGHT + 1].setBrightness(! isPlayable);

		outputs[RUN_OUTPUT].setVoltage(mRunning ? 10.f : 0.f);

static int delayPeriod = 0; // TODO move this 
static int delayTime = 0; // TODO move this 

		if (emitNotes) {
			delayPeriod = 0;
			float swingFactor = 0.25f + (mSwingAmount * ((1.f/3.f) - 0.25f));
			delayTime = int(clock.getSamplesPerBeat() * swingFactor);
			if (mIsDoubleTime) {
				delayTime /= 2;
			}
			// DEBUG("Delay tracers: samplesPerBeat = %f", clock.getSamplesPerBeat());
			// DEBUG("Delay tracers: swing amount = %f", mSwingAmount);
			// DEBUG("Delay tracers: delayTime = %d", delayTime);
		}

		delayTime --;
		if (delayTime <= 0) {
			delayPeriod++;
			if (delayPeriod >= 4) {
				delayPeriod = 0;
			}
			float swingFactor = 0.25f + (mSwingAmount * ((1.f/3.f) - 0.25f));
			delayTime = int(clock.getSamplesPerBeat() * swingFactor);
			if (mIsDoubleTime) {
				delayTime /= 2;
			}
		}

		lights[ARP_DELAY_0_LED_LIGHT].setBrightness(mRunning && delayPeriod == 0);
		lights[ARP_DELAY_1_LED_LIGHT].setBrightness(mRunning && delayPeriod == 1);
		lights[ARP_DELAY_2_LED_LIGHT].setBrightness(mRunning && delayPeriod == 2);
		lights[ARP_DELAY_3_LED_LIGHT].setBrightness(mRunning && delayPeriod == 3);

		// lights[HARMONY_1_DELAY_0_LED_LIGHT].setBrightness(delayPeriod == 0);
		// lights[HARMONY_1_DELAY_1_LED_LIGHT].setBrightness(delayPeriod == 1);
		// lights[HARMONY_1_DELAY_2_LED_LIGHT].setBrightness(delayPeriod == 2);
		// lights[HARMONY_1_DELAY_3_LED_LIGHT].setBrightness(delayPeriod == 3);

		// lights[HARMONY_2_DELAY_0_LED_LIGHT].setBrightness(delayPeriod == 0);
		// lights[HARMONY_2_DELAY_1_LED_LIGHT].setBrightness(delayPeriod == 1);
		// lights[HARMONY_2_DELAY_2_LED_LIGHT].setBrightness(delayPeriod == 2);
		// lights[HARMONY_2_DELAY_3_LED_LIGHT].setBrightness(delayPeriod == 3);

		// // delayPercentageOptions[0] = 0.f; 
		// lights[HARMONY_3_DELAY_0_LED_LIGHT].setBrightness(delayPeriod == 0);
		// lights[HARMONY_3_DELAY_1_LED_LIGHT].setBrightness(delayPeriod == 1);
		// lights[HARMONY_3_DELAY_2_LED_LIGHT].setBrightness(delayPeriod == 2);
		// lights[HARMONY_3_DELAY_3_LED_LIGHT].setBrightness(delayPeriod == 3);

		// delayPercentageOptions[1] = 0.25f + (swingAmount * ((1.f/3.f) - 0.25f)); 
		// delayPercentageOptions[2] = 0.50f + (swingAmount * ((2.f/3.f) - 0.50f)); 
		// delayPercentageOptions[3] = 0.75f + (swingAmount * ((3.f/3.f) - 0.75f)); // TODO: wrap to 0 ? 
		// ARP_DELAY_0_LED_LIGHT,
		// ARP_DELAY_1_LED_LIGHT,
		// ARP_DELAY_2_LED_LIGHT,
		// ARP_DELAY_3_LED_LIGHT,
		// HARMONY_1_DELAY_0_LED_LIGHT,
		// HARMONY_1_DELAY_1_LED_LIGHT,
		// HARMONY_1_DELAY_2_LED_LIGHT,
		// HARMONY_1_DELAY_3_LED_LIGHT,
		// HARMONY_2_DELAY_0_LED_LIGHT,
		// HARMONY_2_DELAY_1_LED_LIGHT,
		// HARMONY_2_DELAY_2_LED_LIGHT,
		// HARMONY_2_DELAY_3_LED_LIGHT,
		// HARMONY_3_DELAY_0_LED_LIGHT,
		// HARMONY_3_DELAY_1_LED_LIGHT,
		// HARMONY_3_DELAY_2_LED_LIGHT,
		// HARMONY_3_DELAY_3_LED_LIGHT,




// 		outputs[CLOCK_OUTPUT].setVoltage(clock.isHigh() ? 10.f : 0.f);      // TODO: clock tracks high/low 
			
		// if (keysOffStrategyRedrawRequired) {
		// 	for (int i = 0; i < KeysOffStrategy::kNUM_KEYS_OFF_STRATEGIES; i++) {
		// 		lights[ keysOffStrategiesLightIds[i] ].setBrightness(i == mKeysOffStrategy);
		// 	}
		// 	keysOffStrategyRedrawRequired = false;
		// }

		// TODO: do this only if update required 
		// lights[ROOT_OCTAVE_MIN_MAX_FOLLOW_LED_LIGHT + 0].setBrightness(mOctaveInterlockRoot == InterlockState::INTERLOCK_OFF);
		// lights[ROOT_OCTAVE_MIN_MAX_FOLLOW_LED_LIGHT + 1].setBrightness(mOctaveInterlockRoot == InterlockState::INTERLOCK_NORMALED);
		// lights[ROOT_OCTAVE_MIN_MAX_FOLLOW_LED_LIGHT + 2].setBrightness(mOctaveInterlockRoot == InterlockState::INTERLOCK_NORMALED_INVERTED);

		// lights[HARMONY_OCTAVE_MIN_MAX_FOLLOW_LED_LIGHT + 0].setBrightness(mOctaveInterlockHarmony != InterlockState::INTERLOCK_OFF); 
		// lights[HARMONY_OCTAVE_MIN_MAX_FOLLOW_LED_LIGHT + 1].setBrightness(mOctaveInterlockHarmony != InterlockState::INTERLOCK_NORMALED); 
		// lights[HARMONY_OCTAVE_MIN_MAX_FOLLOW_LED_LIGHT + 2].setBrightness(mOctaveInterlockHarmony != InterlockState::INTERLOCK_NORMALED_INVERTED); 

		// lights[ROOT_HARMONY_OCTAVE_FOLLOW_LED_LIGHT + 0].setBrightness(mOctaveInterlockBoth != InterlockState::INTERLOCK_OFF);
		// lights[ROOT_HARMONY_OCTAVE_FOLLOW_LED_LIGHT + 1].setBrightness(mOctaveInterlockBoth != InterlockState::INTERLOCK_NORMALED);
		// lights[ROOT_HARMONY_OCTAVE_FOLLOW_LED_LIGHT + 2].setBrightness(mOctaveInterlockBoth != InterlockState::INTERLOCK_NORMALED_INVERTED);

	}

	void onReset() override	
	{
		// TODO: see examples in Fundamental/SEQ.cpp about handling reset, run state, and clocks, etc.

		mRunTrigger.reset(); 
		mRunButtonTrigger.reset(); 
		mKeysOffTrigger.reset();
		mOctaveInterlockBothTrigger.reset(); 
		mOctaveInterlockRootTrigger.reset(); 
		mOctaveInterlockHarmonyTrigger.reset(); 
		mPerfectOctaveTrigger.reset();
		mRandomizeHarmoniesTrigger.reset();
		mResetButtonTrigger.reset();
		mResetTrigger.reset(); 
		mSortingOrderTrigger.reset(); 

		for (int i = 0; i < 12; i++) {  // TODO const 
			scaleButtonTriggers[i].reset();
		}

		for (int i = 0; i < NUM_DELAYS; i++) { // TODO: const, num delays
			for (int s = 0; s < NUM_DELAY_SLOTS; s++) {
				delayTriggers[i][s].reset(); 
			}
		}

		// TODO: reset triggers 
		// TODO: put play cursor at position 0 
	}
	
	void onSampleRateChange() override {
		clock.setSampleRate(APP->engine->getSampleRate());
	}

	virtual json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_object_set_new(rootJ, "sortingOrder", json_integer(mArpPlayerSortingOrder));

		json_object_set_new(rootJ, "scaleIndex", json_integer(activeScaleIdx));

		json_t* enabledNotes = json_array();
		for (int i = 0; i < 12; i++) {
			json_array_insert_new(enabledNotes, i, json_boolean(scaleButtonEnabled[i]));
		}
		json_object_set_new(rootJ, "enabledNotes", enabledNotes);

		json_t* delaySelectionsJ = json_array();
		for (int i = 0; i < NUM_DELAYS; i++) {
			json_array_insert_new(delaySelectionsJ, i, json_integer(delaySelected[i]));
		}
		json_object_set_new(rootJ, "delaySelections", delaySelectionsJ);

		json_object_set_new(rootJ, "octaveIncludesPerfect", json_integer(mOctaveIncludesPerfect));

		json_object_set_new(rootJ, "rulesFilepath", json_string(mRulesFilepath.c_str()));

        return rootJ;
	}

	virtual void dataFromJson(json_t* rootJ) override {
		json_t* jsonValue;

        jsonValue = json_object_get(rootJ, "sortingOrder"); 
		if (jsonValue) {
			mArpPlayerSortingOrder = (SortingOrder) json_integer_value(jsonValue);
        }

        jsonValue = json_object_get(rootJ, "scaleIndex"); // TODO: change this to use a string name
		if (jsonValue) {
			activeScaleIdx = json_integer_value(jsonValue);
			setActiveScale(activeScaleIdx);
			scaleRedrawRequired = true;
        }

		json_t* enabledNotes = json_object_get(rootJ, "enabledNotes");
		if (enabledNotes) {
			for (int i = 0; i < 12; i++) {
				json_t* enabledNote = json_array_get(enabledNotes, i);
				if (enabledNote) {
					scaleButtonEnabled[i] = json_boolean_value(enabledNote);
					// DEBUG("JSON: scaleButtonEnabled[%d] = %d", i, scaleButtonEnabled[i]);
				}
			}
			setScaleFromUi();
			scaleRedrawRequired = true;
		}

		json_t* delaySelectionsJ = json_object_get(rootJ, "delaySelections");
		if (delaySelectionsJ) {
			for (int i = 0; i < NUM_DELAYS; i++) {
				json_t* delaySelectionJ = json_array_get(delaySelectionsJ, i);
				if (delaySelectionJ) {
					delaySelected[i] = json_integer_value(delaySelectionJ);
				}
			}
			delayRedrawRequired = true;
		}

        jsonValue = json_object_get(rootJ, "octaveIncludesPerfect");
		if (jsonValue) {
			mOctaveIncludesPerfect = json_integer_value(jsonValue);
			mArpPlayer.setOctaveIncludesPerfect(mOctaveIncludesPerfect);
        }

        jsonValue = json_object_get(rootJ, "rulesFilepath");
		if (jsonValue) {
				mRulesFilepath = json_string_value(jsonValue);
				if (!mRulesFilepath.empty()) {
					loadRulesFromFile(mRulesFilepath);
				}
		}

	}


	void processStrideChanges() {
		int stride = params[KEY_STRIDE_PARAM].getValue();  // 1..8 
		if (stride != mStride) {
			mStride = stride;
			mArpPlayer.setNoteStride(stride);
		}
	}

	void processLSystemExpansion() {
		int newDepth = int(params[ EXPANSION_DEPTH_PARAM ].getValue());
		if (newDepth != mExpansionDepth) {

			// DEBUG(" EXPANSION Depth changed: was %d, is %d", mExpansionDepth, newDepth);

			// TODO: apply some HYSTERESIS to let the new value settle before doing all the  work to repace the terms 

			mExpansionDepth = newDepth; 
			
			// log_lsystem(mpLSystemPending, "DepthChange: Pending");
			// log_lsystem(mpLSystemActive,  "DepthChange: Active");
 			
			expandLSystemSequence(mpLSystemActive);
		}
	}

	// incoming val is 0..1
	StepperAlgo getStepperStyle(float valZeroToOne) {
		int style = valZeroToOne;
		return (StepperAlgo) style;
	}


	SortingOrder incrementSortingOrder() {
		int order = mArpPlayerSortingOrder + 1;
		if (order >= SortingOrder::kNUM_SORTING_ORDER) {
			order = 0;
		}
		return (SortingOrder) order;
	}

	// TODO: delete 
	// // incoming val is 0..1
	// SortingOrder getSortingOrder(float valZeroToOne) {
	// 	int strategy = round(valZeroToOne * float(SortingOrder::kNUM_SORTING_ORDER - 1));
	// 	return (SortingOrder) strategy;
	// }

	// incoming val is 0..1
	ScaleOutlierStrategy getScaleOutlierStrategy(float val) {
//		float segmentSize = 1.f/3.f; // TODO: const, 
//		int strategy = clamp(int(val / segmentSize), 0, 1);

		// TODO .. remove divide
		// if segment size is 1/3 and result is val/segment size,
		// that should be the same as round(val * 3)

		int strategy = round(val * 3.f);
		strategy = clamp(strategy,0,2);
		return (ScaleOutlierStrategy) strategy;
	}

	LSystemExecuteDirection getPlayDirection(float val) {
		return (LSystemExecuteDirection) round(val);
	}

	void processPlayerConfigurationChanges() {
		// direction

		LSystemExecuteDirection  direction; 
		if (inputs[PLAY_DIRECTION_CV_INPUT].isConnected()) {
			direction = getPlayDirection((inputs[PLAY_DIRECTION_CV_INPUT].getVoltage() + 5.f) * 0.2f); // -5..5 => 0..2
		} else {
			direction = getPlayDirection(params[PLAY_DIRECTION_PARAM].getValue()); 
		}

		if (direction != mPlayDirection) {
			mPlayDirection = direction;
			mArpPlayer.setExecuteDirection(direction);
		}

		if (inputs[ ARP_PROBABILITY_CV_INPUT ].isConnected()) {
			mNoteProbability = inputs[ ARP_PROBABILITY_CV_INPUT ].getVoltage() / 10.f; // 0..10 
		} else {
			mNoteProbability = params[ ARP_PROBABILITY_PARAM ].getValue();
		}

		if (inputs[ HARMONY_PROBABILITY_CV_INPUT ].isConnected()) {
			mHarmonyProbability = inputs[ HARMONY_PROBABILITY_CV_INPUT ].getVoltage() / 10.f; // 0..10 ==> 1..0 
		} else {
			mHarmonyProbability = params[ HARMONY_PROBABILITY_PARAM ].getValue();
		}

		float factor = float(StepperAlgo::kNum_STEPPER_ALGOS - 1) * 0.1f;
		if (inputs[ARP_ALGORITHM_CV_INPUT].isConnected()) {
			mArpPlayerArpStyle = getStepperStyle(inputs[ARP_ALGORITHM_CV_INPUT].getVoltage() * factor); // 0..10 
		} else {
			mArpPlayerArpStyle = getStepperStyle(params[ARP_ALGORITHM_PARAM].getValue()); 
		}

		if (inputs[HARMONY_OVERSHOOT_STRATEGY_CV_INPUT].isConnected()) {
			mHarmonyIntervalStepperStrategy = getStepperStyle(inputs[HARMONY_OVERSHOOT_STRATEGY_CV_INPUT].getVoltage() * factor); // 0..10
		} else {
			mHarmonyIntervalStepperStrategy = getStepperStyle(params[HARMONY_OVERSHOOT_STRATEGY_PARAM].getValue()); 
		}

		if (inputs[SEMITONE_OVERSHOOT_STRATEGY_CV_INPUT].isConnected()) {
			mSemitoneIntervalStepperStrategy = getStepperStyle(inputs[SEMITONE_OVERSHOOT_STRATEGY_CV_INPUT].getVoltage() * factor);
		} else {
			mSemitoneIntervalStepperStrategy = getStepperStyle(params[SEMITONE_OVERSHOOT_STRATEGY_PARAM].getValue()); 
		}

		if (inputs[KEY_ORDER_CV_INPUT].isConnected()) {
			mArpPlayerSortingOrder = (SortingOrder) ((inputs[KEY_ORDER_CV_INPUT].getVoltage() + 5.f) * 0.1f * (SortingOrder::kNUM_SORTING_ORDER)); // -5..5 ==> 0..2
		} else if (mSortingOrderTrigger.process(params[KEY_ORDER_BUTTON_PARAM].getValue())) {
			mArpPlayerSortingOrder = incrementSortingOrder();
		}

		int intervalStepSize;
		if (inputs[INTERVAL_STEP_SIZE_CV_INPUT].isConnected()) {
			intervalStepSize = (inputs[INTERVAL_STEP_SIZE_CV_INPUT].getVoltage() + 5.f) * 8.f;  // -5..5  ==> 1..8 // TODO: const
		} else { 
			intervalStepSize = params[INTERVAL_STEP_SIZE_PARAM].getValue();
		}
		if (intervalStepSize != mIntervalStepSize) {
			// DEBUG("Interval Step size changed: was %d, is %d", mIntervalStepSize, intervalStepSize);
			mIntervalStepSize = intervalStepSize;
			mArpPlayer.setIntervalStepSize(intervalStepSize);
		}
		// TODO: make this a global
		// only set play if changed 
		// this sets semitone and harmony step sizes .. how to set Arp Note increment size? 
	}

	// struct IntegerRange {
	// 	int minValue = 0;
	// 	int maxValue = 0;
	// 	void set(int minVal, int maxVal) { 
	// 		if (minVal <= maxVal) {
	// 			minValue = minVal;
	// 			maxValue = maxVal;
	// 		} else { // swap 
	// 			minValue = maxVal;
	// 			maxValue = minVal;
	// 		}
	// 	}
	// }; 
	// IntegerRange rootOctaveRange; 
	// IntegerRange harmonyOctaveRange; 


	// struct LockableParams {
	// 	float param_1_val; 
	// 	float param_2_val;
	// 	float param_1_delta; 
	// 	float param_2_delta; 
	// 	bool isLocked; 

	// 	void scratch(float param_1_current, float param_2_current) { 
	// 		param_1_delta = param_1_current - param_1_val;
	// 		param_2_delta = param_2_current - param_2_val;
	// 		if (isLocked) {
	// 			float combined_delta = param_1_delta + param_2_delta;
	// 			param_1_delta = combined_delta;
	// 			param_2_delta = combined_delta;
	// 		}
	// 		param_1_val += param_1_delta; // TODOL: clamp? 
	// 		param_2_val += param_2_delta; // TODOL: clamp? 
	// 	}
	// };

	// LockableParams rootOctaveParams;
	// LockableParams harmonyOctaveParams;

/***
 * read all 4 octave values into effective_value[4] array 
 * if all octaves are locked 
 *    compute the deltas for each param
 *    combined_delta = add up all the deltas
 *    for each param
 *       effective value[i] += combined_delta
 * else 
 *     if root octaves are locked
 *        do same thing just for the root octave param pair 
 *     if harmony octaves are locked
 *        do same thing just for the harmony octave param pair 
 * 
 *  for each param 
 *    compute octave range for param based on effective value[i]
 * 
 * 
 * */	

	// TODO take out all the interlock stuff
	// use external control voltage to do that 

	// float rootOctaveMin_prev = 0.f;
	// float rootOctaveMax_prev = 0.f;

	// float harmonyOctaveMin_prev = 0.f;
	// float harmonyOctaveMax_prev = 0.f;

	void processOctaveRangeChanges() {

		float arpOctaveMin = params[ARP_OCTAVE_MIN_PARAM].getValue(); // +/- 1
		float arpOctaveMax = params[ARP_OCTAVE_MAX_PARAM].getValue();  // +/- 1

		float harmonyOctaveMin = params[HARMONY_OCTAVE_MIN_PARAM].getValue(); // +/- 1
		float harmonyOctaveMax = params[HARMONY_OCTAVE_MAX_PARAM].getValue(); // +/- 1

		// Octave CV is +/- 5 volts
		if (inputs[ARP_OCTAVE_MIN_CV_INPUT].isConnected()) {
			arpOctaveMin = inputs[ARP_OCTAVE_MIN_CV_INPUT].getVoltage() * 0.2f; // -5..5 ==> -1..1
		} 
		if (inputs[ARP_OCTAVE_MAX_CV_INPUT].isConnected()) {
			arpOctaveMax = inputs[ARP_OCTAVE_MAX_CV_INPUT].getVoltage() * 0.2f; // +/- 5 => -1..1
		} 
		if (inputs[HARMONY_OCTAVE_MIN_CV_INPUT].isConnected()) {
			harmonyOctaveMin = inputs[HARMONY_OCTAVE_MIN_CV_INPUT].getVoltage() * 0.2f;  // -5..5 ==> -1..1
		} 
		if (inputs[HARMONY_OCTAVE_MAX_CV_INPUT].isConnected()) {
			harmonyOctaveMax = inputs[HARMONY_OCTAVE_MAX_CV_INPUT].getVoltage() * 02.f; // -5..5 ==> -1..1
		} 

		// Perfect Octave 
		// TODO: add CV 
		if (mPerfectOctaveTrigger.process(params[OCTAVE_PERFECT_BUTTON_PARAM].getValue())) {
			mOctaveIncludesPerfect = (! mOctaveIncludesPerfect);
			mArpPlayer.setOctaveIncludesPerfect(mOctaveIncludesPerfect);
		}

// TODO: do this only of the values changed 
// arp note 

static int counter = 0;
bool verbose = false;
counter--;
if (counter <= 0) {  // TODO: remove 
/// 	verbose = true;
	counter = 48000 * 2;
}

		float octaveRange = 2.f;

// Arp Octaves 
		if (verbose) { 
			DEBUG("process octaves: ARP octaveMin %f", arpOctaveMin);
			DEBUG("process octaves: ARP octaveMax %f", arpOctaveMax);
		}

		int octaveMin = round(arpOctaveMin * octaveRange);		
		int octaveMax = round(arpOctaveMax * octaveRange);		

		if (octaveMin > octaveMax) {
			octaveMin = octaveMax; // limit min to be <= max 

			// int limitedMin = std::min(mArpOctaveMin, mArpOctaveMax);
			// int limitedMax = std::max(mArpOctaveMin, mArpOctaveMax);
			// mArpOctaveMin = limitedMin;
			// mArpOctaveMax = limitedMax;
			
			// int temp = mArpOctaveMin; // swap 
			// mArpOctaveMin = mArpOctaveMax;
			// mArpOctaveMax= temp;
		}

		if (verbose) { 
			DEBUG("process octaves: ARP octaves %d ... %d", octaveMin, octaveMax);
		}

		if (octaveMin != mArpOctaveMin || octaveMax != mArpOctaveMax) {
			mArpOctaveMin = octaveMin;
			mArpOctaveMax = octaveMax;
			// DEBUG("process octaves: ARP changed to: octaves %d ... %d", mArpOctaveMin, mArpOctaveMax);
			mArpPlayer.setArpOctaveRange(mArpOctaveMin, mArpOctaveMax);
		}

// Harmony Octaves
		if (verbose) { 
			DEBUG("process octaves: HARMONY octaveMin %f", harmonyOctaveMin);
			DEBUG("process octaves: HARMONY octaveMax %f", harmonyOctaveMax);
		}

		octaveMin = round(harmonyOctaveMin * octaveRange);		
		octaveMax = round(harmonyOctaveMax * octaveRange);		

		if (octaveMin > octaveMax) {
			octaveMin = octaveMax; // limit min to be <= max 
		}

		if (verbose) { 
			DEBUG("process octaves: HARMONY octaves %d ... %d", octaveMin, octaveMax);
		}

		if (octaveMin != mHarmonyOctaveMin || octaveMax != mHarmonyOctaveMax) {
			mHarmonyOctaveMin = octaveMin;
			mHarmonyOctaveMax = octaveMax;
			mArpPlayer.setHarmonyOctaveRange(mHarmonyOctaveMin, mHarmonyOctaveMax);
		}

	}

	void processDelayChanges() { 

/** experiment **/
		float swing; 
		if (inputs[DELAY_TIMING_CV_INPUT].isConnected()) {
			swing = (inputs[DELAY_TIMING_CV_INPUT].getVoltage() + 5.f) * 0.1f; // -5..5 ==> 0..1
		} else {
			swing = params[DELAY_TIMING_PARAM].getValue();
		}
		if (swing != mSwingAmount) {
			//DEBUG("Swing changed: was %f, is %f", mSwingAmount, swing);
			applySwing(swing);
			// for (int i = 0; i < 4; i++) {
			// 	DEBUG("delay[%d] = %f", i, delayPercentage[i]);
			// }
		}
/** end **/
		// TODO: optimize this so that 1 delay channel is evaluated at a time
		// instead of all 24 switches at every sample

		// TODO: if CVinput is connected, set the delay percentage from that

		for (int i = 0; i < NUM_DELAYS; i++) { 
			for (int s = 0; s < NUM_DELAY_SLOTS; s++) {
				int paramId = delaySelectors[i].delaySwitchIds[s];
				if (delayTriggers[i][s].process( params[paramId].getValue() )) {
					// DEBUG("Delay %d, switch %d, trigger fired, delay selected was %d", i, s, delaySelected[i]);
					if (delaySelected[i] != s) { 
						setDelaySelection(i, s);
					}
					// break; // exit inner loop 
				}
			}
		}

		for (int i = 0; i < NUM_DELAYS; i++) { // TODO: const
			int inputId = delaySelectors[i].inputId;
			if (inputs[inputId].isConnected()) {
				int selected = int(inputs[inputId].getVoltage() + 5.f) * 0.3f; // -5..5 ==> 0..3
				setDelaySelection(i, selected);
			}
		}

	}

	void updateDelaySwitches() { 


		// Always redraw to get rid of the "disappearing switch" problem when 
		// clicking on the already selected switch. The switch state toggles to 0 
		// but the trigger does not react to it, so the switch stays in an "off" position

		// TODO: Could optimize this by having 4 flags, one per delay so that not all leds have to be redrawn when any one changes

		if (delayRedrawRequired) {
			for (int i = 0; i < NUM_DELAYS; i++) { // TODO: const
				for (int switchIdx = 0; switchIdx < NUM_DELAY_SLOTS; switchIdx++) { // TODO: const NUM_DELAY_OPTIONS 
					int paramId = delaySelectors[i].delaySwitchIds[switchIdx];
					params[ paramId ].setValue(delaySelected[i] == switchIdx);
				}
			}
		} else {
			for (int i = 0; i < NUM_DELAYS; i++) { // TODO: const
				int selected = delaySelected[i];
				int paramId = delaySelectors[i].delaySwitchIds[selected];
				params[ paramId ].setValue(1);
			}
		}

		delayRedrawRequired = false;
	}

	void processScaleChanges() {

		float rootVoct;

		// -- Poly scale input 
		if (inputs[POLY_SCALE_CV_INPUT].isConnected()) {
			// TODO: check if voltages changed from last time before updating scale 
			setScaleFromPoly(inputs[POLY_SCALE_CV_INPUT].getChannels(), inputs[POLY_SCALE_CV_INPUT].getVoltages());
			rootVoct = inputs[POLY_SCALE_CV_INPUT].getVoltage(0); // -5...5
		}
		else if (inputs[ROOT_PITCH_CV_INPUT].isConnected()) {
			rootVoct = inputs[ROOT_PITCH_CV_INPUT].getVoltage(); // -5..5
		} else {
			rootVoct = (params[ROOT_PITCH_PARAM].getValue() / 12.f) - 5.f; // 0..120 ==> -5..5
		}

		int rootPitch = voctToPitch(rootVoct);
		if (rootPitch != mArpPlayer.getRootPitch()) {
			// DEBUG("Root Pitch Changed: voct = %f, pitch = %d", rootVoct, rootPitch);
			mArpPlayer.setRootPitch(rootPitch);
			captureScaleEnableStates();
			scaleRedrawRequired = true;			
		}

		// -- UI Scale buttons 
		if (! inputs[POLY_SCALE_CV_INPUT].isConnected()) {
			bool scaleButtonChanged = false;
			for (int i = 0; i < 12; i++) {  // TODO const 
				if (scaleButtonTriggers[i].process( params[ scaleButtons[i].mParamIdx ].getValue())) {
					// DEBUG("*** ScaleButton %d clicked (param %d), was enabled %d", i, scaleButtons[i].mParamIdx, scaleButtonEnabled[i]);
					scaleButtonEnabled[i] = (! scaleButtonEnabled[i]);
					scaleButtonChanged = true;
				}
			}
			if (scaleButtonChanged) {
				setScaleFromUi();
			}
		}
	}

	void processHarmonyRandomization() {
		if (mRandomizeHarmoniesTrigger.process(params[RANDOMIZE_NOTES_BUTTON_PARAM].getValue() + inputs[RANDOMIZE_NOTES_CV_INPUT].getVoltage())) {
			mArpPlayer.randomizeHarmonies();
		}
	}

// TODO: move this ?? 
	struct NoteState { 
		int pitch;
		bool enabled;		
	};

enum GateState { 
	GATE_CLOSED, 
	GATE_LEADING_EDGE,
	GATE_OPEN,
	GATE_TRAILING_EDGE
}; 

GateState gateTransitionTable[2][4] = 
{
	// incoming 
	// voltage     // ------------------- previous gate state ---------------------------------------
	// state       // GATE_CLOSED         GATE_LEADING        GATE_OPEN           GATE_TRAILING
    /* 0 CLOSED */   { GATE_CLOSED,       GATE_TRAILING_EDGE, GATE_TRAILING_EDGE, GATE_CLOSED       },
    /* 1 OPEN   */   { GATE_LEADING_EDGE, GATE_OPEN,          GATE_OPEN,          GATE_LEADING_EDGE },
}; 

struct ChannelNote {
	GateState mGateState = GATE_CLOSED; 
	int  mPitch = 0; 
}; 

ChannelNote channelNotes[16]; // TODO: const
// TODO: init 

// bool pitchChange[16]; // TODO: move ... 

	void receiveInputs() {

//		memset(pitchChange, 0, sizeof(pitchChange)); 

		int numVoctChannels = 0;
		int numGateChannels = 0;

		bool changeDetected = false; // gate or pitch change detected 
		
		// bool verbose = false; 		
		// static int counter = 0;
		// counter--; 
		// if (counter <= 0) {
		// 	counter = 10000;
		// 	// verbose = true;
		// }

		// if (verbose) {
		// 	DEBUG("ReceiveInput()");
		// }

// TODO: optimize: bool ChangeDetected = false;
// compare num notes to the number of notes in the previous run 
// 		int numActiveNotes = arpPlayer.getActiveNoteCount(); 

		// Capture input pitches 
		if (inputs[POLY_IN_CV_INPUT].isConnected()) {
			numVoctChannels = inputs[POLY_IN_CV_INPUT].getChannels();
			// if (verbose) {
			// 	DEBUG("  numVoctChannels %d", numVoctChannels);
			// }
			for (int c = 0; c < numVoctChannels; c++) { 
				int pitch = voctToPitch(inputs[POLY_IN_CV_INPUT].getVoltage(c));
				if (pitch != channelNotes[c].mPitch) {
					changeDetected = true;
				}
//				pitchChange[c] = (pitch != channelNotes[c].mPitch);
				channelNotes[c].mPitch = pitch;
				// if (verbose) {
				// 	DEBUG("  channel[%d] pitch = %d, changed = %d", c, pitch, pitchChange[c]);
				// }
			}
		}


// TODO: !! use the Port::getPolyVoltage(c) method to distribute the gate if monophonic

		if (inputs[POLY_GATE_IN_INPUT].isConnected()) {
			numGateChannels = inputs[POLY_GATE_IN_INPUT].getChannels();
		}

// TODO: consider auto-normaling so that all poly input notes are treeated as if always gated on
// if no cable is connected 

		int numMutualChannels = MIN(numGateChannels, numVoctChannels); 
		// if (verbose) {
		// 	DEBUG("  numGateChannels %d", numGateChannels);
		// 	DEBUG("  numMutualChannels %d", numMutualChannels);
		// }

		for (int c = 0; c < numMutualChannels; c++) { 
			// if (verbose) {
			// 	DEBUG("    Gate[%2d] state = %d", c, channelNotes[c].mGateState);
			// 	DEBUG("              voltage %f", inputs[POLY_GATE_IN_INPUT].getVoltage(c));
			// }
			GateState gateStateBefore = channelNotes[c].mGateState;
			channelNotes[c].mGateState = gateTransitionTable[ inputs[POLY_GATE_IN_INPUT].getVoltage(c) >= 9 ? 1 : 0 ][ channelNotes[c].mGateState ];
			if (channelNotes[c].mGateState != gateStateBefore) {
				changeDetected = true;
			}

			// if (verbose) {
			// 	DEBUG("              state became = %d", channelNotes[c].mGateState);
			// }
		}

		for (int c = numMutualChannels; c < 16; c++) {  // TODO: const, max_polyphony  
			GateState gateStateBefore = channelNotes[c].mGateState;
			channelNotes[c].mGateState = gateTransitionTable[ 0 ][ channelNotes[c].mGateState ]; // close the channel 
			if (channelNotes[c].mGateState != gateStateBefore) {
				changeDetected = true;
			}
			// if (verbose) {
			// 	DEBUG("    Closing Gate[%2d] state = %d", c, channelNotes[c].mGateState);
			// }
		}

		if (changeDetected) {
			int notes[ 16 ]; // todo; CONST 
			int numNotes = 0;

			for (int c = 0; c < 16; c++) {  // TODO: const, max_polyphony  
				// if (verbose) {
				// 	DEBUG("   Channel[%2d]  gateState %d, pitchChange %d", c, channelNotes[c].mGateState, pitchChange[c]);
				// }
				if (channelNotes[c].mGateState == GATE_LEADING_EDGE) {
					// DEBUG("- ReceiveInput: ADD note %d, channel %d", channelNotes[c].mPitch, c);
					notes[numNotes] = channelNotes[c].mPitch;
					numNotes++;

	// 				arpPlayer.addNote(c, channelNotes[c].mPitch);
				}
				else if (channelNotes[c].mGateState == GATE_OPEN) {
					// DEBUG("- ReceiveInput: UPDATE note %d, channel %d", channelNotes[c].mPitch, c);
					notes[numNotes] = channelNotes[c].mPitch;
					numNotes++;
				}
				// else if (channelNotes[c].mGateState == GATE_TRAILING_EDGE) {
				// 	DEBUG("- ReceiveInput: REMOVE note %d, channel %d", channelNotes[c].mPitch, c);
				// 	arpPlayer.removeNote(c);
				// 	//removeNote(c, channelNotes[c].mPitch);
				// }
				// else if (channelNotes[c].mGateState == GATE_OPEN && pitchChange[c]) {
				// 	DEBUG("- ReceiveInput: UPDATE note %d, channel %d", channelNotes[c].mPitch, c);
				// 	arpPlayer.updateNote(c, channelNotes[c].mPitch);
				// 	//updateNote(c, channelNotes[c].mPitch);
				// 	notes[numNotes] = channelNotes[c].mPitch;
				// 	numNotes++;
				// }
			}

			mArpPlayer.setNotes(numNotes, notes);
		} // end changeDetected
	}

	// Loads rules into "pending" Lsystem 
	bool loadRulesFromFile(std::string const& filepath) {

		DEBUG("Loading L-System rules from %s", filepath.c_str());

		AsciiFileReader reader;
		reader.open(filepath);
		if (reader.isOpen()) {
			mpLSystemPending->clear();	
			SpecParser specParser(reader, (*mpLSystemPending));
			specParser.parse();
			// DEBUG("After parse: isErrored() = %d", specParser.isErrored());
			if (specParser.isErrored()) {
				DEBUG("Error in L-System Rules file: %s", filepath.c_str());
				DEBUG("   Error message: %s", specParser.getErrorText().c_str() == 0 ? "non-specific error" : specParser.getErrorText().c_str());
				DEBUG("   line %d, column %d", specParser.getErrorLineNumber(), specParser.getErrorColumnNumber());
				return false;
			}
		}

		// TODO: remove these loggings  .. 
		// log_lsystem(mpLSystemPending, "File Load before expand: Pending");
		// log_lsystem(mpLSystemActive,  "File Load before expand: Active");

		mRulesFilepath = filepath; 

  		expandLSystemSequence(mpLSystemPending); 

		// TODO: remove these loggings  .. 
		// log_lsystem(mpLSystemPending, "install, before swap: Pending");
		// log_lsystem(mpLSystemActive,  "install, before swap: Active");

		// Swap the pending and active lsystem pointers
		LSystem * pTemp = mpLSystemActive;
		mpLSystemActive = mpLSystemPending;
		mpLSystemPending = pTemp;

		mpLSystemPending->clear();

		// TODO: remove these loggings  .. 
		// log_lsystem(mpLSystemPending, "install, after swap: Pending");
		// log_lsystem(mpLSystemActive,  "install, after swap: Active");

		return true;
	}

	void expandLSystemSequence(LSystem * pLSystem) {

		// TODO:  ?? maybe spawn a thread to do the parse and expansion ? 
		ExpandedTermSequence * pExpandedTermSequence = expand(pLSystem, mExpansionDepth);
		if (pExpandedTermSequence) {
			//DEBUG("Num Expanded terms: %d", pExpandedTermSequence->getLength());
			installExpandedSequence(pExpandedTermSequence);
		}

		// TODO: remove these loggings  .. 
		// log_lsystem(mpLSystemPending, "File Load after expand: Pending");
		// log_lsystem(mpLSystemActive,  "File Load after expand: Active");
	}

	// Callback from UI 
	void setRulesFile(std::string const& filepath) {
		DEBUG("Load Rules from %s", filepath.c_str());
		loadRulesFromFile(filepath);
	}

	void stepArpPlayer() {

		if (mpExpandedTermSequence == 0) {
			return; // no arp rules configured 
		}

	   	// double samplesPerBeat = mPlayRateQuantizer.getSamplesPerBeatDivision();
	   	// rather than special case, always shorten the note by a bit
	   	// double noteLengthSamples = mPlayRateQuantizer.computeSamplesPerBeatDivision(mArpPlayerNoteLengthBeatDivision);
	  	// double noteLengthSamplesShortened = noteLengthSamples - (samplesPerBeat * 1.5/96.0); // minus one midi tick at 96 PPQ 


	   	//double samplesPerBeat = clock.getSamplesPerBeat();
   		//double noteLengthSamples = samplesPerBeat;   // TODO: redundant 
   		//double noteLengthSamplesShortened = noteLengthSamples - (samplesPerBeat * 1.5/96.0); // minus one midi tick at 96 PPQ

		//arpPlayer.setNominalNoteLengthSamples( int(noteLengthSamplesShortened) );
		mArpPlayer.setNoteEnabled(true);
		// arpPlayer.setNoteDelaySamples( 0 ); 
		mArpPlayer.setNoteWriter(&mOutputNoteWriter);
		mArpPlayer.setScaleOutlierStrategy(mOutlierStrategy);  // TODO: is this used anymore?


// TODO: this is already done in ArpPlayer init .. 
		for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
		{
			mArpPlayer.setHarmonyEnabled(i, true);
			//arpPlayer.setHarmonyVelocityMultiplier(i, mHarmonyVelocityMultiplier[i]);
			// mArpPlayer.setHarmonyChannel(i, i);
			// arpPlayer.setHarmonyDelay(i, int((mHarmonyStagger[i] * samplesPerBeat) + 0.5));
		}

		mArpPlayer.setNoteProbability(mNoteProbability);
		mArpPlayer.setHarmonyProbability(mHarmonyProbability);

		mArpPlayer.setArpStepperStyle(mArpPlayerArpStyle);
		mArpPlayer.setHarmonyOvershootStrategy(mHarmonyIntervalStepperStrategy); 
		mArpPlayer.setSemitoneOvershootStrategy(mSemitoneIntervalStepperStrategy); 
		mArpPlayer.setPlayOrder(mArpPlayerSortingOrder);   // TODO: if ArpAlgo = AsReceived ? set Asreceived: else set LowToHigh
		mArpPlayer.setKeysOffStrategy(mKeysOffStrategy);

		// Update sample delay to match the current clock and sample rate
		// 
		// TODO: fix this so that arpPlayer does not deal with samples at all
		//  assign length and delays at the noteOn() callback when inserting
		// CvEvents into the output queue
		// AND ... optimize so that the delay times are pre-calulated on significant
		//   events like sample rate change, clock rate change, etc

		// float delayPct = delayPercentageOptions[ delaySelected[0] ];
		// float delaySamples = samplesPerBeat * delayPct;  // TODO: hack .. for now 
		// arpPlayer.setNoteDelaySamples(delaySamples);
		// for (int i = 0; i < 3; i++) { // TODO const
		// 	delayPct = delayPercentageOptions[ delaySelected[ (i+1) ] ];
		// 	delaySamples = samplesPerBeat * delayPct;  // TODO: hack .. for now 
		// 	arpPlayer.setHarmonyDelay(i, delaySamples);
		// }



		// arpPlayer.setScaleDescription(); 

		// mArpPlayer.setNominalNoteLengthSamples( int(noteLengthSamplesShortened) );
		// mArpPlayer.setNoteEnabled(mNoteEnabled);
		// mArpPlayer.setNoteDelaySamples( int( (mNoteDelayPercent * samplesPerBeat) + 0.5) ); 
		// mArpPlayer.setNoteChannel( mNoteChannel );

		// mArpPlayer.setPlayOrder(mArpPlayerSortingOrder);
		// mArpPlayer.setOverstepAction(mArpPlayerArpStyle);
		// mArpPlayer.setExecuteDirection(mArpPlayerExecuteDirection);
		// mArpPlayer.setOctaveRange(mArpPlayerNoteOctaveMin, mArpPlayerNoteOctaveMax);
		// mArpPlayer.setHarmonyOctaveRange(mArpPlayerHarmonyOctaveMin, mArpPlayerHarmonyOctaveMax);
		// mArpPlayer.setHarmonyOvershootStrategy(mHarmonyIntervalBoundsStrategy); 
		// mArpPlayer.setSemitoneOvershootStrategy(mSemitoneIntervalBoundsStrategy); 
		// mArpPlayer.setVelocityMode(mVelocityMode);
		// for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
		// {
		// 	mArpPlayer.setHarmonyEnabled(i, mHarmonyEnabled[i]);
		// 	mArpPlayer.setHarmonyVelocityMultiplier(i, mHarmonyVelocityMultiplier[i]);
		// 	mArpPlayer.setHarmonyChannel(i, mHarmonyChannel[i]);
		// 	mArpPlayer.setHarmonyDelay(i, int((mHarmonyStagger[i] * samplesPerBeat) + 0.5));
		// }
		// mArpPlayer.setHarmonyRelativeVelocity(mHarmonyRelativeVelocity);
		// mArpPlayer.setNoteRelativeVelocity(mNoteRelativeVelocity);

			//    // EXPERIMENT to auto-compute harmony delay based on swing amount  
			//    if (mIsSwingDelayComputed)
			//    {
			//       // http://www.emathhelp.net/calculators/algebra-2/parabola-calculator/?ft=p3&p1x=0&p1y=0.5&p2x=0.66666&p2y=0.66666&p3x=1&p3y=0.5&p3dir=x&steps=on

			//       //double numer = 416650000.0;    // TODO: precompute this 
			//       //double denom = 555561111.0;
			//       double A = -(416650000.0/555561111.0);
			//       double B = 416650000.0/555561111.0;
			//       double C = 0.5;

			//       double x = mPlayRateQuantizer.getSwingAmount();
			//       double y = (A*x*x) + (B*x) + C;

			//       double autoSwing = y;

			//       if (mNoteDelayPercent > 0.0)
			//       {
			//          mArpPlayer.setNoteDelaySamples( int( (autoSwing * samplesPerBeat) + 0.5) ); 
			//       }

			//       for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
			//       {
			//          if (mHarmonyStagger[i] > 0.0)
			//          {
			//             MYTRACE((  "Default stagger for harmony[%d] is %lf\n", i, mHarmonyStagger[i] ));

			//             //double minStagger = 0.5;
			//             //double maxStagger = 1.0;
			//             //double autoSwing = minStagger + (mPlayRateQuantizer.getEffectiveSwingPercent() * (maxStagger - minStagger));

			//             if (i == 0)
			//             {
			//                MYTRACE(( "  Computed swing:  swing is %lf%%,  x = %lf, autoDelay is %lf%%\n", 
			//                   mPlayRateQuantizer.getEffectiveSwingPercent() * 100.0,
			//                   x,
			//                   autoSwing * 100.0 ));
			//             }

			//             double delaySamples = autoSwing * samplesPerBeat;
			//             mArpPlayer.setHarmonyDelay(i, int(delaySamples));
			//          }
			//       }
			//    }
			//    // END EXPERIMENT

		//mArpPlayer.setHarmonyStagger(mHarmonyStagger);
		//double adjustedStagger = mHarmonyStagger * (mPlayRateQuantizer.getSamplesPerBeatDivision() / noteLengthSamplesShortened);

		//double adjustedStagger = mHarmonyStagger * (samplesPerBeat / noteLengthSamplesShortened);
		//mArpPlayer.setHarmonyStagger(adjustedStagger);
		
		// mArpPlayer.setNoteProbability(mNoteProbability);
		// mArpPlayer.setHarmonyProbability(mHarmonyProbability);
		// mArpPlayer.setOctaveIncludesPerfect(mOctaveIncludesPerfect);

		//   double swingPercent =  mPlayRateQuantizer.getEffectiveSwingPercent();

		//      double noteLengthSamples = 
		//      mPlayRateQuantizer.getSamplesPerBeatDivisionSwingAdjusted();
		//   mArpPlayer.setNominalNoteLengthSamples(noteLengthSamples);

		mArpPlayer.step();

	}


/**
 * float delayTable[16];  // tells delay percentage for notes on each channel
 * 
 * delay percwentages:    0, 0.25, 0.333, 0.5, 0.667, 0.75, (1.0 ??)
 * 
 * void onDelayUpdate(int channel, float percentage) {
 *     delayTable[channel] = percentage;
 * 
 *     // if pre-computing this, need to recompute on delay percent change,
 *     // a clock rate change (i.e., step size change), or sample rate change  
 *     delaySamples[channel] = delayTable[channel] * stepSizeSamples
 * }
 * 
 * noteOn from arpPlayer tells channelNUmber, noteNumber
 * noteOnDelay = delayTable[channel] * stepSize; // Precompute this 
 * noteOffDelay = noteOnDelay + noteDuration
 * enqueue(noteOnDelay, NoteOn, channel, voltage)
 * enqueue(noteOffDelay, NoteOff, channel, voltage)
*/
	// void noteOn(int channelNumber, int noteNumber, int velocity, int lengthSamples, int delaySamples) {
	void noteOn(int channelNumber, int noteNumber, int velocity) {
		float voltage = pitchToVoct(noteNumber);

		// TODO: pre-compute the delays per channel 
		float delayPct = delayPercentageOptions[ delaySelected[channelNumber] ];
		int delaySamples = int(clock.getSamplesPerBeat() * delayPct);  // TODO: hack .. for now 

		// TODO: precompute this 
	   	float samplesPerBeat = clock.getSamplesPerBeat();
		int noteLengthSamples = int(samplesPerBeat - clock.mSampleRateCalculator.computeSamplesForMilliseconds(10));

		if (mIsDoubleTime > 0) {
			delaySamples = int(clock.getSamplesPerBeat() * mTwinklePercent * delayPct);  // TODO: hack .. for now 
			noteLengthSamples = int((samplesPerBeat * mTwinklePercent) - clock.mSampleRateCalculator.computeSamplesForMilliseconds(10));
		}

		//DEBUG("noteOn() chan %d, note %d, voltage %f, length %d, delay %d", channelNumber, noteNumber, voltage, lengthSamples, delaySamples);
		// DEBUG("noteOn() chan %d, note %d, voltage %f, length %d, delay %d", channelNumber, noteNumber, voltage, noteLengthSamples, delaySamples);
		CvEvent * pCvEvent = cvEventPool.allocate();
		pCvEvent->set(delaySamples, channelNumber, voltage, CV_EVENT_LEADING_EDGE);		
		outputQueue.insertTimeOrdered(pCvEvent);

		// DEBUG("noteOff() chan %d, note %d, voltage %f, delay %d", channelNumber, noteNumber, voltage, noteLengthSamples + delaySamples);
		pCvEvent = cvEventPool.allocate();
		pCvEvent->set(delaySamples + noteLengthSamples, channelNumber, voltage, CV_EVENT_TRAILING_EDGE);		
		outputQueue.insertTimeOrdered(pCvEvent);

		// DoubleLinkListIterator<CvEvent> iter(outputQueue);
		// while (iter.hasMore()) {
		// 	CvEvent * pCvEvent = iter.getNext();
		// 	DEBUG("  queue: cv event, type %d, chan %d, voltage %f, delay %ld", pCvEvent->eventType, pCvEvent->channel, pCvEvent->voltage, pCvEvent->delay);
		// }
	}

	void produceOutput() {

		outputQueue.passTime(1);

		// static int counter = 0;
		// counter --;
		// if (counter <= 0 || outputQueue.hasExpiredItems()) {
		// 	DEBUG(".. produceOutput()  queue size = %d", outputQueue.size());
		// 	DoubleLinkListIterator<CvEvent> iter(outputQueue);
		// 	while (iter.hasMore()) {
		// 		CvEvent * pCvEvent = iter.getNext();
		// 		DEBUG("  queue: cv event, type %d, chan %d, voltage %f, delay %ld", pCvEvent->eventType, pCvEvent->channel, pCvEvent->voltage, pCvEvent->delay);
		// 	}
		// }
		// if (counter <= 0) {
		// 	counter = 2000;
		// }

// bool show_voltages = false; // TODO: remove 

		// Update output channel states
		while (outputQueue.hasExpiredItems()) {
			CvEvent * pCvEvent = outputQueue.getNextExpired();
			if (pCvEvent->isLeadingEdge()) {
//				DEBUG("------------ produceOutput: open gate %d, voltage %f", pCvEvent->channel, pCvEvent->voltage);
				polyChannelManager[ pCvEvent->channel ].openGate(pCvEvent->voltage);
//show_voltages = true;
			} else {
//				DEBUG("------------ produceOutput: close gate %d, voltage %f", pCvEvent->channel, pCvEvent->voltage);
				polyChannelManager[ pCvEvent->channel ].closeGate(pCvEvent->voltage);				
//show_voltages = true;
			}
			cvEventPool.retire(pCvEvent);
		}

		outputs[ GATE_POLY_OUTPUT ].setChannels(4);
		outputs[ VOCT_POLY_OUTPUT ].setChannels(4);

		for (int i = 0; i < 4; i++ ){ // TODO: constt  
			if (polyChannelManager[i].mIsGateOpen) {
				outputs[ outputPorts[i].gateId ].setVoltage(10.f);
				outputs[ outputPorts[i].portId ].setVoltage(polyChannelManager[i].mActiveVoltage);
				outputs[ GATE_POLY_OUTPUT ].setVoltage(10.f, i);
				outputs[ VOCT_POLY_OUTPUT ].setVoltage(polyChannelManager[i].mActiveVoltage, i);
			} else {
				outputs[ outputPorts[i].gateId ].setVoltage(0.f);
				//outputs[ outputPorts[i].portId ].setVoltage(0.f);
				outputs[ GATE_POLY_OUTPUT ].setVoltage(0.f, i);
				//outputs[ VOCT_POLY_OUTPUT ].setVoltage(0.f, i);
			}
			// if (show_voltages) {
			// 	DEBUG("------------ produceOutput: Gate Voltage [%d] (port %d) = %f", i, outputPorts[i].gateId, outputs[ outputPorts[i].gateId ].getVoltage());
			// }
		}
		// if (show_voltages) {
		// 	DEBUG("------------ produceOutput: Poly Gate Voltage = %f, %f, %f, %f", 
		// 		outputs[ GATE_POLY_OUTPUT ].getVoltage(0),
		// 		outputs[ GATE_POLY_OUTPUT ].getVoltage(1),
		// 		outputs[ GATE_POLY_OUTPUT ].getVoltage(2),
		// 		outputs[ GATE_POLY_OUTPUT ].getVoltage(3));
		// }
	}

	// -- Scale ----------------------------------------------------------------

	// enum ScaleTypes {
	// 	SCALE_BY_POLY = 0,
	// 	SCALE_CUSTOM = 1,
	// 	SCALE_PREDEFINED = 2
	// };
	// ScaleSource scaleSource = SCALE_PREDEFINED; 

	// struct PolyVoltage { 
	// 	float voltage[16]; // max polyphony 
	// 	int   numActive = 0; 
	// };

	void setActiveScale(int scaleIdx) {
		// DEBUG("Set Active Scale: %d", scaleIdx);
		if (ScaleTable::isValidIndex(scaleIdx)) {

			mArpPlayer.setScaleDefinition(&ScaleTable::scaleTable[scaleIdx]);
			// activeScale.setScale(&ScaleTable::scaleTable[scaleIdx]);
			scaleRedrawRequired = true;

			// for (int i = 0; i < 12; i++) {
			// 	DEBUG("  degree[%2d] = %d", i, activeScale.isPitchEnabled(i));
			// }
			// DEBUG("== in setActiveScale( %d )", scaleIdx);
			captureScaleEnableStates();
		}
		activeScaleIdx = scaleIdx;
	}

	void sortFloatAscending(int numMembers, float * values) {		// Bubble sort 
        int j, k;
        for (j = 0; j < (numMembers - 1); j++) {    
            // Last i elements are already in place
            for (k = 0; k < (numMembers - j - 1); k++) {
                if (values[k] > values[k + 1]) {
                    // swap these elements
                    float tmp = values[k];
                    values[k] = values[k + 1];
                    values[k + 1] = tmp;
                }
            }
        }
	}


	int poly_scale_channels = 0;
	float poly_scale_voct[16] = {0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f}; 

	float tmp_voct[16];  // TODO: use the getVoltages() form that takes a destination array to copy the floats into 

	void showScaleEnabledStates_debug() { 
		for (int i = 0; i < 12; i++) {
			DEBUG("ScaleEnabled[ %2d ], %s", i, scaleButtonEnabled[i] ? "ENABLED" : "-disabled-");
		}		
	}

	void captureScaleEnableStates() {
		// DEBUG("Capturing Scale Enabled states");
		for (int i = 0; i < 12; i++) {
			scaleButtonEnabled[i] = mArpPlayer.getTwelveToneScale().isPitchEnabledRelativeToC(i);
		}		
		scaleRedrawRequired = true;
		// showScaleEnabledStates_debug();  // DEBUG 
	}

	void setScaleFromPoly(int numChannels, float * polyVoct) {
		// TODO: convert voct voltages to degrees 
		// activeScale.clear()
		// activeScale.setDegreeEnabled(x)
		// Optimize: keep copy of polyVoct and compare so that scale rebuild is only done if polyVoct changes 


		// TODO: OPTIMIZE THIS ... ! 

		for (int i = 0; i < numChannels; i++) {
			tmp_voct[i] = polyVoct[i];
		}

		bool poly_scale_changed = false;
		if (numChannels != poly_scale_channels) {
			poly_scale_changed = true;
		}
		else
		{
			for (int i = 0; i < numChannels; i++) {
				if (polyVoct[i] != poly_scale_voct[i]) { // TODO: epsilon test ?? 
					poly_scale_changed = true;
					break;
				}
			}
		}

		if (poly_scale_changed) {

			// Save current state fotr future comparisons 
			poly_scale_channels = numChannels;

			for (int i = 0; i < numChannels; i++) {
				poly_scale_voct[i] = tmp_voct[i];
			}

			//DEBUG("-- Before Sort --");
			sortFloatAscending(numChannels, tmp_voct);
			//DEBUG("-- After Sort --");

			int rootPitch = voctToPitch(tmp_voct[0]);

			customScaleDefinition.clear();
			customScaleDefinition.name = "PolyUi";
			for (int i = 0; i < numChannels; i++) {
				//DEBUG("  adding pitch for voct %f, pitch = %d", tmp_voct[i], voctToPitch(tmp_voct[i] - tmp_voct[0]));
				customScaleDefinition.addPitch( voctToPitch(tmp_voct[i] - tmp_voct[0]) );
			}

			// DEBUG("Poly Scale Changed: num channels %d", numChannels); 
			// for (int i = 0; i < customScaleDefinition.numPitches; i++) {
			// 	DEBUG("   Poly Scale Degree: %d", customScaleDefinition.pitches[i]); 
			// }			
			mArpPlayer.setRootPitch(rootPitch);   // TODO: combine tonic and scale set into single call to avoid computing table twice 
			mArpPlayer.setScaleDefinition(&customScaleDefinition);
			captureScaleEnableStates();
			scaleRedrawRequired = true; 
		}
	}

	void setScaleFromUi() {
		
		// DEBUG("setScaleFromUi():");
		// showScaleEnabledStates(); // DEBUG 

		customScaleDefinition.clear();
		customScaleDefinition.name = "Custom"; 
		int rootDegree = mArpPlayer.getTwelveToneScale().getTonicPitch() % 12;
		for (int i = 0; i < 12; i++) {
			if (scaleButtonEnabled[i]) {
				int degree = i - rootDegree;  // transpose into 0-based intervals, based on current tonic pitch 
				if (degree < 0) {
					degree += 12;
				}
				customScaleDefinition.addPitch(degree);
			}	
		}
		activeScaleIdx = -1; // custom scale 
		mArpPlayer.setScaleDefinition(&customScaleDefinition);
		captureScaleEnableStates();
		scaleRedrawRequired = true; 
	}

	// -- Arp Player ------------------------------------------------

	ExpandedTermSequence * expand(LSystem * pLSystem, int depth)
	{
		MYTRACE(( " Expanding LSystem .. depth = %d", depth ));
		if (pLSystem == 0)
		{
			MYTRACE(( " LSystem is null .. nothing to expand"));
			return 0;
		}

		LSystemExpander expander((*pLSystem), mMemoryRecycler, mArpPlayer.getExpressionContext() );

		ExpandedTermSequence * pExpandedSequence = expander.expand("S", depth);  // << TODO: use const for staring rule/axiom

		if (pExpandedSequence == 0)
		{
			MYTRACE(( " Expansion errors .. Expansion cancelled" ));  // << todo; write to status bar 
		}

		return pExpandedSequence;
	}

	void installExpandedSequence(ExpandedTermSequence * pExpandedSequence)
	{
		recycleExpandedTerms(this->mpExpandedTermSequence); 
		delete mpExpandedTermSequence;

		mpExpandedTermSequence = pExpandedSequence;

		// reset all pointers, visitor and iterators to point to the new tree 
		// point arp player to point to the new tree 
		mArpPlayer.setExpandedTermSequence((*mpExpandedTermSequence));

		inspectExpandedSequence(mpExpandedTermSequence);

		// Refresh the UI
		arpPlayerRedrawRequired = true;

		// updateExpansionLengthDisplay();
		// updatePlayableDisplay();
		// updateNotePresentDisplays();
		// updateHarmonyPresentDisplays();
		// #if USE_HARMONY_BALANCE_CONTROL 
		// updateHarmonyActiveDisplays();
		// #endif

		// // todo: add updateStatusLine(..) method 
		// char buf[128];
		// sprintf(buf, "Sequence Length %d", mpExpandedSequence->getLength());
		// mpPatternStatsControl->SetTextFromPlug(buf);
	}

	void inspectExpandedSequence(ExpandedTermSequence * pSequence)
	{
		mTermCounter.clearCounts();
		if (pSequence != 0)	{
			if (pSequence->getLength() > 0) {
				pSequence->countTerms( &mTermCounter );
			}
		}
	}

	// // TODO: is this used ??  .. is the term counter used ? 
   	// bool isHarmonyPresent(int harmonyIdx)
   	// {
    // 	return mTermCounter.hasHarmonyActions(harmonyIdx);
   	// }

	void recycleExpandedTerms(ExpandedTermSequence * pExpandedSequence)
	{
		if ((pExpandedSequence != 0) && (pExpandedSequence->isEmpty() == false))
		{
			DoubleLinkList<ExpandedTerm> & terms = pExpandedSequence->getTerms();
			ExpandedTerm * pExpandedTerm;
			while((pExpandedTerm = terms.popFront()) != 0)
			{
				mMemoryRecycler.retireExpandedTerm(pExpandedTerm);
			}
		}
	}

};

#define _UI_POS_ARP_ALGORITHM_PARAM   mm2px(Vec(102.455, 52.821))
#define _UI_POS_ARP_DELAY_0_PARAM   mm2px(Vec(48.007, 48.077))
#define _UI_POS_ARP_DELAY_1_PARAM   mm2px(Vec(54.647, 48.077))
#define _UI_POS_ARP_DELAY_2_PARAM   mm2px(Vec(61.287, 48.077))
#define _UI_POS_ARP_DELAY_3_PARAM   mm2px(Vec(67.927, 48.077))
#define _UI_POS_ARP_OCTAVE_MAX_PARAM   mm2px(Vec(23.037, 52.821))
#define _UI_POS_ARP_OCTAVE_MIN_PARAM   mm2px(Vec(9.128, 52.821))
#define _UI_POS_ARP_PROBABILITY_PARAM   mm2px(Vec(86.649, 52.821))
#define _UI_POS_DELAY_TIMING_PARAM   mm2px(Vec(168.937, 52.821))
#define _UI_POS_DOUBLE_TIME_BUTTON_PARAM   mm2px(Vec(189.632, 52.214))
#define _UI_POS_DOUBLE_TIME_COUNT_PARAM   mm2px(Vec(200.762, 50.301))
#define _UI_POS_EXPANSION_DEPTH_PARAM   mm2px(Vec(172.127, 22.339))
#define _UI_POS_HARMONY_1_DELAY_0_PARAM   mm2px(Vec(48.007, 73.995))
#define _UI_POS_HARMONY_1_DELAY_1_PARAM   mm2px(Vec(54.647, 73.995))
#define _UI_POS_HARMONY_1_DELAY_2_PARAM   mm2px(Vec(61.287, 73.995))
#define _UI_POS_HARMONY_1_DELAY_3_PARAM   mm2px(Vec(67.927, 73.995))
#define _UI_POS_HARMONY_2_DELAY_0_PARAM   mm2px(Vec(48.007, 85.806))
#define _UI_POS_HARMONY_2_DELAY_1_PARAM   mm2px(Vec(54.647, 85.806))
#define _UI_POS_HARMONY_2_DELAY_2_PARAM   mm2px(Vec(61.287, 85.806))
#define _UI_POS_HARMONY_2_DELAY_3_PARAM   mm2px(Vec(67.927, 85.806))
#define _UI_POS_HARMONY_3_DELAY_0_PARAM   mm2px(Vec(48.007, 97.395))
#define _UI_POS_HARMONY_3_DELAY_1_PARAM   mm2px(Vec(54.647, 97.395))
#define _UI_POS_HARMONY_3_DELAY_2_PARAM   mm2px(Vec(61.287, 97.395))
#define _UI_POS_HARMONY_3_DELAY_3_PARAM   mm2px(Vec(67.927, 97.395))
#define _UI_POS_HARMONY_OCTAVE_MAX_PARAM   mm2px(Vec(23.037, 81.154))
#define _UI_POS_HARMONY_OCTAVE_MIN_PARAM   mm2px(Vec(9.279, 81.154))
#define _UI_POS_HARMONY_OVERSHOOT_STRATEGY_PARAM   mm2px(Vec(102.455, 81.141))
#define _UI_POS_HARMONY_PROBABILITY_PARAM   mm2px(Vec(86.649, 81.141))
#define _UI_POS_INTERVAL_STEP_SIZE_PARAM   mm2px(Vec(120.013, 81.141))
#define _UI_POS_KEY_ORDER_BUTTON_PARAM   mm2px(Vec(77.084, 11.082))
#define _UI_POS_KEY_STRIDE_PARAM   mm2px(Vec(120.013, 52.821))
#define _UI_POS_MAINTAIN_CONTEXT_BUTTON_PARAM   mm2px(Vec(151.276, 18.167))
#define _UI_POS_OBEY_PARAM   mm2px(Vec(155.664, 71.412))
#define _UI_POS_OCTAVE_PERFECT_BUTTON_PARAM   mm2px(Vec(49.256, 24.366))
#define _UI_POS_PITCH_A_PARAM   mm2px(Vec(131.384, 116.26))
#define _UI_POS_PITCH_A_SHARP_PARAM   mm2px(Vec(135.0, 108.587))
#define _UI_POS_PITCH_B_PARAM   mm2px(Vec(140.292, 116.26))
#define _UI_POS_PITCH_C_PARAM   mm2px(Vec(86.846, 116.26))
#define _UI_POS_PITCH_C_SHARP_PARAM   mm2px(Vec(91.608, 108.587))
#define _UI_POS_PITCH_D_PARAM   mm2px(Vec(95.754, 116.26))
#define _UI_POS_PITCH_D_SHARP_PARAM   mm2px(Vec(100.34, 108.587))
#define _UI_POS_PITCH_E_PARAM   mm2px(Vec(104.661, 116.26))
#define _UI_POS_PITCH_F_PARAM   mm2px(Vec(113.569, 116.26))
#define _UI_POS_PITCH_F_SHARP_PARAM   mm2px(Vec(117.538, 108.587))
#define _UI_POS_PITCH_G_PARAM   mm2px(Vec(122.477, 116.26))
#define _UI_POS_PITCH_G_SHARP_PARAM   mm2px(Vec(126.269, 108.587))
#define _UI_POS_PLAY_DIRECTION_PARAM   mm2px(Vec(126.651, 14.683))
#define _UI_POS_RANDOMIZE_NOTES_BUTTON_PARAM   mm2px(Vec(64.552, 24.366))
#define _UI_POS_RESET_BUTTON_PARAM   mm2px(Vec(21.901, 24.366))
#define _UI_POS_ROOT_PITCH_PARAM   mm2px(Vec(75.676, 114.725))
#define _UI_POS_RUN_BUTTON_PARAM   mm2px(Vec(34.97, 24.366))
#define _UI_POS_SEMITONE_OVERSHOOT_STRATEGY_PARAM   mm2px(Vec(140.714, 52.821))

#define _UI_POS_ARP_ALGORITHM_CV_INPUT   mm2px(Vec(102.455, 42.502))
#define _UI_POS_ARP_DELAY_CV_INPUT   mm2px(Vec(38.869, 48.077))
#define _UI_POS_ARP_OCTAVE_MAX_CV_INPUT   mm2px(Vec(23.037, 42.502))
#define _UI_POS_ARP_OCTAVE_MIN_CV_INPUT   mm2px(Vec(9.128, 42.502))
#define _UI_POS_ARP_PROBABILITY_CV_INPUT   mm2px(Vec(86.649, 42.502))
#define _UI_POS_CLOCK_CV_INPUT   mm2px(Vec(8.458, 14.841))
#define _UI_POS_DELAY_TIMING_CV_INPUT   mm2px(Vec(168.937, 42.502))
#define _UI_POS_DOUBLE_TIME_CV_INPUT   mm2px(Vec(189.632, 42.502))
#define _UI_POS_HARMONY_1_DELAY_CV_INPUT   mm2px(Vec(38.869, 73.995))
#define _UI_POS_HARMONY_2_DELAY_CV_INPUT   mm2px(Vec(38.869, 85.806))
#define _UI_POS_HARMONY_3_DELAY_CV_INPUT   mm2px(Vec(38.495, 97.395))
#define _UI_POS_HARMONY_OCTAVE_MAX_CV_INPUT   mm2px(Vec(23.037, 92.795))
#define _UI_POS_HARMONY_OCTAVE_MIN_CV_INPUT   mm2px(Vec(9.279, 92.795))
#define _UI_POS_HARMONY_OVERSHOOT_STRATEGY_CV_INPUT   mm2px(Vec(102.455, 91.748))
#define _UI_POS_HARMONY_PROBABILITY_CV_INPUT   mm2px(Vec(86.649, 91.748))
#define _UI_POS_INTERVAL_STEP_SIZE_CV_INPUT   mm2px(Vec(120.013, 91.748))
#define _UI_POS_KEY_ORDER_CV_INPUT   mm2px(Vec(86.142, 11.082))
#define _UI_POS_KEY_STRIDE_CV_INPUT   mm2px(Vec(120.013, 42.502))
#define _UI_POS_OBEY_CV_INPUT   mm2px(Vec(167.717, 71.412))
#define _UI_POS_OCTAVE_PERFECT_CV_INPUT   mm2px(Vec(49.256, 14.841))
#define _UI_POS_PLAY_DIRECTION_CV_INPUT   mm2px(Vec(113.099, 14.683))
#define _UI_POS_POLY_GATE_IN_INPUT   mm2px(Vec(29.137, 114.485))
#define _UI_POS_POLY_IN_CV_INPUT   mm2px(Vec(9.565, 114.601))
#define _UI_POS_POLY_SCALE_CV_INPUT   mm2px(Vec(45.703, 114.857))
#define _UI_POS_RANDOMIZE_NOTES_CV_INPUT   mm2px(Vec(64.552, 14.841))
#define _UI_POS_RESET_CV_INPUT   mm2px(Vec(21.901, 14.841))
#define _UI_POS_ROOT_PITCH_CV_INPUT   mm2px(Vec(64.405, 114.954))
#define _UI_POS_RUN_CV_INPUT   mm2px(Vec(34.97, 14.841))
#define _UI_POS_SEMITONE_OVERSHOOT_STRATEGY_CV_INPUT   mm2px(Vec(140.714, 42.502))

#define _UI_POS_CLOCK_OUTPUT   mm2px(Vec(214.037, 81.906))
#define _UI_POS_END_OF_CYCLE_OUTPUT   mm2px(Vec(203.411, 19.073))
#define _UI_POS_GATE_FOUR_OUTPUT   mm2px(Vec(214.316, 103.16))
#define _UI_POS_GATE_ONE_OUTPUT   mm2px(Vec(174.171, 103.16))
#define _UI_POS_GATE_POLY_OUTPUT   mm2px(Vec(151.792, 103.16))
#define _UI_POS_GATE_THREE_OUTPUT   mm2px(Vec(201.085, 103.16))
#define _UI_POS_GATE_TWO_OUTPUT   mm2px(Vec(187.488, 103.16))
#define _UI_POS_RUN_OUTPUT   mm2px(Vec(198.418, 81.906))
#define _UI_POS_VOCT_FOUR_OUTPUT   mm2px(Vec(214.316, 116.046))
#define _UI_POS_VOCT_ONE_OUTPUT   mm2px(Vec(174.171, 116.046))
#define _UI_POS_VOCT_POLY_OUTPUT   mm2px(Vec(151.792, 116.046))
#define _UI_POS_VOCT_THREE_OUTPUT   mm2px(Vec(201.085, 116.046))
#define _UI_POS_VOCT_TWO_OUTPUT   mm2px(Vec(187.488, 116.046))

#define _UI_POS_ARP_DELAY_0_LED_LIGHT   mm2px(Vec(47.633, 57.733))
#define _UI_POS_ARP_DELAY_1_LED_LIGHT   mm2px(Vec(54.647, 57.733))
#define _UI_POS_ARP_DELAY_2_LED_LIGHT   mm2px(Vec(61.287, 57.733))
#define _UI_POS_ARP_DELAY_3_LED_LIGHT   mm2px(Vec(67.927, 57.733))
#define _UI_POS_DIRECTION_FORWARD_LED_LIGHT   mm2px(Vec(130.663, 25.027))
#define _UI_POS_DIRECTION_REVERSE_LED_LIGHT   mm2px(Vec(121.869, 25.027))
#define _UI_POS_DOUBLE_TIME_LED_LIGHT   mm2px(Vec(189.632, 52.214))
// #define _UI_POS_HARMONY_1_DELAY_0_LED_LIGHT   mm2px(Vec(47.871, 68.974))
// #define _UI_POS_HARMONY_1_DELAY_1_LED_LIGHT   mm2px(Vec(54.511, 68.974))
// #define _UI_POS_HARMONY_1_DELAY_2_LED_LIGHT   mm2px(Vec(61.151, 68.974))
// #define _UI_POS_HARMONY_1_DELAY_3_LED_LIGHT   mm2px(Vec(67.791, 68.974))
// #define _UI_POS_HARMONY_2_DELAY_0_LED_LIGHT   mm2px(Vec(47.871, 80.88))
// #define _UI_POS_HARMONY_2_DELAY_1_LED_LIGHT   mm2px(Vec(54.511, 80.88))
// #define _UI_POS_HARMONY_2_DELAY_2_LED_LIGHT   mm2px(Vec(61.151, 80.88))
// #define _UI_POS_HARMONY_2_DELAY_3_LED_LIGHT   mm2px(Vec(67.791, 80.88))
// #define _UI_POS_HARMONY_3_DELAY_0_LED_LIGHT   mm2px(Vec(47.871, 92.522))
// #define _UI_POS_HARMONY_3_DELAY_1_LED_LIGHT   mm2px(Vec(54.511, 92.522))
// #define _UI_POS_HARMONY_3_DELAY_2_LED_LIGHT   mm2px(Vec(61.151, 92.522))
// #define _UI_POS_HARMONY_3_DELAY_3_LED_LIGHT   mm2px(Vec(67.791, 92.522))
#define _UI_POS_KEY_ORDER_AS_RECEIVED_LED_LIGHT   mm2px(Vec(77.159, 25.953))
#define _UI_POS_KEY_ORDER_HIGH_TO_LOW_LED_LIGHT   mm2px(Vec(77.159, 21.72))
#define _UI_POS_KEY_ORDER_LOW_TO_HIGH_LED_LIGHT   mm2px(Vec(77.159, 17.646))
#define _UI_POS_MAINTAIN_CONTEXT_LED_LIGHT   mm2px(Vec(151.276, 18.167))
#define _UI_POS_OCTAVE_PERFECT_LED_LIGHT   mm2px(Vec(49.256, 24.366))
#define _UI_POS_PITCH_A_LIGHT   mm2px(Vec(131.433, 116.343))
#define _UI_POS_PITCH_A_SHARP_LIGHT   mm2px(Vec(135.049, 108.67))
#define _UI_POS_PITCH_B_LIGHT   mm2px(Vec(140.341, 116.343))
#define _UI_POS_PITCH_C_LIGHT   mm2px(Vec(86.895, 116.343))
#define _UI_POS_PITCH_C_SHARP_LIGHT   mm2px(Vec(91.657, 108.67))
#define _UI_POS_PITCH_D_LIGHT   mm2px(Vec(95.802, 116.343))
#define _UI_POS_PITCH_D_SHARP_LIGHT   mm2px(Vec(100.388, 108.67))
#define _UI_POS_PITCH_E_LIGHT   mm2px(Vec(104.71, 116.343))
#define _UI_POS_PITCH_F_LIGHT   mm2px(Vec(113.618, 116.343))
#define _UI_POS_PITCH_F_SHARP_LIGHT   mm2px(Vec(117.586, 108.67))
#define _UI_POS_PITCH_G_LIGHT   mm2px(Vec(122.525, 116.343))
#define _UI_POS_PITCH_G_SHARP_LIGHT   mm2px(Vec(126.318, 108.67))
#define _UI_POS_RANDOMIZE_NOTES_LED_LIGHT   mm2px(Vec(64.552, 24.366))
#define _UI_POS_RESET_LED_LIGHT   mm2px(Vec(21.901, 24.366))
#define _UI_POS_RULES_VALID_LED_LIGHT   mm2px(Vec(185.545, 22.791))
#define _UI_POS_RUN_LED_LIGHT   mm2px(Vec(34.97, 24.366))

#define _UI_POS_RECT4620_WIDGET   mm2px(Vec(180.445, 6.588))   // mm2px(Vec(13.547, 8.467))


struct ExpansionDepthDisplay : TextDisplay {
	TcArpGen* module;
	void step() override {
		int numTerms = 0;
		if (module) {
			numTerms = module->mArpPlayer.getNumTerms();
		}

		if (numTerms > 0) {
			text = string::f("%4d", numTerms);
		} else {
			text = "----";
		}
	}
};


template <class MODULE>
struct SelectRulesFile : MenuItem
{
	MODULE * module;
	void onAction(const event::Action &e) override
	{
		if (arprulesDir.empty()) {
			arprulesDir = asset::plugin(pluginInstance, "arprules");
		}
		char *path = osdialog_file(OSDIALOG_OPEN, arprulesDir.c_str(), NULL, NULL);
		if(path && module)
		{
			arprulesDir = system::getDirectory(path);
			module->setRulesFile(path);
			free(path);
		}
	}
};

template <class MODULE>
struct ScaleSizeMenu : MenuItem {
	MODULE * module;
	int scaleSize;

    struct ScaleSizeSubItem : MenuItem {
		MODULE * module;
		int scaleIdx;
		void onAction(const event::Action& e) override {
 			module->setActiveScale(scaleIdx);
		}
	};

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		for (int i = 0; i < ScaleTable::NUM_SCALES; i++) {
			if (ScaleTable::scaleTable[i].numDegrees == scaleSize) {
				ScaleSizeSubItem * pScaleSizeSubItem = createMenuItem<ScaleSizeSubItem>(ScaleTable::scaleTable[i].name);
				pScaleSizeSubItem->rightText = CHECKMARK(module->activeScaleIdx == i);
				pScaleSizeSubItem->module = module;
				pScaleSizeSubItem->scaleIdx = i;
				menu->addChild(pScaleSizeSubItem);
			}
		}
		return menu;
	}
};



struct SmallSquareSwitch : app::SvgSwitch {
	SmallSquareSwitch() {
		momentary = false;
		shadow->opacity = 0.0;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/ArpGen/square-6x6-off.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/ArpGen/square-6x6-on.svg")));
	}
};

template <class MODULE>
struct ScaleMenu : MenuItem {
	MODULE * module;

    struct ScaleSubItem : MenuItem {
		MODULE * module;
		int scaleIdx;
		void onAction(const event::Action& e) override {
 			module->setActiveScale(scaleIdx);
		}
	};

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		// for (int i = 0; i < ScaleTable::NUM_SCALES; i++) {
		// 	ScaleSubItem * pScaleSubItem = createMenuItem<ScaleSubItem>(ScaleTable::scaleTable[i].name);
		// 	pScaleSubItem->rightText = CHECKMARK(module->activeScaleIdx == i);
		// 	pScaleSubItem->module = module;
		// 	pScaleSubItem->scaleIdx = i;
		// 	menu->addChild(pScaleSubItem);
		// }

		ScaleSizeMenu<TcArpGen> * pScaleMenu7 = createMenuItem<ScaleSizeMenu<TcArpGen>>("7 Note Scale", RIGHT_ARROW);
		pScaleMenu7->module = module;
		pScaleMenu7->scaleSize = 7;
		menu->addChild(pScaleMenu7);

		ScaleSizeMenu<TcArpGen> * pScaleMenu6 = createMenuItem<ScaleSizeMenu<TcArpGen>>("6 Note Scale", RIGHT_ARROW);
		pScaleMenu6->module = module;
		pScaleMenu6->scaleSize = 6;
		menu->addChild(pScaleMenu6);

		ScaleSizeMenu<TcArpGen> * pScaleMenu5 = createMenuItem<ScaleSizeMenu<TcArpGen>>("5 Note Scale", RIGHT_ARROW);
		pScaleMenu5->module = module;
		pScaleMenu5->scaleSize = 5;
		menu->addChild(pScaleMenu5);

		ScaleSizeMenu<TcArpGen> * pScaleMenu4 = createMenuItem<ScaleSizeMenu<TcArpGen>>("4 Note Scale", RIGHT_ARROW);
		pScaleMenu4->module = module;
		pScaleMenu4->scaleSize = 4;
		menu->addChild(pScaleMenu4);

		return menu;
	}
};

struct TcArpGenWidget : ModuleWidget {
	TcArpGenWidget(TcArpGen* module) {
		DEBUG("ARP GEN: widget constructor() .. ");

		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ArpGen/arpgen-4.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_ARP_ALGORITHM_PARAM, module, TcArpGen::ARP_ALGORITHM_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_ARP_DELAY_0_PARAM, module, TcArpGen::ARP_DELAY_0_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_ARP_DELAY_1_PARAM, module, TcArpGen::ARP_DELAY_1_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_ARP_DELAY_2_PARAM, module, TcArpGen::ARP_DELAY_2_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_ARP_DELAY_3_PARAM, module, TcArpGen::ARP_DELAY_3_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_ARP_OCTAVE_MAX_PARAM, module, TcArpGen::ARP_OCTAVE_MAX_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_ARP_OCTAVE_MIN_PARAM, module, TcArpGen::ARP_OCTAVE_MIN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_ARP_PROBABILITY_PARAM, module, TcArpGen::ARP_PROBABILITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_DELAY_TIMING_PARAM, module, TcArpGen::DELAY_TIMING_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_DOUBLE_TIME_BUTTON_PARAM, module, TcArpGen::DOUBLE_TIME_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_DOUBLE_TIME_COUNT_PARAM, module, TcArpGen::DOUBLE_TIME_COUNT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_EXPANSION_DEPTH_PARAM, module, TcArpGen::EXPANSION_DEPTH_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_1_DELAY_0_PARAM, module, TcArpGen::HARMONY_1_DELAY_0_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_1_DELAY_1_PARAM, module, TcArpGen::HARMONY_1_DELAY_1_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_1_DELAY_2_PARAM, module, TcArpGen::HARMONY_1_DELAY_2_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_1_DELAY_3_PARAM, module, TcArpGen::HARMONY_1_DELAY_3_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_2_DELAY_0_PARAM, module, TcArpGen::HARMONY_2_DELAY_0_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_2_DELAY_1_PARAM, module, TcArpGen::HARMONY_2_DELAY_1_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_2_DELAY_2_PARAM, module, TcArpGen::HARMONY_2_DELAY_2_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_2_DELAY_3_PARAM, module, TcArpGen::HARMONY_2_DELAY_3_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_3_DELAY_0_PARAM, module, TcArpGen::HARMONY_3_DELAY_0_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_3_DELAY_1_PARAM, module, TcArpGen::HARMONY_3_DELAY_1_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_3_DELAY_2_PARAM, module, TcArpGen::HARMONY_3_DELAY_2_PARAM));
		addParam(createParamCentered<SmallSquareSwitch>(_UI_POS_HARMONY_3_DELAY_3_PARAM, module, TcArpGen::HARMONY_3_DELAY_3_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_HARMONY_OCTAVE_MAX_PARAM, module, TcArpGen::HARMONY_OCTAVE_MAX_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_HARMONY_OCTAVE_MIN_PARAM, module, TcArpGen::HARMONY_OCTAVE_MIN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_HARMONY_OVERSHOOT_STRATEGY_PARAM, module, TcArpGen::HARMONY_OVERSHOOT_STRATEGY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_HARMONY_PROBABILITY_PARAM, module, TcArpGen::HARMONY_PROBABILITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_INTERVAL_STEP_SIZE_PARAM, module, TcArpGen::INTERVAL_STEP_SIZE_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_KEY_ORDER_BUTTON_PARAM, module, TcArpGen::KEY_ORDER_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_KEY_STRIDE_PARAM, module, TcArpGen::KEY_STRIDE_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_MAINTAIN_CONTEXT_BUTTON_PARAM, module, TcArpGen::MAINTAIN_CONTEXT_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_OBEY_PARAM, module, TcArpGen::OBEY_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_OCTAVE_PERFECT_BUTTON_PARAM, module, TcArpGen::OCTAVE_PERFECT_BUTTON_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_A_PARAM, module, TcArpGen::PITCH_A_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_A_SHARP_PARAM, module, TcArpGen::PITCH_A_SHARP_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_B_PARAM, module, TcArpGen::PITCH_B_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_C_PARAM, module, TcArpGen::PITCH_C_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_C_SHARP_PARAM, module, TcArpGen::PITCH_C_SHARP_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_D_PARAM, module, TcArpGen::PITCH_D_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_D_SHARP_PARAM, module, TcArpGen::PITCH_D_SHARP_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_E_PARAM, module, TcArpGen::PITCH_E_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_F_PARAM, module, TcArpGen::PITCH_F_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_F_SHARP_PARAM, module, TcArpGen::PITCH_F_SHARP_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_G_PARAM, module, TcArpGen::PITCH_G_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_PITCH_G_SHARP_PARAM, module, TcArpGen::PITCH_G_SHARP_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_PLAY_DIRECTION_PARAM, module, TcArpGen::PLAY_DIRECTION_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_RANDOMIZE_NOTES_BUTTON_PARAM, module, TcArpGen::RANDOMIZE_NOTES_BUTTON_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_RESET_BUTTON_PARAM, module, TcArpGen::RESET_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_ROOT_PITCH_PARAM, module, TcArpGen::ROOT_PITCH_PARAM));
		addParam(createParamCentered<LEDButton>(_UI_POS_RUN_BUTTON_PARAM, module, TcArpGen::RUN_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(_UI_POS_SEMITONE_OVERSHOOT_STRATEGY_PARAM, module, TcArpGen::SEMITONE_OVERSHOOT_STRATEGY_PARAM));

		addInput(createInputCentered<PJ301MPort>(_UI_POS_ARP_ALGORITHM_CV_INPUT, module, TcArpGen::ARP_ALGORITHM_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_ARP_DELAY_CV_INPUT, module, TcArpGen::ARP_DELAY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_ARP_OCTAVE_MAX_CV_INPUT, module, TcArpGen::ARP_OCTAVE_MAX_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_ARP_OCTAVE_MIN_CV_INPUT, module, TcArpGen::ARP_OCTAVE_MIN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_ARP_PROBABILITY_CV_INPUT, module, TcArpGen::ARP_PROBABILITY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_CLOCK_CV_INPUT, module, TcArpGen::CLOCK_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_DELAY_TIMING_CV_INPUT, module, TcArpGen::DELAY_TIMING_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_DOUBLE_TIME_CV_INPUT, module, TcArpGen::DOUBLE_TIME_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_HARMONY_1_DELAY_CV_INPUT, module, TcArpGen::HARMONY_1_DELAY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_HARMONY_2_DELAY_CV_INPUT, module, TcArpGen::HARMONY_2_DELAY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_HARMONY_3_DELAY_CV_INPUT, module, TcArpGen::HARMONY_3_DELAY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_HARMONY_OCTAVE_MAX_CV_INPUT, module, TcArpGen::HARMONY_OCTAVE_MAX_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_HARMONY_OCTAVE_MIN_CV_INPUT, module, TcArpGen::HARMONY_OCTAVE_MIN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_HARMONY_OVERSHOOT_STRATEGY_CV_INPUT, module, TcArpGen::HARMONY_OVERSHOOT_STRATEGY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_HARMONY_PROBABILITY_CV_INPUT, module, TcArpGen::HARMONY_PROBABILITY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_INTERVAL_STEP_SIZE_CV_INPUT, module, TcArpGen::INTERVAL_STEP_SIZE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_KEY_ORDER_CV_INPUT, module, TcArpGen::KEY_ORDER_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_KEY_STRIDE_CV_INPUT, module, TcArpGen::KEY_STRIDE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_OBEY_CV_INPUT, module, TcArpGen::OBEY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_OCTAVE_PERFECT_CV_INPUT, module, TcArpGen::OCTAVE_PERFECT_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_PLAY_DIRECTION_CV_INPUT, module, TcArpGen::PLAY_DIRECTION_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_POLY_GATE_IN_INPUT, module, TcArpGen::POLY_GATE_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_POLY_IN_CV_INPUT, module, TcArpGen::POLY_IN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_POLY_SCALE_CV_INPUT, module, TcArpGen::POLY_SCALE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_RANDOMIZE_NOTES_CV_INPUT, module, TcArpGen::RANDOMIZE_NOTES_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_RESET_CV_INPUT, module, TcArpGen::RESET_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_ROOT_PITCH_CV_INPUT, module, TcArpGen::ROOT_PITCH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_RUN_CV_INPUT, module, TcArpGen::RUN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(_UI_POS_SEMITONE_OVERSHOOT_STRATEGY_CV_INPUT, module, TcArpGen::SEMITONE_OVERSHOOT_STRATEGY_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_CLOCK_OUTPUT, module, TcArpGen::CLOCK_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_END_OF_CYCLE_OUTPUT, module, TcArpGen::END_OF_CYCLE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_GATE_FOUR_OUTPUT, module, TcArpGen::GATE_FOUR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_GATE_ONE_OUTPUT, module, TcArpGen::GATE_ONE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_GATE_POLY_OUTPUT, module, TcArpGen::GATE_POLY_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_GATE_THREE_OUTPUT, module, TcArpGen::GATE_THREE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_GATE_TWO_OUTPUT, module, TcArpGen::GATE_TWO_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_RUN_OUTPUT, module, TcArpGen::RUN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_VOCT_FOUR_OUTPUT, module, TcArpGen::VOCT_FOUR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_VOCT_ONE_OUTPUT, module, TcArpGen::VOCT_ONE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_VOCT_POLY_OUTPUT, module, TcArpGen::VOCT_POLY_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_VOCT_THREE_OUTPUT, module, TcArpGen::VOCT_THREE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(_UI_POS_VOCT_TWO_OUTPUT, module, TcArpGen::VOCT_TWO_OUTPUT));

		addChild(createLightCentered<SmallLight<OrangeLight>>(_UI_POS_ARP_DELAY_0_LED_LIGHT, module, TcArpGen::ARP_DELAY_0_LED_LIGHT));
		addChild(createLightCentered<SmallLight<OrangeLight>>(_UI_POS_ARP_DELAY_1_LED_LIGHT, module, TcArpGen::ARP_DELAY_1_LED_LIGHT));
		addChild(createLightCentered<SmallLight<OrangeLight>>(_UI_POS_ARP_DELAY_2_LED_LIGHT, module, TcArpGen::ARP_DELAY_2_LED_LIGHT));
		addChild(createLightCentered<SmallLight<OrangeLight>>(_UI_POS_ARP_DELAY_3_LED_LIGHT, module, TcArpGen::ARP_DELAY_3_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_DIRECTION_FORWARD_LED_LIGHT, module, TcArpGen::DIRECTION_FORWARD_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_DIRECTION_REVERSE_LED_LIGHT, module, TcArpGen::DIRECTION_REVERSE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_DOUBLE_TIME_LED_LIGHT, module, TcArpGen::DOUBLE_TIME_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_1_DELAY_0_LED_LIGHT, module, TcArpGen::HARMONY_1_DELAY_0_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_1_DELAY_1_LED_LIGHT, module, TcArpGen::HARMONY_1_DELAY_1_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_1_DELAY_2_LED_LIGHT, module, TcArpGen::HARMONY_1_DELAY_2_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_1_DELAY_3_LED_LIGHT, module, TcArpGen::HARMONY_1_DELAY_3_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_2_DELAY_0_LED_LIGHT, module, TcArpGen::HARMONY_2_DELAY_0_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_2_DELAY_1_LED_LIGHT, module, TcArpGen::HARMONY_2_DELAY_1_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_2_DELAY_2_LED_LIGHT, module, TcArpGen::HARMONY_2_DELAY_2_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_2_DELAY_3_LED_LIGHT, module, TcArpGen::HARMONY_2_DELAY_3_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_3_DELAY_0_LED_LIGHT, module, TcArpGen::HARMONY_3_DELAY_0_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_3_DELAY_1_LED_LIGHT, module, TcArpGen::HARMONY_3_DELAY_1_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_3_DELAY_2_LED_LIGHT, module, TcArpGen::HARMONY_3_DELAY_2_LED_LIGHT));
		// addChild(createLightCentered<SmallLight<RedLight>>(_UI_POS_HARMONY_3_DELAY_3_LED_LIGHT, module, TcArpGen::HARMONY_3_DELAY_3_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(_UI_POS_KEY_ORDER_AS_RECEIVED_LED_LIGHT, module, TcArpGen::KEY_ORDER_AS_RECEIVED_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(_UI_POS_KEY_ORDER_HIGH_TO_LOW_LED_LIGHT, module, TcArpGen::KEY_ORDER_HIGH_TO_LOW_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(_UI_POS_KEY_ORDER_LOW_TO_HIGH_LED_LIGHT, module, TcArpGen::KEY_ORDER_LOW_TO_HIGH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_MAINTAIN_CONTEXT_LED_LIGHT, module, TcArpGen::MAINTAIN_CONTEXT_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_OCTAVE_PERFECT_LED_LIGHT, module, TcArpGen::OCTAVE_PERFECT_LED_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_A_LIGHT, module, TcArpGen::PITCH_A_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_A_SHARP_LIGHT, module, TcArpGen::PITCH_A_SHARP_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_B_LIGHT, module, TcArpGen::PITCH_B_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_C_LIGHT, module, TcArpGen::PITCH_C_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_C_SHARP_LIGHT, module, TcArpGen::PITCH_C_SHARP_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_D_LIGHT, module, TcArpGen::PITCH_D_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_D_SHARP_LIGHT, module, TcArpGen::PITCH_D_SHARP_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_E_LIGHT, module, TcArpGen::PITCH_E_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_F_LIGHT, module, TcArpGen::PITCH_F_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_F_SHARP_LIGHT, module, TcArpGen::PITCH_F_SHARP_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_G_LIGHT, module, TcArpGen::PITCH_G_LIGHT));
		addChild(createLightCentered<LargeLight<BlueLight>>(_UI_POS_PITCH_G_SHARP_LIGHT, module, TcArpGen::PITCH_G_SHARP_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(_UI_POS_RANDOMIZE_NOTES_LED_LIGHT, module, TcArpGen::RANDOMIZE_NOTES_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(_UI_POS_RESET_LED_LIGHT, module, TcArpGen::RESET_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(_UI_POS_RULES_VALID_LED_LIGHT, module, TcArpGen::RULES_VALID_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_RUN_LED_LIGHT, module, TcArpGen::RUN_LED_LIGHT));

		// mm2px(Vec(11.695, 8.467))
		ExpansionDepthDisplay* display = createWidget<ExpansionDepthDisplay>(_UI_POS_RECT4620_WIDGET);
		display->box.size = mm2px(Vec(15.5, 8.197));
		display->module = module;
		addChild(display);

	}

	void appendContextMenu(Menu *menu) override
	{
		TcArpGen *module = dynamic_cast<TcArpGen*>(this->module);
		assert(module);

        // menu->addChild(new MenuSeparator());
		// menu->addChild(createMenuLabel("Polyphony"));

		// PolyphonyMenu *polySubMenu = createMenuItem<PolyphonyMenu>("Polyphony", RIGHT_ARROW);
		// polySubMenu->module = module;
		// menu->addChild(polySubMenu);

        menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("L-System Rules File"));
		SelectRulesFile<TcArpGen> *pSelectRulesFile = new SelectRulesFile<TcArpGen>();
		pSelectRulesFile->module = module;
		pSelectRulesFile->text = module->mRulesFilepath;
		menu->addChild(pSelectRulesFile);

        menu->addChild(new MenuSeparator());
		ScaleMenu<TcArpGen> * pScaleMenu = createMenuItem<ScaleMenu<TcArpGen>>("Scale", RIGHT_ARROW);
		pScaleMenu->module = module;
		menu->addChild(pScaleMenu);
	}

};


Model* modelTcArpGen = createModel<TcArpGen, TcArpGenWidget>("tcArpGen");