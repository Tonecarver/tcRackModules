#include "plugin.hpp"
#include <math.hpp>
#include <dsp/fft.hpp>
#include "../lib/CircularBuffer.hpp"
#include "../lib/List.hpp"

const int MAX_FFT_FRAME_SIZE = 16384;
const int DFLT_FFT_FRAME_SIZE = 2048;

const int MIN_HISTORY_FRAMES = 2; // allow for minimum blur
const int MAX_HISTORY_FRAMES = 10000; 

const float MAX_HISTORY_SECONDS = 20;

// Tooltip formatters 
struct LengthParamQuantity;
struct PositionParamQuantity;
struct FreqCenterParamQuantity;
struct FreqWidthParamQuantity;
struct PitchShiftParamQuantity; 
struct GainParamQuantity; 
struct BlurFreqSelectorParamQuantity;
struct GainFreqSelectorParamQuantity;
struct PitchFreqSelectorParamQuantity;
struct PitchQuantizeSelectorParamQuantity;
struct MixParamQuantity;
struct FrameDropParamQuantity;
struct FreezeParamQuantity;
struct RobotParamQuantity;
struct BlurSpreadParamQuantity;
struct BlurMixParamQuantity;

struct AlignedBuffer {
    public: float * values;
    
    private: int    numValues;

    public:
        AlignedBuffer(const int numFloats) {
            size_t numBytes = sizeof(float)*numFloats;
            values = (float *) pffft_aligned_malloc(numBytes);
            numValues = numFloats;
            memset(values, 0, numBytes);
        }

        ~AlignedBuffer() {
            pffft_aligned_free(values);
        }

        void clear() { 
            memset(values, 0, sizeof(float)*numValues);
        }    

        int size() { 
            return numValues;
        }

        void makeWindow(int windowSize) {
            clear();
            for (int k = 0; k < windowSize; k++) {
                values[k] = -.5 * cos(2.*M_PI*(float)k/(float)windowSize) + .5; // Hamming window 
                //values[k] = .5 * (1. - cos(2.*M_PI*(float)k/(float)sizenumValues));  // Hann window
            }
        }
};

struct FftFrame : public AlignedBuffer, ListNode {
    private: int binCount;

    public:
        FftFrame(int numBins) : AlignedBuffer(numBins * 2) {
            this->binCount = numBins;
        }

        int numBins() {
            return binCount;
        }
};

struct AudioBuffer : public AlignedBuffer {
    AudioBuffer(const int numSamples) : AlignedBuffer(numSamples) {
    }
};


struct Random { 
    uint32_t x = 123456789;
    uint32_t y = 362436069;
    uint32_t z = 521288629;
    uint32_t w = 88675123;
    
    float generateZeroToOne() { 
        return (float) xor128() / (float)UINT32_MAX;
    }

    float generatePlusMinusOne() { 
        return (((float) xor128() / (float)UINT32_MAX) - 0.5f) * 2.f;
    }
    
    float generatePlusMinusPi() { 
        return (((float) xor128() / (float)UINT32_MAX) - 0.5f) * 2.f * M_PI;
    }
    
    //float generate() { 
    //    return (float) xor128() / (float)UINT32_MAX;
    //}

    uint32_t xor128(void) {
        uint32_t t;
        t = x ^ (x << 11);   
        x = y; y = z; z = w;   
        return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
    }
};

// from https://www.codeproject.com/tips/700780/fast-floor-ceiling-functions
inline int myFloor(float fVal) { 
    return int(fVal + 32768.) - 32768;
}

inline int myCeil(float fVal) {
    return 32768 - int(32768. - fVal);
}

inline float wrapPlusMinusPi(float phase)
{
   // wrap phase difference to -PI .. PI range 
   // bernsee
   //long qpd = phase / M_PI;  // long long needed ??
   long qpd = phase/float(M_PI);
   if (qpd >= 0)
      qpd += qpd & 1;
   else
      qpd -= qpd & 1;
   phase -= M_PI * float(qpd);
   return phase;
}

// Scaling factors for multiplying the synthesized FFT values
// Determined emperically using Sine and Saw waveforms
// Top Value (5.f) is the range of VCV rack audio signals
// Bottom Value is the level of the synthesized FFT output without amplification 

const float DFLT_SAMPLE_RATE_SCALING_FACTOR = 5.f/3.75f;
const struct ScalingFactor {
    float sampleRate;
    float scalingFactor;
} scalingFactors[] = {
    {  11025.f, 5.f/3.75f },
    {  12000.f, 5.f/3.75f },
    {  22050.f, 5.f/3.75f },
    {  24000.f, 5.f/3.75f },
    {  44100.f, 5.f/3.75f },
    {  48000.f, 5.f/3.75f },
    {  88200.f, 5.f/3.75f },
    {  96000.f, 5.f/3.75f },
    { 176400.f, 5.f/3.75f },
    { 192000.f, 5.f/3.75f },
    { 352800.f, 5.f/3.68f },
    { 384000.f, 5.f/3.62f },
    { 705600.f, 5.f/2.60f },
    { 768000.f, 5.f/2.445f },
};

inline float gainForDb(float dB) {
    return pow(10, dB * 0.1); // 10^(db/10)
}

inline float dbForGain(float gain) {
    return 10 * log10(gain);
}

bool isOctaveSemitone(int semitone) {
    switch (semitone) {
        case -36:
        case -24:
        case -12:
        case 0:
        case 12:
        case 24:
        case 36:
            return true;
    }
    return false;
}

bool isFifthSemitone(int semitone) {
    switch (semitone) {
        case -36:
        case -29:
        case -24:
        case -17:
        case -12:
        case -5:
        case 0:
        case 7:
        case 12:
        case 19:
        case 24:
        case 31:
        case 36:
            return true;
    }
    return false;
}


// NOTYET 
struct SelectionCycler { 
    int numOptions;
    int dfltOption; 
    int selected; 
    Light  ** pLights;

    SelectionCycler(int numOptions, int dfltOption) {
        this->numOptions = numOptions;
        this->pLights = new Light * [numOptions];
        this->selected = dfltOption;
    }

    ~SelectionCycler() {
        delete[] pLights;
    }

    void addLight(int val, Light * pLight) { 
        pLights[val] = pLight;
    }   

    int advance() {
        return selectOption(selected == numOptions - 1 ? 0 : selected + 1);
    }

    int selectDefault() {
        return selectOption(dfltOption);
    }

    int selectOption(int option) {
        selected = option;
        for (int k = 0; k < numOptions; k++) {
            pLights[k]->value = (selected == k);
        }
        return selected;
    }
};


struct Blur : Module {
	enum ParamIds {
		HISTORY_LENGTH_PARAM,
		HISTORY_ATTENUVERTER_PARAM,
		FREQ_CENTER_PARAM,
		FREQ_CENTER_ATTENUVERTER_PARAM,
		POSITION_ATTENUVERTER_PARAM,
		POSITION_PARAM,
		FREQ_SPAN_PARAM,
		FREQ_SPAN_ATTENUVERTER_PARAM,
		FRAME_DROP_ATTENUVERTER_PARAM,
		FRAME_DROP_PARAM,
		FREEZE_PARAM,
		BLUR_FREQ_SELECTOR_PARAM,
		PITCH_FREQ_SELECTOR_PARAM,
		GAIN_FREQ_SELECTOR_PARAM,
		BLUR_SPREAD_ATTENUVERTER_PARAM,
		BLUR_SPREAD_PARAM,
		PITCH_PARAM,
		PITCH_ATTENUVERTER_PARAM,
		ROBOT_PARAM,
		PITCH_QUANTIZE_PARAM,
		BLUR_MIX_ATTENUVERTER_PARAM,
		BLUR_MIX_PARAM,
		GAIN_ATTENUVERTER_PARAM,
		GAIN_PARAM,
		PHASE_RESET_PARAM,
		MIX_PARAM,
		MIX_ATTENUVERTER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		HISTORY_CV_INPUT,
		FREQ_CENTER_CV_INPUT,
		POSITION_CV_INPUT,
		FREQ_SPAN_CV_INPUT,
		FRAME_DROP_CV_INPUT,
		FREEZE_CV_INPUT,
		BLUR_SPREAD_CV_INPUT,
		PITCH_CV_INPUT,
		ROBOT_CV_INPUT,
		BLUR_MIX_CV_INPUT,
		GAIN_CV_INPUT,
		AUDIO_IN_INPUT,
		MIX_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		AUDIO_OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		BLUR_FREQ_INSIDE_LED_LIGHT,
		PITCH_FREQ_INSIDE_EXCLUSIVE_LED_LIGHT,
		PITCH_FREQ_INSIDE_LED_LIGHT,
		GAIN_FREQ_INSIDE_LED_LIGHT,
		BLUR_FREQ_OUTSIDE_LED_LIGHT,
		PITCH_FREQ_OUTSIDE_EXCLUSIVE_LED_LIGHT,
		PITCH_FREQ_OUTSIDE_LED_LIGHT,
		GAIN_FREQ_OUTSIDE_LED_LIGHT,
		FREEZE_LED_LIGHT,
		BLUR_FREQ_ALL_LED_LIGHT,
		PITCH_FREQ_ALL_LED_LIGHT,
		GAIN_FREQ_ALL_LED_LIGHT,
		BLUR_FREQ_SELECTOR_LED_LIGHT,
		PITCH_FREQ_SELECTOR_LED_LIGHT,
		GAIN_FREQ_SELECTOR_LED_LIGHT,
		ROBOT_LED_LIGHT,
		PITCH_QUANT_SELECTOR_LED_LIGHT,
		PITCH_QUANT_SMOOTH_LED_LIGHT,
		PITCH_QUANT_SEMITONES_LED_LIGHT,
		PITCH_QUANT_FIFTHS_LED_LIGHT,
		PITCH_QUANT_OCTAVE_LED_LIGHT,
		RESIDUAL_SWITCH_LED_LIGHT,
		NUM_LIGHTS
	};

    // Variables for Tooltips 
    float fHistoryLengthSeconds;
    float fPositionSeconds;
    float fFreqCenterHz;
    float fFreqLowerHz;
    float fFreqUpperHz;


    // +/- gain for final output level
    float fOutputGain = 1.0;
    float fSelectedOutputGain = 1.0;

    DoubleLinkList<FftFrame>  fftFramePool;
    CircularBuffer<FftFrame>  fftFrameHistory{MAX_HISTORY_FRAMES};
    int iMaxHistoryFrames;  // limit the number of history frames accuulated  
    float fFrameDropProbability; // probabiilty of dropping incoming frames, range [0,1]
    bool bFreeze; // drop all incoming frames when buffer is frozen
	dsp::SchmittTrigger freezeTrigger;

    AlignedBuffer window{MAX_FFT_FRAME_SIZE};
    AlignedBuffer inBuffer{MAX_FFT_FRAME_SIZE};
    AlignedBuffer outBuffer{MAX_FFT_FRAME_SIZE};
    int iInIndex; 
    int iLatency;

    int iFftFrameSize;   
    int iOversample;

    int iSelectedFftFrameSize;   // Set by Menu Item (is UI thread concurrent?)
    int iSelectedOversample;     // Set by Menu Item (is UI thread concurrent?)
    float fSelectedSampleRate;   // Set by args.samplerate in process() 

    int iStepSize; 
    FftFrame fftWorkspace{2 * MAX_FFT_FRAME_SIZE};
    FftFrame fftTemp{MAX_FFT_FRAME_SIZE};

    AlignedBuffer lastPhase{MAX_FFT_FRAME_SIZE/2 + 1};
    AlignedBuffer sumPhase{MAX_FFT_FRAME_SIZE/2 + 1};
    double phaseExpected;

    AlignedBuffer outputAccumulator{2 * MAX_FFT_FRAME_SIZE};

    rack::dsp::ComplexFFT * pComplexFftEngine;
    float fSampleRateScalingFactor; 

    Random random;

    float fActiveSampleRate; 

	bool bRobot = false;
	dsp::SchmittTrigger robotTrigger;
    float fRobotGainAdjustment; 

    // --------------------------------------------------------------------
    // -- PITCH -- 

    enum PitchQuantizationEnum { 
        PITCH_QUANT_SMOOTH    = 0, 
        PITCH_QUANT_SEMITONES = 1, 
        PITCH_QUANT_FIFTHS    = 2, 
        PITCH_QUANT_OCTAVES   = 3 }; 
	
    dsp::SchmittTrigger pitchQuantizationTrigger;
    int pitchQuantization = PITCH_QUANT_SMOOTH;

    int iSemitoneShift = 0; 
    float fActivePitchShift;

    void selectNextPitchQuantization() { 
        setPitchQuantization((pitchQuantization + 1) & 0x03); // % 4
    }

    void setPitchQuantization(int quantization) {
        pitchQuantization = quantization;
        // set lights 
 		lights[PITCH_QUANT_SMOOTH_LED_LIGHT].value    = (pitchQuantization == PITCH_QUANT_SMOOTH);
		lights[PITCH_QUANT_SEMITONES_LED_LIGHT].value = (pitchQuantization == PITCH_QUANT_SEMITONES);
		lights[PITCH_QUANT_FIFTHS_LED_LIGHT].value    = (pitchQuantization == PITCH_QUANT_FIFTHS);
		lights[PITCH_QUANT_OCTAVE_LED_LIGHT].value    = (pitchQuantization == PITCH_QUANT_OCTAVES);
    }

    AlignedBuffer anaMagnitude{MAX_FFT_FRAME_SIZE};
    AlignedBuffer anaFrequency{MAX_FFT_FRAME_SIZE};

    AlignedBuffer synMagnitude{MAX_FFT_FRAME_SIZE};
    AlignedBuffer synFrequency{MAX_FFT_FRAME_SIZE};

    // -----------------------------------------





    // Frequency Isolation
    float fFreqNyquist;
    double dFreqPerBin; 

    float fMinExponent;     // 10^x for frequency in center of bin[0]
    float fCenterExponent;  // 10^x for frequency selected as center of freq range 
    float fMaxExponent;     // 10^x for frequency in center of bin[iFftFrameSize -1]
    float fExponentRange;   // maxExponent - minExponent 

    int iFreqLowerBinIndex;  // index of bin at lower end of freq range
    int iFreqUpperBinIndex;  // index of bin at upper end of freq range

    float fActiveFreqCenter;
    float fActiveFreqSpan;

    enum BandRangeEnum { 
        BAND_APPLY_INSIDE  = 0, 
        BAND_APPLY_OUTSIDE = 1, 
        BAND_APPLY_ALL     = 2 }; 

    enum ExtendedBandRangeEnum { 
        BAND_APPLY_INSIDE_ONLY  = 0, // Suppress outside
        BAND_APPLY_INSIDE_PLUS  = 1, 
        BAND_APPLY_OUTSIDE_ONLY = 2, // suppress inside 
        BAND_APPLY_OUTSIDE_PLUS = 3, 
        BAND_APPLY_FULL_RANGE   = 4 }; 

	dsp::SchmittTrigger blurBandSelectionTrigger;
	dsp::SchmittTrigger pitchBandSelectionTrigger;
	dsp::SchmittTrigger gainBandSelectionTrigger;

    int blurBandRange  = BAND_APPLY_ALL;
    int gainBandRange  = BAND_APPLY_ALL;
    int pitchBandRange = BAND_APPLY_FULL_RANGE;

    void selectNextBlurBandRange() { 
        setBlurBandRange(blurBandRange == 2 ? 0 : blurBandRange + 1);
    }

    void setBlurBandRange(int range) {
        blurBandRange = range;
        // set lights 
 		lights[BLUR_FREQ_INSIDE_LED_LIGHT].value  = (blurBandRange == BAND_APPLY_INSIDE);
 		lights[BLUR_FREQ_OUTSIDE_LED_LIGHT].value = (blurBandRange == BAND_APPLY_OUTSIDE);
 		lights[BLUR_FREQ_ALL_LED_LIGHT].value     = (blurBandRange == BAND_APPLY_ALL);
    }


    void selectNextPitchBandRange() { 
        setPitchBandRange(pitchBandRange == 4 ? 0 : pitchBandRange + 1);
    }

    void setPitchBandRange(int range) {
        pitchBandRange = range;
        // set lights 
 		//lights[PITCH_FREQ_INSIDE_EXCLUSIVE_LED_LIGHT].value  = (pitchBandRange == BAND_APPLY_INSIDE_ONLY || pitchBandRange == BAND_APPLY_INSIDE_PLUS);
 		//lights[PITCH_FREQ_INSIDE_LED_LIGHT].value            = (pitchBandRange == BAND_APPLY_INSIDE_PLUS);
 		//lights[PITCH_FREQ_OUTSIDE_EXCLUSIVE_LED_LIGHT].value = (pitchBandRange == BAND_APPLY_OUTSIDE_ONLY|| pitchBandRange == BAND_APPLY_OUTSIDE_PLUS);
 		//lights[PITCH_FREQ_OUTSIDE_LED_LIGHT].value           = (pitchBandRange == BAND_APPLY_OUTSIDE_PLUS);
 		//lights[PITCH_FREQ_ALL_LED_LIGHT].value               = (pitchBandRange == BAND_APPLY_FULL_RANGE);

 		lights[PITCH_FREQ_INSIDE_EXCLUSIVE_LED_LIGHT].value  = (pitchBandRange == BAND_APPLY_INSIDE_ONLY);
 		lights[PITCH_FREQ_INSIDE_LED_LIGHT].value            = (pitchBandRange == BAND_APPLY_INSIDE_PLUS);
 		lights[PITCH_FREQ_OUTSIDE_EXCLUSIVE_LED_LIGHT].value = (pitchBandRange == BAND_APPLY_OUTSIDE_ONLY);
 		lights[PITCH_FREQ_OUTSIDE_LED_LIGHT].value           = (pitchBandRange == BAND_APPLY_OUTSIDE_PLUS);
 		lights[PITCH_FREQ_ALL_LED_LIGHT].value               = (pitchBandRange == BAND_APPLY_FULL_RANGE);

    }

    void selectNextGainBandRange() { 
        setGainBandRange(gainBandRange == 2 ? 0 : gainBandRange + 1);
    }

    void setGainBandRange(int range) {
        gainBandRange = range;
        // set lights 
 		lights[GAIN_FREQ_INSIDE_LED_LIGHT].value  = (gainBandRange == BAND_APPLY_INSIDE);
 		lights[GAIN_FREQ_OUTSIDE_LED_LIGHT].value = (gainBandRange == BAND_APPLY_OUTSIDE);
 		lights[GAIN_FREQ_ALL_LED_LIGHT].value     = (gainBandRange == BAND_APPLY_ALL);
    }

    // --------------------------------------------------------------------------------
    //  Phase Reset - zero out phase history to clean up accumulated phase deviations 
    // --------------------------------------------------------------------------------

	dsp::SchmittTrigger phaseResetTrigger;

    // --------------------------------------------------------------------------------

    float fActiveOutputMix; 
    float fActiveDryGain; 
    float fActiveWetGain;
        
    // --------------------------------------------------------------------------------

    float fActiveBlurMix; 
    float fActiveBlurSpread; 

    void applyFftConfiguration() {
        bool bConfigurationRequired = false;

        // TODO: consider using single if (a or b or c) check 
        if (iSelectedFftFrameSize != iFftFrameSize) {
            bConfigurationRequired = true;
        } 

        if (iSelectedOversample != iOversample) {
            bConfigurationRequired = true;
        } 

        if (fSelectedSampleRate != fActiveSampleRate) {
            bConfigurationRequired = true;
        }

        if (bConfigurationRequired) {
            configureFftEngine(iSelectedFftFrameSize, iSelectedOversample, fSelectedSampleRate);
        }
    }

    void configureFftEngine(int frameSize, int oversample, float sampleRate) { 

        if (iFftFrameSize != frameSize) {
            iFftFrameSize = frameSize;
            if (pComplexFftEngine != NULL) {
                delete pComplexFftEngine;
            }
            pComplexFftEngine = new rack::dsp::ComplexFFT(iFftFrameSize);
        }

        iOversample = oversample;

        if (fActiveSampleRate != sampleRate) {
            fActiveSampleRate = sampleRate; 

            fFreqNyquist = fActiveSampleRate * 0.5;
            dFreqPerBin = fActiveSampleRate / double(iFftFrameSize); 

            fMinExponent = log10(dFreqPerBin);
            fMaxExponent = log10(fFreqNyquist);
            fExponentRange = fMaxExponent - fMinExponent;

            fSampleRateScalingFactor = DFLT_SAMPLE_RATE_SCALING_FACTOR;
            for (size_t k = 0; k < sizeof(scalingFactors)/sizeof(scalingFactors[0]); k++) {
                if (scalingFactors[k].sampleRate == fActiveSampleRate) {
                    fSampleRateScalingFactor = scalingFactors[k].scalingFactor;
                    break;
                }
            }
        }

        iSelectedFftFrameSize = iFftFrameSize;
        iSelectedOversample = iOversample;
        fSelectedSampleRate = fActiveSampleRate; 

        iStepSize = iFftFrameSize / iOversample;
        iLatency = iFftFrameSize - iStepSize;
        iInIndex = iLatency; 
        phaseExpected = 2.* M_PI * double(iStepSize)/double(iFftFrameSize);

        window.makeWindow(frameSize);
        inBuffer.clear();
        outBuffer.clear();
        outputAccumulator.clear();
        fftFrameHistory.deleteMembers();
        fftFramePool.deleteMembers();
        fftWorkspace.clear();
        lastPhase.clear();
        sumPhase.clear();

        fRobotGainAdjustment = gainForDb(4.8f); 

        //DEBUG("--- Initialize FFT ---");
        //DEBUG("inBuffer.size() = %d", inBuffer.size());
        //DEBUG("outBuffer.size() = %d", outBuffer.size());
        //DEBUG("FftFrameHistory capacity = %d", fftFrameHistory.getCapacity());
        //DEBUG("FftFrameHistory available = %d", fftFrameHistory.getAvailable());
        ////DEBUG("phaseExpected = %f", phaseExpected);
        //DEBUG(" fftTemp,   size %d, numBins %d, address %p", fftTemp.size(), fftTemp.numBins(), fftTemp.values);
        //DEBUG(" InBuffer,  size %d, address %p", inBuffer.size(), inBuffer.values);
        //DEBUG(" OutBuffer, size %d, address %p", outBuffer.size(), outBuffer.values);
        //DEBUG(" Sample Rate = %f", fActiveSampleRate);
    }

    // if sample rate changes, drain the history - the values are likely to be noise at the new sample rate 
    // if the length changes, calculate a new max length and drain just enough frames to leave 'length' number in the history
    void adjustFrameHistoryLength() {

        float fFramesPerSecond = (float(iOversample) * fActiveSampleRate) / float(iFftFrameSize);
        if (fFramesPerSecond < 1.f) {
            fFramesPerSecond = 1.f;
        }

        float fHistoryLength = params[HISTORY_LENGTH_PARAM].getValue();
        fHistoryLength += params[HISTORY_ATTENUVERTER_PARAM].getValue() * getCvInput(HISTORY_CV_INPUT);    
        fHistoryLength = clamp(fHistoryLength, 0.f, 1.f);

        float fFrameCountLimit = std::min(MAX_HISTORY_SECONDS * fFramesPerSecond, float(fftFrameHistory.getCapacity()));   
        
        iMaxHistoryFrames = myCeil(fHistoryLength * fFrameCountLimit);
        iMaxHistoryFrames = clamp(iMaxHistoryFrames, MIN_HISTORY_FRAMES, fftFrameHistory.getCapacity());

        fHistoryLengthSeconds = float(iMaxHistoryFrames) / fFramesPerSecond;

        // Siphon frames from the History to the Pool until the number of history frames
        // is less than or equal to the max
        while (fftFrameHistory.numMembers() > iMaxHistoryFrames) {
            FftFrame *pFftFrame = fftFrameHistory.deQueue();
            fftFramePool.pushTail(pFftFrame);
        }

        // Fill history with enough empty frames to reach max history length
        while (fftFrameHistory.numMembers() < iMaxHistoryFrames) {
            FftFrame * pFftFrame = fftFramePool.popFront();
            if (pFftFrame == NULL) {
                pFftFrame = new FftFrame(iFftFrameSize);
            }
            pFftFrame->clear();
            fftFrameHistory.enQueue(pFftFrame);
        }

        // if the max history frames exceeds the initial capacity then
        // shrink the actual number of seconds displayed in the tooltip  
        // at 44100.0 with oversample 1 and frames size of 1 the frames-per-second is 44100.0 
        // at 44100.0 with oversample 8 and frame size of 2048 the frames-per-second is 721.9 
        //
        // Clearly the extreme FFT settings and sample rates would swamp memory very quickly
        // - limit the history buffer length to accomodate the extreme settings
    }

    void configureFftEngine_default() {
        iSelectedFftFrameSize = DFLT_FFT_FRAME_SIZE;
        iSelectedOversample = 4;
        fSelectedSampleRate = 44100.0;

        configureFftEngine(iSelectedFftFrameSize, iSelectedOversample, fSelectedSampleRate);
    }

    void initialize() { 
        configureFftEngine_default();
        setPitchQuantization(PITCH_QUANT_SMOOTH);  // TODO: make constant for this 
        setBlurBandRange(BAND_APPLY_INSIDE);    // TODO: make DFLT constants for these 
        setPitchBandRange(BAND_APPLY_FULL_RANGE);
        setGainBandRange(BAND_APPLY_ALL);
        fActivePitchShift = 1.0; // unity 
        fFrameDropProbability = 0.f; 
        bFreeze = false; // drop all incoming frames when buffer is frozen

        // these will be computed on the first call into process
        iFreqLowerBinIndex = 0;  
        iFreqUpperBinIndex = 0;
        fActiveFreqCenter = -1.f;  // force a recalculation
        fActiveFreqSpan = -1.f;    // force a recalculation
        updateFreqRange();

        fActiveOutputMix = -999.f;  // force a recalculation 
    }

	Blur() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam<LengthParamQuantity>(HISTORY_LENGTH_PARAM, 0.f, 1.f, 0.5f, "Audio History");
		configParam(HISTORY_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Audio History Attenuverter");
		configParam<FreqCenterParamQuantity>(FREQ_CENTER_PARAM, 0.f, 1.f, 0.5f, "Freq Band Center");
		configParam(FREQ_CENTER_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Freq Center Attenuverter");
		configParam(POSITION_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Position Attenuverter");
		configParam<PositionParamQuantity>(POSITION_PARAM, 0.f, 1.f, 0.f, "Playback Position");
		configParam<FreqWidthParamQuantity>(FREQ_SPAN_PARAM, 0.f, 1.f, 1.f, "Freq Band Width");
		configParam(FREQ_SPAN_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Freq Span Attenuverter");
		configParam(FRAME_DROP_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Frame Drop Attenuverter");
		configParam<FrameDropParamQuantity>(FRAME_DROP_PARAM, 0.f, 1.f, 0.f, "Frame Drop Prob");
		configParam<FreezeParamQuantity>(FREEZE_PARAM, 0.f, 1.f, 0.f, "Freeze");
		configParam<BlurFreqSelectorParamQuantity>(BLUR_FREQ_SELECTOR_PARAM, 0.f, 1.f, 0.f, "Blur Freq");
		configParam<PitchFreqSelectorParamQuantity>(PITCH_FREQ_SELECTOR_PARAM, 0.f, 1.f, 0.f, "Pitch Freq");
		configParam<GainFreqSelectorParamQuantity>(GAIN_FREQ_SELECTOR_PARAM, 0.f, 1.f, 0.f, "Gain Freq");
		configParam(BLUR_SPREAD_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Blur Spread Attenuverter");
		configParam<BlurSpreadParamQuantity>(BLUR_SPREAD_PARAM, 0.f, 1.f, 0.f, "Blur Spread");
		configParam<PitchShiftParamQuantity>(PITCH_PARAM, 0.f, 1.f, 0.5f, "Pitch");
		configParam(PITCH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Pitch Attenuverter");
		configParam<RobotParamQuantity>(ROBOT_PARAM, 0.f, 1.f, 0.f, "Robot");
		configParam<PitchQuantizeSelectorParamQuantity>(PITCH_QUANTIZE_PARAM, 0.f, 1.f, 0.f, "Pitch Quantize");
		configParam(BLUR_MIX_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Blur Mix Attenuverter");
		configParam(BLUR_MIX_PARAM, 0.f, 1.f, 1.f, "Blur Mix", "%", 0.f, 100.f);
		configParam(GAIN_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Gain Attenuverter");
		configParam<GainParamQuantity>(GAIN_PARAM, 0.f, 1.f, 0.5f, "Gain");
		configParam(PHASE_RESET_PARAM, 0.f, 1.f, 0.f, "Phase Reset");
		configParam<MixParamQuantity>(MIX_PARAM, 0.f, 1.f, 1.f, "Wet/Dry Mix");
		configParam(MIX_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Mix Attenuverter");
		
        pComplexFftEngine = NULL;
        initialize();
	}

    ~Blur() {
        fftFramePool.deleteMembers();
        fftFrameHistory.deleteMembers();
        delete pComplexFftEngine;
    }

	virtual json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "fftFrameSize", json_integer(iFftFrameSize));
		json_object_set_new(rootJ, "fftOversample", json_integer(iOversample));
		return rootJ;
	}

	virtual void dataFromJson(json_t* rootJ) override {
		json_t* jsonValue;
        
        jsonValue = json_object_get(rootJ, "fftFrameSize");
		if (jsonValue)
			iFftFrameSize = json_integer_value(jsonValue);

		jsonValue = json_object_get(rootJ, "fftOversample");
		if (jsonValue)
			iOversample = json_integer_value(jsonValue);
	}

    void resetPhaseHistory() { 
        lastPhase.clear();
        sumPhase.clear();
    }

// From Module parent class 
//    /** Called when the engine sample rate is changed. */
//    virtual void onSampleRateChange() override { }
//    
    /** Called when user clicks Initialize in the module context menu. */
    virtual void onReset() override {

        // TODO: this code is amost identical to initialize()
        //  call one of the other from the other

        configureFftEngine_default();
        
        setPitchQuantization(PITCH_QUANT_SMOOTH);
        setBlurBandRange(BAND_APPLY_INSIDE);  // TODO: make DFLT constants for these 
        setPitchBandRange(BAND_APPLY_FULL_RANGE);
        setGainBandRange(BAND_APPLY_ALL);
        pitchQuantizationTrigger.reset();
        blurBandSelectionTrigger.reset();
	    pitchBandSelectionTrigger.reset();
	    gainBandSelectionTrigger.reset();
        phaseResetTrigger.reset();

        bRobot = false;
        robotTrigger.reset();
        //bSemitone = false;

        bFreeze = false;
        freezeTrigger.reset();

        fActivePitchShift = 1.0; // unity 
        fActiveOutputMix = 999.0; // force a recalculation
        fActiveBlurMix = 1.0; 
    }
    
//    /** Called when user clicks Randomize in the module context menu. */
//    virtual void onRandomize() override {}
//    
//    /** Called when the Module is added to the Engine */
//    virtual void onAdd() override {}
//    
//    /** Called when the Module is removed from the Engine */
//    virtual void onRemove() override {}

    // Convert [0,10] range to [0,1]
    // Return 0 if not connected 
    inline float getCvInput(int input_id) {
        if (inputs[input_id].isConnected()) {
            return (inputs[input_id].getVoltage() / 10.0);
        }
        return 0.f;
    }


    virtual void process(const ProcessArgs& args) override {
        fSelectedSampleRate = args.sampleRate;

        float in = inputs[AUDIO_IN_INPUT].getVoltageSum();

        inBuffer.values[iInIndex] = in;
        float out = outBuffer.values[ iInIndex - iLatency ];

        // TODO: move to separate method
        float fMix = params[MIX_PARAM].getValue();
        fMix += params[MIX_ATTENUVERTER_PARAM].getValue() * getCvInput(GAIN_CV_INPUT);    
        fMix = clamp(fMix, 0.f, 1.f);
        if (fMix != fActiveOutputMix) {
            fActiveOutputMix = fMix; 
            fMix = (fMix -0.5f) * 2.f; // convert to [-1,1]
            fActiveDryGain = sqrt(0.5f * (1. - fMix)); // equal power law
            fActiveWetGain = sqrt(0.5f * (1. + fMix)); // equal power law
        }        
        
        outputs[AUDIO_OUT_OUTPUT].setVoltage( (in * fActiveDryGain) + (out * fActiveWetGain) );

		if (pitchQuantizationTrigger.process(params[PITCH_QUANTIZE_PARAM].getValue())) {
            selectNextPitchQuantization();
		}

		if (blurBandSelectionTrigger.process(params[BLUR_FREQ_SELECTOR_PARAM].getValue())) {
            selectNextBlurBandRange();
		}

		if (pitchBandSelectionTrigger.process(params[PITCH_FREQ_SELECTOR_PARAM].getValue())) {
            selectNextPitchBandRange();
		}

		if (gainBandSelectionTrigger.process(params[GAIN_FREQ_SELECTOR_PARAM].getValue())) {
            selectNextGainBandRange();
		}

		if (robotTrigger.process(params[ROBOT_PARAM].getValue() + inputs[ROBOT_CV_INPUT].getVoltage())) {
			bRobot = !bRobot;
		}

		if (freezeTrigger.process(params[FREEZE_PARAM].getValue() + inputs[FREEZE_CV_INPUT].getVoltage())) {
			bFreeze = !bFreeze;
		}

        iInIndex++;
        if (iInIndex >= iFftFrameSize) {
            iInIndex = iLatency;
            if (phaseResetTrigger.process(params[PHASE_RESET_PARAM].getValue())) {
                resetPhaseHistory();
            }
            updateFreqRange();
            updateGain();
            applyFftConfiguration();
            adjustFrameHistoryLength();
            processFrame();
        }

   		lights[ROBOT_LED_LIGHT].value = bRobot;
        lights[FREEZE_LED_LIGHT].value = bFreeze;

        //lights[PITCH_QUANT_SELECTOR_LED_LIGHT].value = pitchQuantLed;

        // TODO: update lights only when selection changes            
   		// lights[SEMITONE_LED_LIGHT].value = bSemitone;
    }

    void updateFreqRange() {
        // TODO: move this outside the frame process ... in the main process() but only recalc 
        //  if the current values != active values 
        //      float fActiveFreqCenter
        //      float fActiveFreqSpan

        // EXPENSIVE PROCESS -- try to move some or all into the frame process section

        float fFreqCenter = params[FREQ_CENTER_PARAM].getValue();
        float fFreqSpan = params[FREQ_SPAN_PARAM].getValue();

        fFreqSpan += params[FREQ_SPAN_ATTENUVERTER_PARAM].getValue() * getCvInput(FREQ_SPAN_CV_INPUT);    
        fFreqSpan = clamp(fFreqSpan, 0.f, 1.f);

        fFreqCenter += params[FREQ_CENTER_ATTENUVERTER_PARAM].getValue() * getCvInput(FREQ_CENTER_CV_INPUT);    
        fFreqCenter = clamp(fFreqCenter, 0.f, 1.f);

        if (fFreqCenter == fActiveFreqCenter && fFreqSpan == fActiveFreqSpan) {
            return; 
        }

        fCenterExponent = fMinExponent + (fFreqCenter * fExponentRange);
        fFreqSpan *= fExponentRange;

        //DEBUG(" - Frequency Range: minExponent %f", fMinExponent);
        //DEBUG("   Frequency Range: maxExponent %f", fMaxExponent);
        //DEBUG("   Frequency Range: centerExponent %f", fCenterExponent);

        // Slide the span range to always be inside the 0..Nyquist range
        float lowerExponent = fCenterExponent - (fFreqSpan * 0.5);
        if (lowerExponent < fMinExponent) {
            lowerExponent = fMinExponent;            
        }

        float upperExponent = lowerExponent + fFreqSpan;
        if (upperExponent > fMaxExponent) {
            upperExponent = fMaxExponent;
            lowerExponent = upperExponent - fFreqSpan;
        }

        //DEBUG("      lowerExponent %f", lowerExponent);
        //DEBUG("      upperExponent %f", upperExponent);

        // compute bin indexes
        float lowerFreq = pow(10.f, lowerExponent);
        float upperFreq = pow(10.f, upperExponent);

        //DEBUG("      lowerFreq %f", lowerFreq);
        //DEBUG("      upperFreq %f", upperFreq);

        iFreqLowerBinIndex = myFloor(lowerFreq / dFreqPerBin);
        iFreqUpperBinIndex = myFloor(upperFreq / dFreqPerBin);
        iFreqLowerBinIndex = clamp(iFreqLowerBinIndex, 0, iFftFrameSize/2 + 1);
        iFreqUpperBinIndex = clamp(iFreqUpperBinIndex, 0, iFftFrameSize/2 + 1);

        // Tooltips
        fFreqCenterHz = pow(10.f, fCenterExponent); ///  - (dFreqPerBin * 0.5f); // b
        fFreqLowerHz  = float(iFreqLowerBinIndex) * dFreqPerBin;
        fFreqUpperHz  = float(iFreqUpperBinIndex) * dFreqPerBin;

        //DEBUG("   minExponent, maxExponent = (%f, %f)", minExponent, maxExponent);
        //DEBUG("   lowerExponent, upperExponent = %f, %f", lowerExponent, upperExponent);
        //DEBUG("   lowerFreq, upperFreq = %f, %f", lowerFreq, upperFreq);
        //DEBUG("   lowerBinIndex, upperBinIndex = %d, %d", iLowerBinIndex, iUpperBinIndex);
    }

    void updateGain() {
        float fGain = params[GAIN_PARAM].getValue();
        fGain += params[GAIN_ATTENUVERTER_PARAM].getValue() * getCvInput(GAIN_CV_INPUT);    
        fGain = clamp(fGain, 0.f, 1.f);
        if (fGain != fSelectedOutputGain) { 
            fSelectedOutputGain = fGain;
            fGain = (fGain - 0.5) * 2; // convert to [-1,1]
            if (fGain < 0.f) {
                fOutputGain = pow(10.f, fGain * 6.f); // -60 db; 
            } else {
                fOutputGain = pow(10.f, fGain * .6f); // +6 db; 
            }
        }
    }

    void processFrame() {

        float fFrameDrop = params[FRAME_DROP_PARAM].getValue();

        fFrameDrop += params[FRAME_DROP_ATTENUVERTER_PARAM].getValue() * getCvInput(FRAME_DROP_CV_INPUT);    
        fFrameDrop = clamp(fFrameDrop, 0.f, 1.f);

        fFrameDropProbability = fFrameDrop;

        bool bAcceptNewFrame = (!bFreeze) && (random.generateZeroToOne() > fFrameDropProbability); 

        if (bAcceptNewFrame) {
            // Apply the window 
            for (int k = 0; k < iFftFrameSize; k++) {
                fftWorkspace.values[2*k] = inBuffer.values[k] * window.values[k];
                fftWorkspace.values[2*k+1] = 0.;
            }
            
            // TODO: change order to ensure that frames are held somewhere 
            // prevents memory leak if stopped midway
            //   transfer history oldest to pool
            //   allocate new frame
            //   enqueue to history
            //   perform fft to populate new frame

            // Allocate a FFT Frame
            FftFrame * pFftFrame = fftFramePool.popFront();
            if (pFftFrame == NULL)
            {
                pFftFrame = new FftFrame(iFftFrameSize);
            }

            // Run FFT Forward 
            pComplexFftEngine->fft(fftWorkspace.values, pFftFrame->values);

            // Remove DC component            
            pFftFrame->values[0] = 0.;

            // Convert Complex real/imag to magnitude/phase
            for (int k = 0; k <= iFftFrameSize/2; k++)
            {
                //if (k <= 2) {
                //    DEBUG(" - after analysis: bin[%d] is %f, %f", k, pFftFrame->values[2*k], pFftFrame->values[2*k+1]);
                //}

                double real = pFftFrame->values[2*k];
                double imag = pFftFrame->values[2*k+1];

                double magnitude = 2. * sqrt(real*real + imag*imag);
                double phase = atan2(imag,real);

                pFftFrame->values[2*k] = magnitude;
                pFftFrame->values[2*k+1] = phase;
            }

            //DEBUG(" inbound frame: history (population, max) = (%d, %d)", fftFrameHistory.numMembers(), iMaxHistoryFrames);

            // Push the frame into the history list
            // Purge older frames
            while (fftFrameHistory.numMembers() >= iMaxHistoryFrames) {
                FftFrame * pOldFrame = fftFrameHistory.deQueue();
                fftFramePool.pushTail(pOldFrame);
            }
            fftFrameHistory.enQueue(pFftFrame);
            //DEBUG("      enqueued: history (population, max) = (%d, %d)", fftFrameHistory.numMembers(), iMaxHistoryFrames);
        }

        fillOutputFrame(fftTemp);
        applyPitchShift(fftTemp);
        
        // Convert Magnitude/Phase to Real/Imag complex pair
        for (int k = 0; k <= iFftFrameSize/2; k++)
        {
            double magnitude = fftTemp.values[2*k];
            double phase = fftTemp.values[2*k+1];

            // phase = wrapPlusMinusPi(phase);

            // Apply frequency band constraints 
            // Disable gain for bins inside or outside the freq range 
            // depending on the range mode
            float fGain = fOutputGain;
            if (gainBandRange == BAND_APPLY_INSIDE) {
                if (k < iFreqLowerBinIndex || k > iFreqUpperBinIndex) {
                    fGain = 1.0; // unity
                }
            }
            else if (gainBandRange == BAND_APPLY_OUTSIDE) {
                if (k >= iFreqLowerBinIndex && k <= iFreqUpperBinIndex) {
                    fGain = 1.0; // unity
                }
            }

            fftTemp.values[2*k]   = magnitude*cos(phase) * fGain;
            fftTemp.values[2*k+1] = magnitude*sin(phase) * fGain;
        }

        // DEBUG("- after Magn/Phase conversion -");

        // Run FFT Backward 
        // Apply scale
        // zero negative frequencies
        for (int k = iFftFrameSize+2; k < 2*iFftFrameSize; k++) {
            fftTemp.values[k] = 0.;
        }
        pComplexFftEngine->ifft(fftTemp.values, fftWorkspace.values);
        //pComplexFftEngine->scale(fftWorkspace.values);

        // Do windowing and add to output accumulator
        double scale = 1.0 / double((iFftFrameSize/2) * iOversample);
        scale *= fSampleRateScalingFactor;
        for(int k = 0; k < iFftFrameSize; k++) {
            float fOutValue = (fftWorkspace.values[2*k] * scale * window.values[k]);
            outputAccumulator.values[k] += fOutValue;
        }

        // Copy ready values to output buffer 
        for (int k = 0; k < iStepSize; k++) {
            outBuffer.values[k] = outputAccumulator.values[k];
        }

        // Shift the accumulator 
        memmove(outputAccumulator.values, outputAccumulator.values + iStepSize, iFftFrameSize*sizeof(float));

        // Move the input FIFO 
        for (int k = 0; k < iLatency; k++) {
            inBuffer.values[k] = inBuffer.values[k+iStepSize];
        }
    }

    void fillOutputFrame(FftFrame & outputFrame) {

        int iNumFrames = fftFrameHistory.numMembers();
        if (iNumFrames <= 0) {
            outputFrame.clear();
            return;
        }

        float fMaxFrameIndex = float(iNumFrames - 1);
        //DEBUG("-- max frame index = %f", fMaxFrameIndex);
        
        float fPosition = params[POSITION_PARAM].getValue();
        float fSpread = params[BLUR_SPREAD_PARAM].getValue();
        float fBlurMix = params[BLUR_MIX_PARAM].getValue();
               
        fPosition += params[POSITION_ATTENUVERTER_PARAM].getValue() * getCvInput(POSITION_CV_INPUT);    
        fPosition = clamp(fPosition, 0.f, 1.f);

        // Blur Spread 
        fSpread += params[BLUR_SPREAD_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_SPREAD_CV_INPUT);    
        fSpread = clamp(fSpread, 0.f, 1.f);
        fSpread = std::pow(fSpread, 3.f);
        fSpread *= fMaxFrameIndex;

        // Blur Mix 
        fBlurMix += params[BLUR_MIX_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_MIX_CV_INPUT);    
        fBlurMix = clamp(fBlurMix, 0.f, 1.f);

        // for tooltips
        fActiveBlurMix = fBlurMix; 
        fActiveBlurSpread = fSpread;
        fPositionSeconds = fPosition * fHistoryLengthSeconds;


        float fCursor = fPosition * fMaxFrameIndex;

        int iIndexDry = myFloor(fCursor);
        FftFrame * pFrameDry = fftFrameHistory.peekAt(iIndexDry);

        //DEBUG("-- Position = %f", fPosition);
        //DEBUG("   Spread   = %f", fSpread);
        //DEBUG("   Cursor   = %f", fCursor);
        //DEBUG("   History Buffer: num members = %d", fftFrameHistory.numMembers());
        //DEBUG("                   rear  = %d", fftFrameHistory.rear);
        //DEBUG("                   front = %d", fftFrameHistory.front);
        //DEBUG("              population = %d", fftFrameHistory.population);
        //DEBUG("-- outputFrame (size, numBins) = (%d, %d)", outputFrame.size(), outputFrame.numBins());
        //DEBUG("   Index Dry = %d", iIndexDry);
        //DEBUG("   pFrameDry = 0x%p", pFrameDry);
        //DEBUG("   Index Before, After, Dry, Max  = (%d, %d, %d, %f)", r  = %d", fftFrameHistory.rear);

        for (int k = 0; k <= iFftFrameSize/2; k++) {

            // Slide the spread range to always be inside the range of frames in the frame history
            float fSelected = fCursor + (fSpread  * random.generatePlusMinusOne()); 
            if (fSelected < 0.f) {
                fSelected += fSpread;
            }
            else if (fSelected > fMaxFrameIndex) {
                fSelected -= fMaxFrameIndex;
            }

            // Apply frequency band constraints 
            // Disable spread for bins inside or outside the freq range 
            // depending on the range mode
            if (blurBandRange == BAND_APPLY_INSIDE) {
                if (k < iFreqLowerBinIndex || k > iFreqUpperBinIndex) {
                    fSelected = fCursor; // no spread 
                }
            }
            else if (blurBandRange == BAND_APPLY_OUTSIDE) {
                if (k >= iFreqLowerBinIndex && k <= iFreqUpperBinIndex) {
                    fSelected = fCursor; // no spread 
                }
            }

            int iIndexBefore = myFloor(fSelected);
            int iIndexAfter = iIndexBefore + 1;

            if (iIndexBefore < 0) {
                iIndexBefore = 0;
            }
            
            if (iIndexAfter >= iNumFrames) {
                iIndexAfter = iNumFrames - 1;
            }
            
            //if (k < 8) {
            //    DEBUG("  [%d] ", k);
            //    DEBUG("       Selected = %f", fSelected);
            //    DEBUG("       index (before, after) = (%d, %d)", iIndexBefore, iIndexAfter);
            //}

            FftFrame * pFrameBefore = fftFrameHistory.peekAt(iIndexBefore);
            FftFrame * pFrameAfter = fftFrameHistory.peekAt(iIndexAfter);

            //DEBUG("  [%d] pFrameBefore = 0x%p", k, pFrameBefore);
            //DEBUG("  [%d] pFrameAfter  = 0x%p", k, pFrameAfter);

            float fSelectedFractional = fSelected - float(myFloor(fSelected));

            // Linear interpolate: a + t(b-a) where t is [0,1]
            float fValueBefore = pFrameBefore->values[2*k];
            float fValueAfter = pFrameAfter->values[2*k];        
            double magn = fValueBefore + fSelectedFractional * (fValueAfter - fValueBefore);

            float fPhaseBefore = pFrameBefore->values[2*k+1];
            float fPhaseAfter = pFrameAfter->values[2*k+1];
            double phase = fPhaseBefore + fSelectedFractional * (fPhaseAfter - fPhaseBefore);

            if (bRobot) {
                phase = 0.f;
                magn *= fRobotGainAdjustment; 
            }

            float fDryMagnitude = pFrameDry->values[2*k];
            float fDryPhase = pFrameDry->values[2*k+1];

            // Apply Blur Mix    
            magn  = (magn * fBlurMix) + (fDryMagnitude * (1.0 - fBlurMix));
            phase = (phase * fBlurMix) + (fDryPhase * (1.0 - fBlurMix));            
        
            outputFrame.values[2*k] = magn;
            outputFrame.values[2*k+1] = phase;            
        }

/** EXPERIMENT -- shift/rotate the bins **
 * This works - but this might not be the place to do the shifting
        float fShiftAmount = (params[PITCH_PARAM].getValue() - 0.5f) * 2.f * float(iFftFrameSize/2);
        shiftFrame(outputFrame, int(shiftAmount))
** end EXPERIMENT **/

    }

    void applyPitchShift(FftFrame & outputFrame) {

        float fPitchShift = params[PITCH_PARAM].getValue();
        fPitchShift += params[PITCH_ATTENUVERTER_PARAM].getValue() * getCvInput(PITCH_CV_INPUT);    
        fPitchShift = clamp(fPitchShift, 0.f, 1.f);
        
        if (pitchQuantization == PITCH_QUANT_SMOOTH) {
            fActivePitchShift = fPitchShift *= 2.f;  // 0 .. 1 .. 2    
            //if (fPitchShift > 1.0) {
            //    fPitchShift = 1.0 + ((fPitchShift - 1.f) * 3.f);  // 0 .. 1..2..3    
            //}
        } else { 

            // apply quantization 
            // if current semitones not valid for the selected quant type 
            // then leave fActivePitchShift unchanged 

            // convert [-1..1] -> [-36..36]
            fPitchShift = (fPitchShift - 0.5f) * 2.f;  // -1..1
            int candidateSemitoneShift = int(fPitchShift * 36.0); 

            // Quantize
            if (pitchQuantization == PITCH_QUANT_SEMITONES) {
                iSemitoneShift = candidateSemitoneShift; 
                fActivePitchShift = pow(2.0, double(candidateSemitoneShift)/12.0);
            }
            else if (pitchQuantization == PITCH_QUANT_FIFTHS) {
                if (isFifthSemitone(candidateSemitoneShift)) {
                    iSemitoneShift = candidateSemitoneShift; 
                    fActivePitchShift = pow(2.0, double(candidateSemitoneShift)/12.0);
                }
            }
            else if (pitchQuantization == PITCH_QUANT_OCTAVES) {
                if (isOctaveSemitone(candidateSemitoneShift)) {
                    iSemitoneShift = candidateSemitoneShift; 
                    fActivePitchShift = pow(2.0, double(candidateSemitoneShift)/12.0);
                }
            }
        }

        //DEBUG(" - Pitch Quant Mode = %d", pitchQuantization);
        //DEBUG("   Semitones        = %d", iSemitoneShift);
        //DEBUG("   pitsh shift      = %f", fActivePitchShift);

        if (fActivePitchShift == 1.0) {
            return;
        }

        for (int k = 0; k <= iFftFrameSize/2; k++) {

            float magn = outputFrame.values[2*k];
            float phase = outputFrame.values[2*k+1];
            
            // compute the phase delta
            double tmp = phase - lastPhase.values[k];
            lastPhase.values[k] = phase;

            // subtract expected phase difference
            tmp -= float(k) * phaseExpected;

            // map to +/- Pi 
            tmp = wrapPlusMinusPi(tmp);

            // get deviation from bin frequency
            tmp = (double(iOversample) * tmp) / (2.0 * M_PI);

            // compute the k'th partial's true frequency
            tmp = double(k)*dFreqPerBin + tmp*dFreqPerBin;

            anaMagnitude.values[k] = magn;  // magnitude
            anaFrequency.values[k] = tmp;   // frequency
        }

        // I think this is a mistake - some leftover code after adding the 5-way pitch mode selector
        // but it works well as coded, so it stays in  
        bool bPitchSuppressUnselected = true;

    	synMagnitude.clear();
        synFrequency.clear();
        for (int k = 0; k <= iFftFrameSize/2; k++) { 

            // Apply frequency band constraints 
            // Disable shift for bins inside or outside the freq range 
            // depending on the range mode

            float fPitchShift = fActivePitchShift;
            if (pitchBandRange == BAND_APPLY_INSIDE) {
                if (k < iFreqLowerBinIndex || k > iFreqUpperBinIndex) {
                    if (bPitchSuppressUnselected) {
                        continue;
                    }
                    fPitchShift = 1.0;
                }
            }
            else if (pitchBandRange == BAND_APPLY_OUTSIDE) {
                if (k >= iFreqLowerBinIndex && k <= iFreqUpperBinIndex) {
                    if (bPitchSuppressUnselected) {
                        continue;
                    }
                    fPitchShift = 1.0;
                }
            }


            if (pitchBandRange != BAND_APPLY_FULL_RANGE) {
                bool isInside = (k >= iFreqLowerBinIndex && k <= iFreqUpperBinIndex);
                if (pitchBandRange == BAND_APPLY_INSIDE_ONLY) {
                    if (!isInside) {
                        continue; // suppress this frequency 
                    }
                }
                else if (pitchBandRange == BAND_APPLY_INSIDE_PLUS) {
                    if (!isInside) {
                        fPitchShift = 1.0; // leave frequency unshifted 
                    }
                }
                else if (pitchBandRange == BAND_APPLY_OUTSIDE_ONLY) {
                    if (isInside) {
                        continue;
                    }
                }
                else if (pitchBandRange == BAND_APPLY_OUTSIDE_PLUS) {
                    if (isInside) {
                        fPitchShift = 1.0; // leave frequency unshifted 
                    }
                }
            }

			int index = k * fPitchShift;

        	if (index <= iFftFrameSize/2) { 
				synMagnitude.values[index] += anaMagnitude.values[k];
				synFrequency.values[index] = anaFrequency.values[k] * fPitchShift; 
		} 

/*** TODO: Add option and controls for this frequency "Butterfly" effect ***   
			int index = k*fPitchShift;
            if (k <= 50)
            {
                index = k*(0.-fPitchShift);
            }
			if (index <= iFftFrameSize/2) { 
				synMagnitude.values[index] += anaMagnitude.values[k];
				synFrequency.values[index] = anaFrequency.values[k] * fPitchShift; 
			}
**/             
		}

        // Synthesize the shifted pitches into outbound FFT frame 
        for (int k = 0; k <= iFftFrameSize/2; k++) { 

            double magn = synMagnitude.values[k]; // magnitude
            double tmp = synFrequency.values[k];  // true frequency 

            // subtract bin mid frequency 
            tmp -= double(k) * dFreqPerBin;

            // get bin deviation from freq deviation
            tmp /= dFreqPerBin;

            // take oversample into account
            tmp = 2.0 * M_PI * tmp / double(iOversample);

            // add the overlap phase advance back in 
            tmp += double(k) * phaseExpected;

            // accumulate delta phase to get bin phase
            sumPhase.values[k] += tmp;
            double phase = sumPhase.values[k];

            outputFrame.values[2*k] = magn;
            outputFrame.values[2*k+1] = phase;            
        }        

    }

    // TODO: Future - this is not called yet 
    void shiftFrame(FftFrame & sourceFrame, int shiftAmount) {

        if (shiftAmount == 0) {
            return;
        }

        FftFrame * pShiftedFrame = fftFramePool.popFront();
        if (pShiftedFrame == NULL) {
            pShiftedFrame = new FftFrame(iFftFrameSize);
        }
        pShiftedFrame->clear();

        // leave bins 0 and fftFrameSize/2 alone 
        for (int k = 1; k < iFftFrameSize/2; k++) {
            int targetBin = k + shiftAmount;

            // Discard over/under shoots
            if (targetBin > 0 && targetBin < iFftFrameSize/2) {
                pShiftedFrame->values[2*targetBin] += sourceFrame.values[2*k];
                pShiftedFrame->values[2*targetBin+1] += sourceFrame.values[2*k+1];
            }
        }

        for (int k = 1; k < iFftFrameSize/2; k++) {
            sourceFrame.values[2*k] = pShiftedFrame->values[2*k];
            sourceFrame.values[2*k+1] = pShiftedFrame->values[2*k+1];
        }

        fftFramePool.pushFront(pShiftedFrame);
    }
};



// -------------------------------------------------------
//  Tooltips 
// -------------------------------------------------------

const std::string bandRangeNames[3] = {
    "Inside", 
    "Outside",
    "All"
};

const std::string extendedBandRangeNames[5] = {
    "Inside Only (Suppress Outside)", 
    "Inside", 
    "Outside Only (Suppress Inside)",
    "Outside",
    "All"
};

const std::string pitchQuantizeNames[4] = {
    "Smooth",
    "Semitones",
    "Fifths",
    "Octaves" 
};

struct BlurFreqSelectorParamQuantity : ParamQuantity {

	std::string getDisplayValueString() override {
        return bandRangeNames[ ((Blur*)module)->blurBandRange ];
	}
};

struct BlurSpreadParamQuantity : ParamQuantity {
    char formattedValue[24];

	std::string getDisplayValueString() override {
        float spread =  ((Blur*)module)->fActiveBlurSpread; // # frames 
        int numFrames = ((Blur*)module)->iMaxHistoryFrames;
        float historySeconds = ((Blur*)module)->fHistoryLengthSeconds;

        float spreadPercent = numFrames > 0 ? spread / float(numFrames) : 0.f;

        sprintf(formattedValue, "%.4f seconds", spreadPercent * historySeconds);
        return formattedValue;
    }
};

struct GainFreqSelectorParamQuantity : ParamQuantity {

	std::string getDisplayValueString() override {
        return bandRangeNames[ ((Blur*)module)->gainBandRange ];
	}
};

struct PitchFreqSelectorParamQuantity : ParamQuantity {

	std::string getDisplayValueString() override {
        return extendedBandRangeNames[ ((Blur*)module)->pitchBandRange ];
	}
};

struct PitchQuantizeSelectorParamQuantity : ParamQuantity {

	std::string getDisplayValueString() override {
        return pitchQuantizeNames[ ((Blur*)module)->pitchQuantization ];
	}
};


struct FreqCenterParamQuantity : ParamQuantity {
    char formattedValue[24];

	std::string getDisplayValueString() override {
        float hertz = ((Blur*)module)->fFreqCenterHz;
        if (hertz < 1000.0) {
            sprintf(formattedValue, "%.1f Hz", hertz);
        } else {
            sprintf(formattedValue, "%.3f kHz", hertz * 0.001);
        }
        return formattedValue;
	}
};


struct FreqWidthParamQuantity : ParamQuantity {
    char formattedValue[36];

	std::string getDisplayValueString() override {
        char lower[24]; 
        char upper[24];
        formatHz(lower, ((Blur*)module)->fFreqLowerHz);
        formatHz(upper, ((Blur*)module)->fFreqUpperHz);
        sprintf(formattedValue, "%s .. %s", lower, upper); 
        return formattedValue;
	}
void formatHz(char * buffer, float hertz) {
        if (hertz < 1000.0) {
            sprintf(buffer, "%.1f Hz", hertz);
        } else {
            sprintf(buffer, "%.3f kHz", hertz * 0.001);
        }

    }
};

struct FftSizeSubMenu : MenuItem {
	Blur * module;

    struct FFTSizeSubItem : MenuItem {
		Blur* module;
		int fftSize;
		void onAction(const event::Action& e) override {
            //DEBUG("FFT Sub Item .. set FFT Size %d", fftSize);
 			module->iSelectedFftFrameSize = fftSize;
		}
	};

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		std::string fftSizeNames[10] = {"32", "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384"};
		for (int i = 0; i < 10; i++) {
            int fftSize = std::stoi(fftSizeNames[i]);
			FFTSizeSubItem* fftItem = createMenuItem<FFTSizeSubItem>(fftSizeNames[i]);
			fftItem->rightText = CHECKMARK(module->iFftFrameSize == fftSize);
			fftItem->module = module;
			fftItem->fftSize = fftSize;
			menu->addChild(fftItem);
		}

		return menu;
	}

};

struct FrameDropParamQuantity : ParamQuantity {
    char formattedValue[24];

	std::string getDisplayValueString() override {
        float probability = ((Blur*)module)->fFrameDropProbability;
        probability = clamp(probability, 0.f, 1.f);
        sprintf(formattedValue, "%.1f%%", probability * 100.f); 
        return formattedValue;
	}
};

struct FreezeParamQuantity : ParamQuantity {

	std::string getDisplayValueString() override {
        return ((Blur*)module)->bFreeze ? "frozen" : "off";
	}
};


struct GainParamQuantity : ParamQuantity {
    char formattedValue[24];

	std::string getDisplayValueString() override {
        float fGain = ((Blur*)module)->fOutputGain;
        sprintf(formattedValue, "%.3f dB", dbForGain(fGain)); 
        return formattedValue;
	}
};

struct LengthParamQuantity : ParamQuantity {
    char formattedValue[24];

	std::string getDisplayValueString() override {
        sprintf(formattedValue, "%.3f seconds", ((Blur*)module)->fHistoryLengthSeconds);
        return formattedValue;
	}
};

struct MixParamQuantity : ParamQuantity {
    char formattedValue[24];

	std::string getDisplayValueString() override {
        int gain = int(((Blur*)module)->fActiveOutputMix * 100.f);
        int wetGain = clamp(gain, 0, 100);
        int dryGain = 100 - wetGain;
        sprintf(formattedValue, "%d%% / %d%%", wetGain, dryGain);
        return formattedValue;
	}
};

struct OversampleSubMenu : MenuItem {
	Blur * module;

    struct OversampleSubItem : MenuItem {
		Blur* module;
		int oversample;
		void onAction(const event::Action& e) override {
            //DEBUG("Oversample .. set Oversample %d", oversample);
			module->iSelectedOversample = oversample;
		}
	};

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		std::string oversampleNames[5] = {"1", "2", "4", "8", "16"};
		for (int i = 0; i < 5; i++) {
            int oversample = std::stoi(oversampleNames[i]);
			OversampleSubItem* oversampleItem = createMenuItem<OversampleSubItem>(oversampleNames[i]);
			oversampleItem->rightText = CHECKMARK(module->iOversample == oversample);
			oversampleItem->module = module;
			oversampleItem->oversample = oversample;
			menu->addChild(oversampleItem);
		}

		return menu;
	}
};

struct PitchShiftParamQuantity : ParamQuantity {
    char formattedValue[36];

	std::string getDisplayValueString() override {

        int quantMode = ((Blur*)module)->pitchQuantization;
        int semitones = ((Blur*)module)->iSemitoneShift;
        float pitchShift = ((Blur*)module)->fActivePitchShift;
        
        if (quantMode == Blur::PITCH_QUANT_SEMITONES) {
            sprintf(formattedValue, "%+d Semitones", semitones);            
        }
        else if (quantMode == Blur::PITCH_QUANT_FIFTHS) {
            sprintf(formattedValue, "%+d Semitones (by 5ths)", semitones);            
        }
        else if (quantMode == Blur::PITCH_QUANT_OCTAVES) {
            sprintf(formattedValue, "%+d Octaves", semitones / 12);                            
        }
        else { // if (quantMode == Blur::PITCH_QUANT_SMOOTH) {
            sprintf(formattedValue, "x %.3f", pitchShift);
        }

        return formattedValue;
	}
};

struct PositionParamQuantity : ParamQuantity {
    char formattedValue[24];

	std::string getDisplayValueString() override {
        sprintf(formattedValue, "%.3f seconds", ((Blur*)module)->fPositionSeconds);
        return formattedValue;
	}
};

struct RobotParamQuantity : ParamQuantity {

	std::string getDisplayValueString() override {
        return ((Blur*)module)->bRobot ? "on" : "off";
	}
};


struct BlurWidget : ModuleWidget {
	BlurWidget(Blur* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Blur.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(28.046, 17.615)), module, Blur::HISTORY_LENGTH_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.462, 17.879)), module, Blur::HISTORY_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(76.16, 17.879)), module, Blur::FREQ_CENTER_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(87.521, 17.879)), module, Blur::FREQ_CENTER_ATTENUVERTER_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.462, 29.722)), module, Blur::POSITION_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(28.046, 29.722)), module, Blur::POSITION_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(76.16, 29.722)), module, Blur::FREQ_SPAN_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(87.521, 29.722)), module, Blur::FREQ_SPAN_ATTENUVERTER_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.462, 42.171)), module, Blur::FRAME_DROP_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(28.046, 42.171)), module, Blur::FRAME_DROP_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(21.398, 52.409)), module, Blur::FREEZE_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(75.174, 59.616)), module, Blur::BLUR_FREQ_SELECTOR_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(85.798, 59.616)), module, Blur::PITCH_FREQ_SELECTOR_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(96.515, 59.616)), module, Blur::GAIN_FREQ_SELECTOR_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.2, 68.733)), module, Blur::BLUR_SPREAD_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(27.783, 68.733)), module, Blur::BLUR_SPREAD_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(76.16, 75.083)), module, Blur::PITCH_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(87.521, 75.083)), module, Blur::PITCH_ATTENUVERTER_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(21.136, 80.529)), module, Blur::ROBOT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(72.681, 87.078)), module, Blur::PITCH_QUANTIZE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.2, 91.788)), module, Blur::BLUR_MIX_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(27.783, 91.788)), module, Blur::BLUR_MIX_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(87.521, 102.611)), module, Blur::GAIN_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(76.16, 102.843)), module, Blur::GAIN_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(41.582, 114.189)), module, Blur::PHASE_RESET_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(76.16, 114.189)), module, Blur::MIX_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(87.521, 114.189)), module, Blur::MIX_ATTENUVERTER_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.408, 17.879)), module, Blur::HISTORY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(98.369, 17.879)), module, Blur::FREQ_CENTER_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.408, 29.722)), module, Blur::POSITION_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(98.369, 29.722)), module, Blur::FREQ_SPAN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.408, 42.171)), module, Blur::FRAME_DROP_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.642, 52.409)), module, Blur::FREEZE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.146, 68.733)), module, Blur::BLUR_SPREAD_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(98.369, 75.083)), module, Blur::PITCH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.379, 80.529)), module, Blur::ROBOT_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.146, 91.788)), module, Blur::BLUR_MIX_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(98.369, 102.611)), module, Blur::GAIN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.437, 114.189)), module, Blur::AUDIO_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(98.369, 114.189)), module, Blur::MIX_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.628, 114.189)), module, Blur::AUDIO_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(75.174, 45.848)), module, Blur::BLUR_FREQ_INSIDE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(83.521, 45.848)), module, Blur::PITCH_FREQ_INSIDE_EXCLUSIVE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(88.073, 45.848)), module, Blur::PITCH_FREQ_INSIDE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(96.485, 45.848)), module, Blur::GAIN_FREQ_INSIDE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(75.174, 49.816)), module, Blur::BLUR_FREQ_OUTSIDE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(83.521, 49.816)), module, Blur::PITCH_FREQ_OUTSIDE_EXCLUSIVE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(88.073, 49.816)), module, Blur::PITCH_FREQ_OUTSIDE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(96.485, 49.816)), module, Blur::GAIN_FREQ_OUTSIDE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(21.398, 52.409)), module, Blur::FREEZE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(75.174, 53.942)), module, Blur::BLUR_FREQ_ALL_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(85.769, 53.942)), module, Blur::PITCH_FREQ_ALL_LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(96.485, 53.942)), module, Blur::GAIN_FREQ_ALL_LED_LIGHT));
//		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(75.174, 59.616)), module, Blur::BLUR_FREQ_SELECTOR_LED_LIGHT));
//		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(85.798, 59.616)), module, Blur::PITCH_FREQ_SELECTOR_LED_LIGHT));
//		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(96.515, 59.616)), module, Blur::GAIN_FREQ_SELECTOR_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(21.136, 80.529)), module, Blur::ROBOT_LED_LIGHT));
//		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(72.681, 87.078)), module, Blur::PITCH_QUANT_SELECTOR_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(78.743, 87.078)), module, Blur::PITCH_QUANT_SMOOTH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(83.631, 87.078)), module, Blur::PITCH_QUANT_SEMITONES_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(88.519, 87.078)), module, Blur::PITCH_QUANT_FIFTHS_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(93.407, 87.078)), module, Blur::PITCH_QUANT_OCTAVE_LED_LIGHT));
//		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(41.582, 114.189)), module, Blur::RESIDUAL_SWITCH_LED_LIGHT));
	}

    virtual void appendContextMenu(Menu* menu) override {
		Blur * module = dynamic_cast<Blur*>(this->module);

        menu->addChild(new MenuSeparator());
		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("FFT Size"));

       FftSizeSubMenu *fftSizeSubMenu = createMenuItem<FftSizeSubMenu>("FFT Size", RIGHT_ARROW);
		fftSizeSubMenu->module = module;
		menu->addChild(fftSizeSubMenu);

      OversampleSubMenu *oversampleSubMenu = createMenuItem<OversampleSubMenu>("Oversample", RIGHT_ARROW);
		oversampleSubMenu->module = module;
		menu->addChild(oversampleSubMenu);
	}
};


Model* modelBlur = createModel<Blur, BlurWidget>("Blur");