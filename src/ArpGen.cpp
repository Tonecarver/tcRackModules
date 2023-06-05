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
#include "../lib/arpeggiator/Stepper.hpp"
#include "../lib/clock/ClockWatcher.hpp"
#include "../lib/scale/PolyScale.hpp"
#include "../lib/scale/Scales.hpp"
#include "../lib/scale/TwelveToneScale.hpp"

#include "../lib/cv/Voltage.hpp"
#include "../lib/cv/CvEvent.hpp"
#include "../lib/cv/Gate.hpp"
#include "../lib/cv/PolyGatedCv.hpp"
#include "../lib/cv/PolyVoltages.hpp"

#include "../lib/datastruct/DelayList.hpp"
#include "../lib/datastruct/FreePool.hpp"

#include "../lib/dsp/TriggerPool.hpp"


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
	virtual void noteOn(int channelNumber, int noteNumber, int velocity) override {
		// DEBUG("NoteWriter: channel %d, note %d, velocity %d", channelNumber, noteNumber, velocity);
		pReceiver->noteOn(channelNumber, noteNumber, velocity);
	}
}; 


struct ThemeManager {
	int  selectedThemeId; 
	bool mRedrawRequired;

	ThemeManager() 
	  : selectedThemeId(0)
	  , mRedrawRequired(false)
	{
	}

	void selectTheme(int themeId) {
		if (themeId != selectedThemeId) {
			selectedThemeId = themeId;
			mRedrawRequired = true;
		}
	}

	int getTheme() const {
		return selectedThemeId;
	}

	bool redrawRequired() const { 
		return mRedrawRequired; 
	}

	void redrawComplete() { 
		mRedrawRequired = false; 
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


	std::string mRulesFilepath = ""; 

	// -- Scale --
	int activeScaleIdx = 0;
	ScaleDefinition customScaleDefinition;
	PolyVoltages    mExternalScaleVoltages;
	PolyScale       mExternalScale;
	bool            scaleRedrawRequired = true;

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

	TriggerPool<TwelveToneScale::NUM_DEGREES_PER_SCALE> mScaleButtonTriggers;
	bool scaleButtonEnabled[TwelveToneScale::NUM_DEGREES_PER_SCALE];

	// -- random -- 
	FastRandom mRandom;
	dsp::SchmittTrigger  mRandomizeHarmoniesTrigger; 

	// -- clock --
	ClockWatcher clock;
	dsp::SchmittTrigger	mResetButtonTrigger; 
	dsp::SchmittTrigger	mResetTrigger; 
	dsp::PulseGenerator mResetPulse;

	// -- inputs --
	NoteTable   noteTable;          // Input Notes 
	PolyGatedCv polyInputGatedCv;   // Gated V/Oct ControlVoltages 

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

	int mStride = 1; // number of key positions to move on an increment or decrement operation

	bool                mRunning = true; 
	dsp::SchmittTrigger mRunTrigger; 
	dsp::SchmittTrigger mRunButtonTrigger; 

	// double-time
	int mTwinkleSteps = 3;  // number of steps to take when activated
	int mTwinkleCount = 0;  // number of active steps pending 
	const float minTwinkleSteps = 3.f; 
	const float maxTwinkleSteps = 32.f;
	dsp::SchmittTrigger mTwinkleTrigger; // TODO: add trigger to reset 
	float mTwinklePercent = 0.5f;

	bool mIsDoubleTime = false; // true if outputting at 2x clock 

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
	int delayPeriod = 0; // 0..3 to indicate which delay light tracer to light 
	int delayTime = 0;   // numbr of samples until the next delay period 

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

	TriggerPool<NUM_DELAY_SLOTS> mDelayTriggers[4]; // 4 trigger pools, one for one 4 steps of arp arp + 3 for 4 steps of harm harm 

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


	// Panel Theme
	ThemeManager mTheme;

	// -- -- 
	TcArpGen() {

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
			delayPeriod = (delayPeriod + 1) & 0x03; // 0..3
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

	}

	void onReset() override	
	{
		// TODO: see examples in Fundamental/SEQ.cpp about handling reset, run state, and clocks, etc.

		mRunTrigger.reset(); 
		mRunButtonTrigger.reset(); 
		mKeysOffTrigger.reset();
		mPerfectOctaveTrigger.reset();
		mRandomizeHarmoniesTrigger.reset();
		mResetButtonTrigger.reset();
		mResetTrigger.reset(); 
		mSortingOrderTrigger.reset(); 

		mScaleButtonTriggers.reset();

		for (int i = 0; i < NUM_DELAYS; i++) { // TODO: const, num delays
			mDelayTriggers[i].reset(); 
		}

		// TODO: put play cursor at position 0 
	}
	
	void onSampleRateChange() override {
		clock.setSampleRate(APP->engine->getSampleRate());
	}

	virtual json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_object_set_new(rootJ, "sortingOrder", json_integer(mArpPlayerSortingOrder));

		json_object_set_new(rootJ, "scaleIndex", json_integer(activeScaleIdx));

		json_object_set_new(rootJ, "extScaleFormat", json_integer(mExternalScale.getScaleFormat()));

		json_t* enabledNotes = json_array();
		for (int i = 0; i < 12; i++) {
			json_array_insert_new(enabledNotes, i, json_boolean(mArpPlayer.getTwelveToneScale().isDegreeEnabled(i)));
		}
		json_object_set_new(rootJ, "enabledNotes", enabledNotes);

		json_t* delaySelectionsJ = json_array();
		for (int i = 0; i < NUM_DELAYS; i++) {
			json_array_insert_new(delaySelectionsJ, i, json_integer(delaySelected[i]));
		}
		json_object_set_new(rootJ, "delaySelections", delaySelectionsJ);

		json_object_set_new(rootJ, "octaveIncludesPerfect", json_integer(mOctaveIncludesPerfect));

 		json_object_set_new(rootJ, "rulesFilepath", json_string(mRulesFilepath.c_str()));

		json_object_set_new(rootJ, "themeId", json_integer(mTheme.getTheme()));

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

        jsonValue = json_object_get(rootJ, "extScaleFormat"); /// TODO: change this to use a string name ? 
		if (jsonValue) {
			PolyScale::PolyScaleFormat format = (PolyScale::PolyScaleFormat) json_integer_value(jsonValue);
			mExternalScale.setScaleFormat(format);
			scaleRedrawRequired = true;
		}

		json_t* enabledNotes = json_object_get(rootJ, "enabledNotes");
		if (enabledNotes) {
			bool degreeEnabled[12]; // TODO: const 
			for (int i = 0; i < 12; i++) {
				json_t* enabledNote = json_array_get(enabledNotes, i);
				if (enabledNote) {
					degreeEnabled[i] = json_boolean_value(enabledNote);
				}
			}
			mArpPlayer.getTwelveToneScale().setDegreesEnabled(degreeEnabled);
			captureScaleEnableStates();
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
			std::string filepath = json_string_value(jsonValue);
			if (! filepath.empty()) {
				loadRulesFromFile(filepath);
			}
		}

        jsonValue = json_object_get(rootJ, "themeId");
		if (jsonValue) {
			mTheme.selectTheme(json_integer_value(jsonValue));
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

			// TODO: apply some HYSTERESIS to let the new value settle before doing all the work to repace the terms 

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

	// incoming val is 0..1
	ScaleOutlierStrategy getScaleOutlierStrategy(float val) {
		int strategy = round(val * 3.f);
		strategy = clamp(strategy,0,2);
		return (ScaleOutlierStrategy) strategy;
	}

	LSystemExecuteDirection getPlayDirection(float val) {
		return (LSystemExecuteDirection) round(val);
	}

	void processPlayerConfigurationChanges() {
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
			mNoteProbability = inputs[ ARP_PROBABILITY_CV_INPUT ].getVoltage() * 0.1f; // 0..10 ==> 0..1
		} else {
			mNoteProbability = params[ ARP_PROBABILITY_PARAM ].getValue();
		}

		if (inputs[ HARMONY_PROBABILITY_CV_INPUT ].isConnected()) {
			mHarmonyProbability = inputs[ HARMONY_PROBABILITY_CV_INPUT ].getVoltage() * 0.1f; // 0..10 ==> 0..1 
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
			mIntervalStepSize = intervalStepSize;
			mArpPlayer.setIntervalStepSize(intervalStepSize);
		}
	}

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
			arpOctaveMax = inputs[ARP_OCTAVE_MAX_CV_INPUT].getVoltage() * 0.2f; // +/- 5 ==> -1..1
		} 
		if (inputs[HARMONY_OCTAVE_MIN_CV_INPUT].isConnected()) {
			harmonyOctaveMin = inputs[HARMONY_OCTAVE_MIN_CV_INPUT].getVoltage() * 0.2f;  // -5..5 ==> -1..1
		} 
		if (inputs[HARMONY_OCTAVE_MAX_CV_INPUT].isConnected()) {
			harmonyOctaveMax = inputs[HARMONY_OCTAVE_MAX_CV_INPUT].getVoltage() * 0.2f; // -5..5 ==> -1..1
		} 

		// Perfect Octave 
		// TODO: add CV 
		if (mPerfectOctaveTrigger.process(params[OCTAVE_PERFECT_BUTTON_PARAM].getValue())) {
			mOctaveIncludesPerfect = (! mOctaveIncludesPerfect);
			mArpPlayer.setOctaveIncludesPerfect(mOctaveIncludesPerfect);
		}

		float octaveRange = 2.f;

		// Arp Octaves 
		int octaveMin = round(arpOctaveMin * octaveRange);		
		int octaveMax = round(arpOctaveMax * octaveRange);		

		if (octaveMin > octaveMax) {
			octaveMin = octaveMax; // limit min to be <= max 
		}

		if (octaveMin != mArpOctaveMin || octaveMax != mArpOctaveMax) {
			mArpOctaveMin = octaveMin;
			mArpOctaveMax = octaveMax;
			// DEBUG("process octaves: ARP changed to: octaves %d ... %d", mArpOctaveMin, mArpOctaveMax);
			mArpPlayer.setArpOctaveRange(mArpOctaveMin, mArpOctaveMax);
		}

		// Harmony Octaves
		octaveMin = round(harmonyOctaveMin * octaveRange);		
		octaveMax = round(harmonyOctaveMax * octaveRange);		

		if (octaveMin > octaveMax) {
			octaveMin = octaveMax; // limit min to be <= max 
		}

		if (octaveMin != mHarmonyOctaveMin || octaveMax != mHarmonyOctaveMax) {
			mHarmonyOctaveMin = octaveMin;
			mHarmonyOctaveMax = octaveMax;
			mArpPlayer.setHarmonyOctaveRange(mHarmonyOctaveMin, mHarmonyOctaveMax);
		}
	}

	void processDelayChanges() { 
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

		// TODO: optimize this so that 1 delay channel is evaluated at a time
		// instead of all 16 switches at every sample

		// TODO: if CVinput is connected, set the delay percentage from that

		for (int i = 0; i < NUM_DELAYS; i++) { 
			for (int s = 0; s < NUM_DELAY_SLOTS; s++) {
				int paramId = delaySelectors[i].delaySwitchIds[s];
				if (mDelayTriggers[i].process(s, params[paramId].getValue() )) {
					if (delaySelected[i] != s) { 
						setDelaySelection(i, s);
					}
				}
			}
		}

		for (int i = 0; i < NUM_DELAYS; i++) {
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
			for (int i = 0; i < NUM_DELAYS; i++) {
				for (int switchIdx = 0; switchIdx < NUM_DELAY_SLOTS; switchIdx++) { // TODO: const NUM_DELAY_OPTIONS 
					int paramId = delaySelectors[i].delaySwitchIds[switchIdx];
					params[ paramId ].setValue(delaySelected[i] == switchIdx);
				}
			}
		} else {
			for (int i = 0; i < NUM_DELAYS; i++) {
				int selected = delaySelected[i];
				int paramId = delaySelectors[i].delaySwitchIds[selected];
				params[ paramId ].setValue(1);
			}
		}

		delayRedrawRequired = false;
	}

	void processScaleChanges() {
		if (inputs[POLY_SCALE_CV_INPUT].isConnected()) {
			int numChannels = inputs[POLY_SCALE_CV_INPUT].getChannels();
			float * voltages = inputs[POLY_SCALE_CV_INPUT].getVoltages();
			if (mExternalScaleVoltages.process(numChannels, voltages)) {
				setScaleFromPoly(numChannels, voltages);
			}
		} else { 
			mExternalScaleVoltages.reset();

			float rootVoct;
			if (inputs[ROOT_PITCH_CV_INPUT].isConnected()) {
				rootVoct = inputs[ROOT_PITCH_CV_INPUT].getVoltage(); // -5..5
			} else {
				rootVoct = (params[ROOT_PITCH_PARAM].getValue() / 12.f) - 5.f; // 0..120 ==> -5..5
			}

			int rootPitch = voctToPitch(rootVoct);
			if (rootPitch != mArpPlayer.getRootPitch()) {
				mArpPlayer.setRootPitch(rootPitch);
				captureScaleEnableStates();
				scaleRedrawRequired = true;			
			}

			bool scaleButtonChanged = false;
			for (int i = 0; i < TwelveToneScale::NUM_DEGREES_PER_SCALE; i++) {
				if (mScaleButtonTriggers.process( i, params[ scaleButtons[i].mParamIdx ].getValue())) {
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

	void receiveInputs() {
		if (polyInputGatedCv.process(
			inputs[POLY_IN_CV_INPUT].getChannels(),   inputs[POLY_IN_CV_INPUT].getVoltages(), 
			inputs[POLY_GATE_IN_INPUT].getChannels(), inputs[POLY_GATE_IN_INPUT].getVoltages()))
		{ 
			int notes[ PORT_MAX_CHANNELS ]; // todo; max_channels
			int numNotes = 0;
			for (int c = 0; c < PORT_MAX_CHANNELS; c++) {  // TODO: const, max_channels
				if (polyInputGatedCv.isGateOpen(c)) {
					notes[numNotes] = voctToPitch( polyInputGatedCv.getControlVoltage(c) );
					numNotes++;
				}
			}
			mArpPlayer.setNotes(numNotes, notes);
		}
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

		// log_lsystem(mpLSystemPending, "File Load before expand: Pending");
		// log_lsystem(mpLSystemActive,  "File Load before expand: Active");

		mRulesFilepath = filepath; 

  		expandLSystemSequence(mpLSystemPending); 

		// log_lsystem(mpLSystemPending, "install, before swap: Pending");
		// log_lsystem(mpLSystemActive,  "install, before swap: Active");

		// Swap the pending and active lsystem pointers
		LSystem * pTemp = mpLSystemActive;
		mpLSystemActive = mpLSystemPending;
		mpLSystemPending = pTemp;

		mpLSystemPending->clear();

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

		mArpPlayer.setNoteEnabled(true);
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

	void noteOn(int channelNumber, int noteNumber, int velocity) {
		float voltage = pitchToVoct(noteNumber);

		// TODO: pre-compute the delay samples per channel whever the clock, the swing, or the delay selection changes 
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

		// Update output channel states
		while (outputQueue.hasExpiredItems()) {
			CvEvent * pCvEvent = outputQueue.getNextExpired();
			if (pCvEvent->isLeadingEdge()) {
				polyChannelManager[ pCvEvent->channel ].openGate(pCvEvent->voltage);
			} else {
				polyChannelManager[ pCvEvent->channel ].closeGate(pCvEvent->voltage);				
			}
			cvEventPool.retire(pCvEvent);
		}

		outputs[ GATE_POLY_OUTPUT ].setChannels(4);
		outputs[ VOCT_POLY_OUTPUT ].setChannels(4);

		for (int c = 0; c < 4; c++ ){ // TODO: const
			if (polyChannelManager[c].mIsGateOpen) {
				outputs[ outputPorts[c].gateId ].setVoltage(10.f);
				outputs[ outputPorts[c].portId ].setVoltage(polyChannelManager[c].mActiveVoltage);
				outputs[ GATE_POLY_OUTPUT ].setVoltage(10.f, c);
				outputs[ VOCT_POLY_OUTPUT ].setVoltage(polyChannelManager[c].mActiveVoltage, c);
			} else {
				outputs[ outputPorts[c].gateId ].setVoltage(0.f);
				outputs[ GATE_POLY_OUTPUT ].setVoltage(0.f, c);
			}
		}
	}

	// -- Scale ----------------------------------------------------------------

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

	// void showScaleEnabledStates_debug() { 
	// 	for (int i = 0; i < 12; i++) {
	// 		DEBUG("ScaleEnabled[ %2d ], %s", i, scaleButtonEnabled[i] ? "ENABLED" : "-disabled-");
	// 	}		
	// }

	void captureScaleEnableStates() {
		// DEBUG("Capturing Scale Enabled states");
		for (int i = 0; i < 12; i++) {
			scaleButtonEnabled[i] = mArpPlayer.getTwelveToneScale().isPitchEnabledRelativeToC(i);
		}		
		scaleRedrawRequired = true;
		//showScaleEnabledStates_debug();  // DEBUG 
	}

	void setPolyExternalScaleFormat(PolyScale::PolyScaleFormat format) { 
		mExternalScale.setScaleFormat(format);
	}

	PolyScale::PolyScaleFormat getPolyExternalScaleFormat() const { 
		return mExternalScale.getScaleFormat();
	}

	void setScaleFromPoly(int numChannels, float * polyVoct) {
		mExternalScale.computeScale(numChannels, polyVoct);
		// todo: make show_external_scale() method 
		// DEBUG(" SCALE ------------------ Tonic %d", mExternalScale.getTonic());
		// for (int i = 0; i < 12; i++) { 
		// 	DEBUG("   Degree[%2d] = %s", i, mExternalScale.containsDegree(i) ? "yes" : "-");
		// }

		customScaleDefinition.clear();
		customScaleDefinition.name = "PolyUi";
		for (int i = 0; i < 12; i++) {
			if (mExternalScale.containsDegree(i)) {
				customScaleDefinition.addPitch( i );
			}
		}

// TODO: set active scale idx ?? 

		activeScaleIdx = -1; // custom scale 

		// TODO: combine tonic and scale set into single call to avoid computing table twice 
		mArpPlayer.setRootPitch(mExternalScale.getTonic());
		mArpPlayer.setScaleDefinition(&customScaleDefinition);
		captureScaleEnableStates();
		scaleRedrawRequired = true; 
	}

	void setScaleFromUi() {		
		// DEBUG("setScaleFromUi():");
		// showScaleEnabledStates_debug(); // DEBUG 

		customScaleDefinition.clear();
		customScaleDefinition.name = "Custom"; 
		int rootDegree = mArpPlayer.getTwelveToneScale().getTonicPitch() % 12;
		for (int i = 0; i < TwelveToneScale::NUM_DEGREES_PER_SCALE; i++) {
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
#define _UI_POS_HARMONY_1_DELAY_0_PARAM   mm2px(Vec(48.007, 72.563))
#define _UI_POS_HARMONY_1_DELAY_1_PARAM   mm2px(Vec(54.647, 72.563))
#define _UI_POS_HARMONY_1_DELAY_2_PARAM   mm2px(Vec(61.287, 72.563))
#define _UI_POS_HARMONY_1_DELAY_3_PARAM   mm2px(Vec(67.927, 72.563))
#define _UI_POS_HARMONY_2_DELAY_0_PARAM   mm2px(Vec(48.007, 84.373))
#define _UI_POS_HARMONY_2_DELAY_1_PARAM   mm2px(Vec(54.647, 84.373))
#define _UI_POS_HARMONY_2_DELAY_2_PARAM   mm2px(Vec(61.287, 84.373))
#define _UI_POS_HARMONY_2_DELAY_3_PARAM   mm2px(Vec(67.927, 84.373))
#define _UI_POS_HARMONY_3_DELAY_0_PARAM   mm2px(Vec(48.007, 95.963))
#define _UI_POS_HARMONY_3_DELAY_1_PARAM   mm2px(Vec(54.647, 95.963))
#define _UI_POS_HARMONY_3_DELAY_2_PARAM   mm2px(Vec(61.287, 95.963))
#define _UI_POS_HARMONY_3_DELAY_3_PARAM   mm2px(Vec(67.927, 95.963))
#define _UI_POS_HARMONY_OCTAVE_MAX_PARAM   mm2px(Vec(23.037, 81.154))
#define _UI_POS_HARMONY_OCTAVE_MIN_PARAM   mm2px(Vec(9.279, 81.154))
#define _UI_POS_HARMONY_OVERSHOOT_STRATEGY_PARAM   mm2px(Vec(102.455, 81.141))
#define _UI_POS_HARMONY_PROBABILITY_PARAM   mm2px(Vec(86.649, 81.154))
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
#define _UI_POS_HARMONY_1_DELAY_CV_INPUT   mm2px(Vec(38.869, 72.563))
#define _UI_POS_HARMONY_2_DELAY_CV_INPUT   mm2px(Vec(38.869, 84.373))
#define _UI_POS_HARMONY_3_DELAY_CV_INPUT   mm2px(Vec(38.495, 95.963))
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

#define _UI_POS_CLOCK_OUTPUT   mm2px(Vec(210.333, 81.906))
#define _UI_POS_END_OF_CYCLE_OUTPUT   mm2px(Vec(203.411, 19.073))
#define _UI_POS_GATE_FOUR_OUTPUT   mm2px(Vec(210.612, 103.16))
#define _UI_POS_GATE_ONE_OUTPUT   mm2px(Vec(170.467, 103.16))
#define _UI_POS_GATE_POLY_OUTPUT   mm2px(Vec(153.542, 103.16))
#define _UI_POS_GATE_THREE_OUTPUT   mm2px(Vec(197.381, 103.16))
#define _UI_POS_GATE_TWO_OUTPUT   mm2px(Vec(183.783, 103.16))
#define _UI_POS_RUN_OUTPUT   mm2px(Vec(194.714, 81.906))
#define _UI_POS_VOCT_FOUR_OUTPUT   mm2px(Vec(210.612, 116.046))
#define _UI_POS_VOCT_ONE_OUTPUT   mm2px(Vec(170.467, 116.046))
#define _UI_POS_VOCT_POLY_OUTPUT   mm2px(Vec(153.542, 116.046))
#define _UI_POS_VOCT_THREE_OUTPUT   mm2px(Vec(197.381, 116.046))
#define _UI_POS_VOCT_TWO_OUTPUT   mm2px(Vec(183.783, 116.046))

#define _UI_POS_ARP_DELAY_0_LED_LIGHT   mm2px(Vec(47.633, 57.733))
#define _UI_POS_ARP_DELAY_1_LED_LIGHT   mm2px(Vec(54.647, 57.733))
#define _UI_POS_ARP_DELAY_2_LED_LIGHT   mm2px(Vec(61.287, 57.733))
#define _UI_POS_ARP_DELAY_3_LED_LIGHT   mm2px(Vec(67.927, 57.733))
#define _UI_POS_DIRECTION_FORWARD_LED_LIGHT   mm2px(Vec(130.663, 25.027))
#define _UI_POS_DIRECTION_REVERSE_LED_LIGHT   mm2px(Vec(121.869, 25.027))
#define _UI_POS_DOUBLE_TIME_LED_LIGHT   mm2px(Vec(189.632, 52.214))
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

template <class MODULE>
struct PolyExternalScaleFormatMenu : MenuItem {
	MODULE * module;

    struct PolyExternalScaleFormatSubItem : MenuItem {
		MODULE * module;
		PolyScale::PolyScaleFormat format;
		void onAction(const event::Action& e) override {
 			module->setPolyExternalScaleFormat(format);
		}
	};

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		// TODO: get names from the PolyScale class 
		std::string formatNames[PolyScale::PolyScaleFormat::kNum_POLY_SCALE_FORMAT] = {
			"Poly Ext Scale", 
			"Sorted V/Oct"};

		PolyScale::PolyScaleFormat formats[PolyScale::PolyScaleFormat::kNum_POLY_SCALE_FORMAT] = {
			PolyScale::PolyScaleFormat::POLY_EXT_SCALE,
			PolyScale::PolyScaleFormat::SORTED_VOCT_SCALE
		};

		for (int i = 0; i < PolyScale::PolyScaleFormat::kNum_POLY_SCALE_FORMAT; i++) {
			PolyExternalScaleFormatSubItem * pSubItem = createMenuItem<PolyExternalScaleFormatSubItem>(formatNames[i]);
			pSubItem->format = formats[i];
			pSubItem->rightText = CHECKMARK(module->getPolyExternalScaleFormat() == formats[i]);
			pSubItem->module = module;
			menu->addChild(pSubItem);
		}
		return menu;
	}
};


template <class MODULE>
struct ThemeMenu : MenuItem {
	MODULE * module;

    struct ThemeSubItem : MenuItem {
		MODULE * module;
		int themeId;
		void onAction(const event::Action& e) override {
			module->mTheme.selectTheme(themeId);
		}
	};

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		std::string themeNames[3] = { "Silver", "Grey", "Blue" };

		for (int i = 0; i < 3; i++) {
			ThemeSubItem * pThemeSubItem = createMenuItem<ThemeSubItem>(themeNames[i]);
 			pThemeSubItem->rightText = CHECKMARK(module->mTheme.getTheme() == i);
			pThemeSubItem->module = module;
			pThemeSubItem->themeId = i;
 			menu->addChild(pThemeSubItem);
		}
		return menu;
	}
};

struct TcArpGenWidget : ModuleWidget {
	TcArpGenWidget(TcArpGen* module) {

		setModule(module);
		//setPanel(createPanel(asset::plugin(pluginInstance, "res/ArpGen/arpgen-5.svg")));
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ArpGen/arpgen-5-silver.svg")));

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

		addChild(createLightCentered<TinyLight<OrangeLight>>(_UI_POS_ARP_DELAY_0_LED_LIGHT, module, TcArpGen::ARP_DELAY_0_LED_LIGHT));
		addChild(createLightCentered<TinyLight<OrangeLight>>(_UI_POS_ARP_DELAY_1_LED_LIGHT, module, TcArpGen::ARP_DELAY_1_LED_LIGHT));
		addChild(createLightCentered<TinyLight<OrangeLight>>(_UI_POS_ARP_DELAY_2_LED_LIGHT, module, TcArpGen::ARP_DELAY_2_LED_LIGHT));
		addChild(createLightCentered<TinyLight<OrangeLight>>(_UI_POS_ARP_DELAY_3_LED_LIGHT, module, TcArpGen::ARP_DELAY_3_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_DIRECTION_FORWARD_LED_LIGHT, module, TcArpGen::DIRECTION_FORWARD_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_DIRECTION_REVERSE_LED_LIGHT, module, TcArpGen::DIRECTION_REVERSE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(_UI_POS_DOUBLE_TIME_LED_LIGHT, module, TcArpGen::DOUBLE_TIME_LED_LIGHT));
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
		pSelectRulesFile->text = module->mRulesFilepath.empty() ? "click to select file" : module->mRulesFilepath;
		menu->addChild(pSelectRulesFile);

        menu->addChild(new MenuSeparator());
		ScaleMenu<TcArpGen> * pScaleMenu = createMenuItem<ScaleMenu<TcArpGen>>("Scale", RIGHT_ARROW);
		pScaleMenu->module = module;
		menu->addChild(pScaleMenu);

		menu->addChild(new MenuSeparator());
		PolyExternalScaleFormatMenu<TcArpGen> * pScaleFormatMenu = createMenuItem<PolyExternalScaleFormatMenu<TcArpGen>>("Poly External Scale Format", RIGHT_ARROW);
		pScaleFormatMenu->module = module;
		menu->addChild(pScaleFormatMenu);

		menu->addChild(new MenuSeparator());
		ThemeMenu<TcArpGen> * pThemeMenu = createMenuItem<ThemeMenu<TcArpGen>>("Theme", RIGHT_ARROW);
		pThemeMenu->module = module;
		menu->addChild(pThemeMenu);

	}

	void step() override {
		if (module) {
			// Redraw panel if required 
			TcArpGen * pModule = dynamic_cast<TcArpGen*>(this->module);
			if (pModule->mTheme.redrawRequired()) {
				int themeId = pModule->mTheme.getTheme(); 
				if (themeId == 0) {        // silver
					setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ArpGen/arpgen-5-silver.svg")));
				} else if (themeId == 1) { // grey
					setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ArpGen/arpgen-5-grey.svg")));
				} else if (themeId == 2) { // blue
					setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ArpGen/arpgen-5.svg")));
				}
				pModule->mTheme.redrawComplete();
			}
		}
		Widget::step();
	}
};


Model* modelTcArpGen = createModel<TcArpGen, TcArpGenWidget>("tcArpGen");