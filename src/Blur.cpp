#include "plugin.hpp"
#include <math.hpp>
#include <dsp/fft.hpp>

const int MAX_FFT_FRAME_SIZE = 16384;
//const int DFLT_FFT_FRAME_SIZE = 16384;
//const int DFLT_FFT_FRAME_SIZE = 8192;
//const int DFLT_FFT_FRAME_SIZE = 4096;
const int DFLT_FFT_FRAME_SIZE = 2048;
//const int DFLT_FFT_FRAME_SIZE = 1024;
//const int DFLT_FFT_FRAME_SIZE = 512;
//const int DFLT_FFT_FRAME_SIZE = 256;
//const int DFLT_FFT_FRAME_SIZE = 32;

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

struct ListNode {
    struct ListNode * pNext;
    struct ListNode * pPrev;

    void detach() {
        pNext = NULL;
        pPrev = NULL;
    }
};
template <class T>
struct DoubleLinkList {
    T * pHead;
    T * pTail;
    int numMembers;

    DoubleLinkList() {
        pHead = NULL;
        pTail = NULL;
        numMembers = 0;
    }

    ~DoubleLinkList() {
        deleteMembers();
    }

    int size() const { return numMembers; }

    bool isEmpty() const { return pHead == NULL; }

    void deleteMembers() { 
        while (pHead != NULL) {
            T * pItem = pHead;
            pHead = (T*) pHead->pNext;
            delete pItem;
        }
        pHead = NULL;
        pTail = NULL;
        numMembers = 0;
    }

    T * popFront() { 
        if (pHead != NULL) {
            T * pItem = pHead;
            pHead = (T*) pItem->pNext;
            if (pHead == NULL) {
                pTail = NULL;
            }
            numMembers--;
            pItem->detach();
            return pItem;
        }
        return NULL;
    }

    T * popTail() { 
        if (pTail != NULL) {
            T * pItem = pTail;
            pTail = (T*) pItem->pPrev;
            if (pTail == NULL) {
                pHead = NULL;
            }
            numMembers--;
            pItem->detach();
            return pItem;
        }
        return NULL;
    }

    void pushFront(T * pItem) { 
        if (pHead == NULL) {
            pHead = pItem;
            pTail = pItem;
            pItem->detach();
        }
        else // pHead != null
        {
            pItem->pPrev = NULL;
            pItem->pNext = pHead;
            pHead = pItem;
        }
        numMembers++;
    }

    void pushTail(T * pItem) { 
        if (pTail == NULL) {
            pHead = pItem;
            pTail = pItem;
            pItem->detach();
        }
        else // pTail != null
        {
            pItem->pPrev = pTail;
            pItem->pNext = NULL;
            pTail = pItem;
        }
        numMembers++;
    }
};


template<class T>
class CircularBuffer
{
    public:
        CircularBuffer(size_t size) 
        {
            data = allocateCapacity(size);
            capacity = int(size);
            front = 1;
            rear = 0;
            population = 0;
        }

        ~CircularBuffer()
        {
            deleteMembers();
            delete[] data;
        }

        bool isFull() const { return population == capacity; }
        bool isEmpty() const { return population == 0; }
        int  getCapacity() const { return capacity; }
        int  getAvailable() const { return capacity - population; }
        int  numMembers() const { return population; }

//        void setCapacity(int capacityDesired) {
//            if (capacityDesired > capacity) {
//                growArray(capacityDesired);
//            } else if (capacityDesired < capacity) {
//                shrinkArray(capacityDesired);
//            }
//        }

        void enQueue(T * pItem)
        {
            if (population < capacity)
            {
                population++;
                rear = (rear + 1) % capacity;
                data[rear] = pItem;
            }
            else
            {
                // Overflow !
                DEBUG("ERROR: -- Circular buffer OVERFLOW: ptr = %p", pItem);
            }
        }

        T * deQueue()
        {
            if (population > 0)
            {
                population--;
                T * pItem = data[front];
                data[front] = NULL;
                front = (front + 1) % capacity;
                return pItem;
            }
            else
            {
                return NULL; // Underflow !
            }
        }

        T * peekAt(size_t iIndex) 
        {
            int idx = (rear - iIndex);
            if (idx < 0) {
                idx += capacity;
            }
            return data[idx];
        }

        void deleteMembers() 
        {
            while (! isEmpty()) {
                T * pItem = deQueue();
                delete pItem;
            }
        }

    private:
        T ** data;
        int capacity; // physical capacity
        int rear; // index of most recent (newest) entry
        int front;// index of oldest entry
        int population; // number of occupants 

        T ** allocateCapacity(int capacityDesired) {
            T ** ptr = new T*[capacityDesired];
            memset(ptr, 0, sizeof(T*) * capacityDesired);
            return ptr;
        }

//        void growArray(int capacityDesired) {
//            T ** new_data = allocateCapacity(capacityDesired);
//            for (int k = 0; k < capacity; k++) {
//                new_data[k] = data[k];
//            }
//            delete[] data;
//            data = new_data;
//            capacity = capacityDesired;
//        }
//
//        void shrinkArray(int capacityDesired) {
//            // This would be more efficient if the buffer had a 'limit' variable that 
//            // can be less or equal to capacity. limit would be the maximum gap between the
//            // front and rear indexes .. or something .. more optimal perhaps but trickier to implement
//
//            T ** new_data = allocateCapacity(capacityDesired);
//
//            for (int k = 0; k < capacityDesired; k++) {
//                new_data[k] = deQueue();
//            }
//
//            deleteMembers();
//            
//            delete[] data;
//            data = new_data;
//            capacity = capacityDesired;
//            front = 0; 
//            rear = capacityDesired - 1;
//            population = capacityDesired;
//        }

};

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
// BOttom Value is the level of the synthesized FFT output without amplification 
const struct ScalingFactor {
    float sampleRate;
    float scalingFactor;
} scalingFactors[] = {
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

//inline lerp(float a, float b, float fraction) {
//    return a + fraction * (b - a);
//}

//inline clampZeroOne(float f) {
//    return clamp(f, 0.f, 1.f);
//}
//
//inline getAttenuvert(int param_idx, int cv_idx) { 
//    return (params[param_idx].getValue() * (inputs[cv_idx].getVoltage()/10.f)); 
//}
//
// Convert [0,10] range to [-1,1]
// Return 0 if not connected 
//inline float getBipolarCvInput(int input_id) {
//    if (inputs[input_id].isConnected()) {
//        return (inputs[input_id].getVoltage() -5.f) / 5.f;
//    }
//    return 0.f;
//}

struct Blur : Module {
    enum ParamIds {
        LENGTH_ATTENUVERTER_PARAM,
        LENGTH_KNOB_PARAM,
        POSITION_ATTENUVERTER_PARAM,
        POSITION_KNOB_PARAM,
        BLUR_SPREAD_ATTENUVERTER_PARAM,
		BLUR_SPREAD_KNOB_PARAM,
		BLUR_FREQ_WIDTH_ATTENUVERTER_PARAM,
		BLUR_FREQ_WIDTH_KNOB_PARAM,
		BLUR_FREQ_CENTER_ATTENUVERTER_PARAM,
		BLUR_FREQ_CENTER_KNOB_PARAM,
		BLUR_MIX_ATTENUVERTER_PARAM,
		BLUR_MIX_KNOB_PARAM,
		PITCH_ATTENUVERTER_PARAM,
		PITCH_KNOB_PARAM,
		SEMITONE_BUTTON_PARAM,
        ROBOT_BUTTON_PARAM,
		FREEZE_ATTENUVERTER_PARAM,
        FREEZE_KNOB_PARAM,
		GAIN_ATTENUVERTER_PARAM,
		GAIN_KNOB_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        LENGTH_CV_INPUT,
        POSITION_CV_INPUT,
   		BLUR_SPREAD_CV_INPUT,
		BLUR_FREQ_WIDTH_CV_INPUT,
		BLUR_FREQ_CENTER_CV_INPUT,
		BLUR_MIX_CV_INPUT,
		ROBOT_CV_INPUT,
		PITCH_CV_INPUT,
		SEMITONE_CV_INPUT,
		FREEZE_CV_INPUT,
		GAIN_CV_INPUT,
        AUDIO_IN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUT_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
		SEMITONE_LED_LIGHT,
		ROBOT_LED_LIGHT,
        NUM_LIGHTS
    };

    // Variables for Tooltips 
    float fHistoryLengthSeconds;
    float fPositionSeconds;
    float fFreqCenterHz;
    float fFreqLowerHz;
    float fFreqUpperHz;
    float fPitchShiftTooltipValue;  // set in fillOutputFrame()


    // +/- gain for final output level
    float fOutputGain = 1.0;
    float fSelectedOutputGain = 1.0;

    DoubleLinkList<FftFrame>  fftFramePool;
    CircularBuffer<FftFrame>  fftFrameHistory{MAX_HISTORY_FRAMES};
    int iMaxHistoryFrames;  // limit the number of history frames accuulated  

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
    // This will ultimately be an FFT Frame pulled from the frame pool
    // and added to the history list
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

	bool bSemitone = false;
    int iSemitoneShift = 0; 
	dsp::SchmittTrigger semitoneTrigger;

    AlignedBuffer anaMagnitude{MAX_FFT_FRAME_SIZE};
    AlignedBuffer anaFrequency{MAX_FFT_FRAME_SIZE};

    AlignedBuffer synMagnitude{MAX_FFT_FRAME_SIZE};
    AlignedBuffer synFrequency{MAX_FFT_FRAME_SIZE};

        // TODO: consider using single flag like "reconfigRequired"
        // set that flag AFTER setting the SelectedXXX variable 
        //
        //  menu: 
        //     module->selectedFoo = 99
        //     module->reconfigRequired = true
        //
        // this method
        //    if reconfig required
        //       reconfig required = false  << race condition?
        //       fftsize = selected 
        //       sample rate = selected
        //       osamp = selected
        //       reconfigure()
        //
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
        
        iFftFrameSize = frameSize;
        iOversample = oversample;
        fActiveSampleRate = sampleRate; 

        iSelectedFftFrameSize = iFftFrameSize;
        iSelectedOversample = iOversample;
        fSelectedSampleRate = fActiveSampleRate; 


        iStepSize = iFftFrameSize / iOversample;
        iLatency = iFftFrameSize - iStepSize;
        iInIndex = iLatency; 
        phaseExpected = 2.* M_PI * double(iStepSize)/double(iFftFrameSize);

        fSampleRateScalingFactor = 1.f;
        for (size_t k = 0; k < sizeof(scalingFactors)/sizeof(scalingFactors[0]); k++) {
            if (scalingFactors[k].sampleRate == fActiveSampleRate) {
                fSampleRateScalingFactor = scalingFactors[k].scalingFactor;
                break;
            }
        }

        window.makeWindow(frameSize);
        inBuffer.clear();
        outBuffer.clear();
        outputAccumulator.clear();
        fftFrameHistory.deleteMembers();
        fftFramePool.deleteMembers();
        fftWorkspace.clear();
        lastPhase.clear();
        sumPhase.clear();
      
        if (pComplexFftEngine != NULL) {
            delete pComplexFftEngine;
        }
        pComplexFftEngine = new rack::dsp::ComplexFFT(iFftFrameSize);

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

        float fHistoryLength = params[LENGTH_KNOB_PARAM].getValue();
        fHistoryLength += params[LENGTH_ATTENUVERTER_PARAM].getValue() * getCvInput(LENGTH_CV_INPUT);    
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

// TODO: if the max history frames exceeds the initial capacity then
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
    }

    Blur() {
   		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(LENGTH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam<LengthParamQuantity>(LENGTH_KNOB_PARAM, 0.f, 1.f, 0.5f, "Length");

		configParam(POSITION_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam<PositionParamQuantity>(POSITION_KNOB_PARAM, 0.f, 1.f, 0.f, "Position");

		configParam(BLUR_SPREAD_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(BLUR_SPREAD_KNOB_PARAM, 0.f, 1.f, 0.f, "Blur");

		configParam(BLUR_FREQ_WIDTH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam<FreqWidthParamQuantity>(BLUR_FREQ_WIDTH_KNOB_PARAM, 0.f, 1.f, 1.f, "Blur Freq Width");

		configParam(BLUR_FREQ_CENTER_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam<FreqCenterParamQuantity>(BLUR_FREQ_CENTER_KNOB_PARAM, 0.f, 1.f, 0.5f, "Blur Freq Center");

		configParam(BLUR_MIX_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(BLUR_MIX_KNOB_PARAM, 0.f, 1.f, 1.f, "Blur Mix", "%", 0.f, 100.f, 0.f);

		configParam(SEMITONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Semitone");

		configParam(PITCH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam<PitchShiftParamQuantity>(PITCH_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pitch");

		configParam(ROBOT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Robot");

		configParam(FREEZE_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(FREEZE_KNOB_PARAM, 0.f, 1.f, 0.f, "Frame Drop", "%", 0.f, 100.f, 0.f);

		configParam(GAIN_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam<GainParamQuantity>(GAIN_KNOB_PARAM, 0.f, 1.f, 0.5f, "Gain");

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
        configureFftEngine_default();
        bRobot = false;
        robotTrigger.reset();
        bSemitone = false;
        semitoneTrigger.reset();
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

        float in = inputs[AUDIO_IN_INPUT].getVoltage();
        inBuffer.values[iInIndex] = in;
        float out = outBuffer.values[ iInIndex - iLatency ];

        float fGain = params[GAIN_KNOB_PARAM].getValue();
        fGain += params[GAIN_ATTENUVERTER_PARAM].getValue() * getCvInput(GAIN_CV_INPUT);    
        fGain = clamp(fGain, 0.f, 1.f);
        if (fGain != fSelectedOutputGain) { 
            fSelectedOutputGain = fGain;

            // TODO: for efficiency, only do this computation if the gain value changed 
            fGain = (fGain - 0.5) * 2; // convert to [-1,1]
            fOutputGain = pow(10.f, fGain * .6f); // +/- 6 db; 
        }

        outputs[AUDIO_OUT_OUTPUT].setVoltage(out * fOutputGain);

		if (semitoneTrigger.process(params[SEMITONE_BUTTON_PARAM].getValue() + inputs[SEMITONE_CV_INPUT].getVoltage())) {
			bSemitone = !bSemitone;
		}

		if (robotTrigger.process(params[ROBOT_BUTTON_PARAM].getValue() + inputs[ROBOT_CV_INPUT].getVoltage())) {
			bRobot = !bRobot;
		}

        iInIndex++;
        if (iInIndex >= iFftFrameSize) {
            iInIndex = iLatency;
            applyFftConfiguration();
            adjustFrameHistoryLength();
            processFrame();
        }

   		lights[ROBOT_LED_LIGHT].value = bRobot;
   		lights[SEMITONE_LED_LIGHT].value = bSemitone;
    }


    void processFrame() {

        float fFreeze = params[FREEZE_KNOB_PARAM].getValue();
        bool bAcceptNewFrame = random.generateZeroToOne() > fFreeze; 

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

        // Convert Magnitude/Phase to Real/Imag complex pair
        for (int k = 0; k <= iFftFrameSize/2; k++)
        {
            double magnitude = fftTemp.values[2*k];
            double phase = fftTemp.values[2*k+1];

            // phase = wrapPlusMinusPi(phase);

            fftTemp.values[2*k]   = magnitude*cos(phase);
            fftTemp.values[2*k+1] = magnitude*sin(phase);
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

            // ---- This worked at 44100.0 when not converting to magn/phase and back ----
            //float fOutValue = (fftWorkspace.values[2*k] * scale * window.values[k] * (5.f/1.9f));
            //// compute tilt factor to even out slight variances in output levels per bin
            //// bin[0] is 0.07 volts too high 
            //// bin[iFftFrameSize/2 - 1] is 0.07 volts too low 
            //// divide the additive factor by the oversampling so that each pass just makes part of the change 
            //float fTiltFactor = (float(2*k - iFftFrameSize/2) / float(iFftFrameSize/2)) * (0.05f / float(iOversample)); 
            //if (fOutValue < 0) {
            //    fOutValue -= fTiltFactor;
            //}
            //else { 
            //    fOutValue += fTiltFactor;
            //}
            // ---- 

            //float fOutValue = (fftWorkspace.values[2*k] * scale * window.values[k] * (5.0f/3.75f));
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
        
        float fPosition = params[POSITION_KNOB_PARAM].getValue();
        float fSpread = params[BLUR_SPREAD_KNOB_PARAM].getValue();
        float fBlurMix = params[BLUR_MIX_KNOB_PARAM].getValue();
        float fBlurFreqWidth = params[BLUR_FREQ_WIDTH_KNOB_PARAM].getValue();
        float fBlurFreqCenter = params[BLUR_FREQ_CENTER_KNOB_PARAM].getValue();
        float fPitchShift = params[PITCH_KNOB_PARAM].getValue();
       
        fPosition += params[POSITION_ATTENUVERTER_PARAM].getValue() * getCvInput(POSITION_CV_INPUT);    
        fPosition = clamp(fPosition, 0.f, 1.f);

        // for tooltips
        fPositionSeconds = fPosition * fHistoryLengthSeconds;

        fSpread += params[BLUR_SPREAD_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_SPREAD_CV_INPUT);    
        fSpread = clamp(fSpread, 0.f, 1.f);
        //if (fSpread <= 0.5) {
        //    fSpread = (fSpread / 0.5f) * 0.1f; // small increments left of center 
        //} else {
        //    fSpread = fSpread - (0.5f - 0.1f);
        //}

        fSpread += params[BLUR_SPREAD_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_SPREAD_CV_INPUT);    
        fSpread = clamp(fSpread, 0.f, 1.f);
        fSpread = std::pow(fSpread,3.0f);

        fSpread *= fMaxFrameIndex;

        fBlurMix += params[BLUR_MIX_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_MIX_CV_INPUT);    
        fBlurMix = clamp(fBlurMix, 0.f, 1.f);

        fBlurFreqWidth += params[BLUR_FREQ_WIDTH_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_FREQ_WIDTH_CV_INPUT);    
        fBlurFreqWidth = clamp(fBlurFreqWidth, 0.f, 1.f);

        fBlurFreqCenter += params[BLUR_FREQ_CENTER_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_FREQ_CENTER_CV_INPUT);    
        fBlurFreqCenter = clamp(fBlurFreqCenter, 0.f, 1.f);

        fPitchShift += params[PITCH_ATTENUVERTER_PARAM].getValue() * getCvInput(PITCH_CV_INPUT);    
        fPitchShift = clamp(fPitchShift, 0.f, 1.f);

        // TODO: do the min/max exponent calculations once on sample rate changes 
        float freqNyquist = fActiveSampleRate * 0.5;
        double freqPerBin = fActiveSampleRate / double(iFftFrameSize); 

        float minExponent = log10(freqPerBin);
        float maxExponent = log10(freqNyquist);
        float exponentRange = maxExponent - minExponent;

        float fCenterExponent = minExponent + (fBlurFreqCenter * exponentRange);
        fBlurFreqWidth *= exponentRange;


        float lowerExponent = fCenterExponent - (fBlurFreqWidth * 0.5);
        float upperExponent = lowerExponent + fBlurFreqWidth;
        // conmpute bin indexes
        float lowerFreq = pow(10.f, lowerExponent);
        float upperFreq = pow(10.f, upperExponent);

        // Tooltips
        fFreqCenterHz = pow(10.f, fCenterExponent) - (freqPerBin * 0.5f);
        fFreqLowerHz  = pow(10.f, lowerExponent);
        fFreqUpperHz  = pow(10.f, upperExponent);

        int iLowerBinIndex = myFloor(lowerFreq / freqPerBin);
        int iUpperBinIndex = myFloor(upperFreq / freqPerBin);
        iLowerBinIndex = clamp(iLowerBinIndex, 0, iFftFrameSize/2 + 1);
        iUpperBinIndex = clamp(iUpperBinIndex, 0, iFftFrameSize/2 + 1);

        //DEBUG("   minExponent, maxExponent = (%f, %f)", minExponent, maxExponent);
        //DEBUG("   lowerExponent, upperExponent = %f, %f", lowerExponent, upperExponent);
        //DEBUG("   lowerFreq, upperFreq = %f, %f", lowerFreq, upperFreq);
        //DEBUG("   lowerBinIndex, upperBinIndex = %d, %d", iLowerBinIndex, iUpperBinIndex);

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

            if (k < iLowerBinIndex || k > iUpperBinIndex) {
                fSelected = fCursor; // no spread 
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

/********* PITCH SHIFT *******/            
            // compute the phase delta
            double tmp = phase - lastPhase.values[k];
            lastPhase.values[k] = phase;

            // subtract expected phase difference
            tmp -= float(k) * phaseExpected;

            // map to +/- Pi 
            tmp = wrapPlusMinusPi(tmp);

            // get deviation from bin frequency
            tmp = (double(iOversample) * tmp) / (2. * M_PI);

            // compute the k'th partial's true frequency
            tmp = double(k)*freqPerBin + tmp*freqPerBin;

            anaMagnitude.values[k] = magn;  // magnitude
            anaFrequency.values[k] = tmp;   // frequency
        }

        // Perform Pitch Shifting 

        /// Semitone 
        if (bSemitone) {
            // convert [-1..1] -> [-36..36]
            fPitchShift = (fPitchShift - 0.5f) * 2.f;  // -1..1
            iSemitoneShift = int(fPitchShift * 36.0); 
            fPitchShift = pow(2.0, double(iSemitoneShift)/12.0);
        }
        else
        {
            fPitchShift *= 2.f;  // 0 .. 1 .. 2    
            //if (fPitchShift > 1.0) {
            //    fPitchShift = 1.0 + ((fPitchShift - 1.f) * 3.f);  // 0 .. 1..2..3    
            //}
        }

        fPitchShiftTooltipValue = fPitchShift;

    	synMagnitude.clear();
        synFrequency.clear();
        for (int k = 0; k <= iFftFrameSize/2; k++) { 
			int index = k*fPitchShift;
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
            tmp -= double(k) * freqPerBin;

            // get bin deviation from freq deviation
            tmp /= freqPerBin;

            // take oversample into account
            tmp = 2.f * M_PI * tmp / double(iOversample);

            // add the overlap phase advance back in 
            tmp += double(k) * phaseExpected;

            // accumulate delta phase to get bin phase
            sumPhase.values[k] += tmp;
            double phase = sumPhase.values[k];

/********* End PITCH SHIFT *******/   

            outputFrame.values[2*k] = magn;
            outputFrame.values[2*k+1] = phase;            
        }        

/** EXPERIMENT -- shift/rotate the bins **
 * This works - but this might not be the place to do the shifting
        float fShiftAmount = (params[PITCH_KNOB_PARAM].getValue() - 0.5f) * 2.f * float(iFftFrameSize/2);
        shiftFrame(outputFrame, int(shiftAmount))
** end EXPERIMENT **/

    }

// TODO: consider this 
//    FftFrame * getEmptyFrame() {
//        FftFrame * pFrame = fftFramePool.popFront();
//        if (pFrame == NULL) {
//            pFrame = new FftFrame(iFftFrameSize);
//        }
//        pFrame->clear();
//        return pFrame;
//    }
//
//    void cacheFrame(FftFrame * pFrame) {
//        fftFramePool.pushFront(pFrame);
//    }

    // TODO: Future - this is not called yet 
    void shiftFrame(FftFrame & sourceFrame, int shiftAmount) {

        FftFrame * pShiftedFrame = fftFramePool.popFront();
        if (pShiftedFrame == NULL) {
            pShiftedFrame = new FftFrame(iFftFrameSize);
        }
        pShiftedFrame->clear();

        // leave bins 0 and fftFrameSize/2 alone 
        for (int k = 1; k < iFftFrameSize/2; k++) {
            int targetBin = k + shiftAmount;
            if (targetBin <= 0) {
                targetBin += iFftFrameSize/2;
            }
            if (targetBin >= iFftFrameSize/2) {
                targetBin -= iFftFrameSize/2;
            }
            pShiftedFrame->values[2*targetBin] += sourceFrame.values[2*k];
            pShiftedFrame->values[2*targetBin+1] += sourceFrame.values[2*k+1];
        }

        for (int k = 1; k < iFftFrameSize/2; k++) {
            sourceFrame.values[2*k] = pShiftedFrame->values[2*k];
            sourceFrame.values[2*k+1] = pShiftedFrame->values[2*k+1];
        }

        fftFramePool.pushFront(pShiftedFrame);
    }
};


struct FreqCenterParamQuantity : ParamQuantity {
    char formattedValue[24];

	std::string getDisplayValueString() override {
        float hertz = ((Blur*)module)->fFreqCenterHz;
        if (hertz < 1000.0) {
            sprintf(formattedValue, "%.3f Hz", hertz);
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
    char formattedValue[24];

	std::string getDisplayValueString() override {
        bool bSemitone    = ((Blur*)module)->bSemitone;
        if (bSemitone) {
            int semitones = ((Blur*)module)->iSemitoneShift;
            sprintf(formattedValue, "%d Semitones", semitones);
        } else {
            float fPitchShift = ((Blur*)module)->fPitchShiftTooltipValue;
            sprintf(formattedValue, "x %.3f", fPitchShift);
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


struct BlurWidget : ModuleWidget {
    BlurWidget(Blur* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Blur.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.804, 13.836)), module, Blur::LENGTH_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.841, 13.836)), module, Blur::LENGTH_KNOB_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.804, 24.565)), module, Blur::POSITION_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.841, 24.565)), module, Blur::POSITION_KNOB_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.625, 34.471)), module, Blur::BLUR_SPREAD_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.662, 34.471)), module, Blur::BLUR_SPREAD_KNOB_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.625, 44.719)), module, Blur::BLUR_FREQ_WIDTH_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.662, 44.719)), module, Blur::BLUR_FREQ_WIDTH_KNOB_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.625, 55.141)), module, Blur::BLUR_FREQ_CENTER_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.662, 55.141)), module, Blur::BLUR_FREQ_CENTER_KNOB_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.625, 65.903)), module, Blur::BLUR_MIX_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.662, 65.903)), module, Blur::BLUR_MIX_KNOB_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.804, 77.018)), module, Blur::PITCH_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.841, 77.018)), module, Blur::PITCH_KNOB_PARAM));

		addParam(createParamCentered<LEDButton>(mm2px(Vec(23.905, 84.924)), module, Blur::SEMITONE_BUTTON_PARAM));

		addParam(createParamCentered<LEDButton>(mm2px(Vec(69.314, 86.581)), module, Blur::ROBOT_BUTTON_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.804, 95.137)), module, Blur::FREEZE_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.841, 95.137)), module, Blur::FREEZE_KNOB_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.804, 105.799)), module, Blur::GAIN_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.841, 105.799)), module, Blur::GAIN_KNOB_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 13.836)), module, Blur::LENGTH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 24.565)), module, Blur::POSITION_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.177, 34.471)), module, Blur::BLUR_SPREAD_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.177, 44.719)), module, Blur::BLUR_FREQ_WIDTH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.177, 55.141)), module, Blur::BLUR_FREQ_CENTER_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.177, 65.903)), module, Blur::BLUR_MIX_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(69.314, 73.96)), module, Blur::ROBOT_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 77.018)), module, Blur::PITCH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.784, 84.924)), module, Blur::SEMITONE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 95.137)), module, Blur::FREEZE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 105.799)), module, Blur::GAIN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.812, 117.474)), module, Blur::AUDIO_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(67.464, 117.474)), module, Blur::AUDIO_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(23.905, 84.924)), module, Blur::SEMITONE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(69.314, 86.581)), module, Blur::ROBOT_LED_LIGHT));
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