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
        drain();
    }

    int size() const { return numMembers; }

    bool isEmpty() const { return pHead == NULL; }

    void drain() { 
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
            this->size = size;
            data = new T*[size];
            front = 1;
            rear = 0;
            population = 0;
        }

        ~CircularBuffer()
        {
            delete[] data;
        }

        bool isFull() const { return population == size; }
        bool isEmpty() const { return population == 0; }
        int  capacity() const { return size; }
        int  available() const { return size - population; }
        int  numMembers() const { return population; }

        void enQueue(T * pItem)
        {
            if (population < size)
            {
                population++;
                rear = (rear + 1) % size;
                data[rear] = pItem;
            }
            else
            {
                // Overflow !
            }
        }

        T * deQueue()
        {
            if (population > 0)
            {
                population--;
                T * pItem = data[front];
                data[front] = NULL;
                front = (front + 1) % size;
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
                idx += size;
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
        int size; // physical capacity
        int rear; // index of most recent (newest) entry
        int front;// index of oldest entry
        int population; // number of occupants 
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
        static uint32_t x = 123456789;
        static uint32_t y = 362436069;
        static uint32_t z = 521288629;
        static uint32_t w = 88675123;
        uint32_t t;
        t = x ^ (x << 11);   
        x = y; y = z; z = w;   
        return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
    }
};

// from https://www.codeproject.com/tips/700780/fast-floor-ceiling-functions
inline int myFloor(float fVal) { 
    return (int)(fVal + 32768.) - 32768;
}

inline int myCeil(float fVal) {
    return 32768 - (int)(32768. - fVal);
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
// Determined emperically 
// Top Value (5.f) is the range of VCV rack audio signals
static struct ScalingFactor {
    float sampleRate;
    float scalingFactor;
} scalingFactors[] = {
    {  44100.f, 5.f/3.75f },
    {  48000.f, 5.f/3.75f }, //5.f/3.56f },
    {  88200.f, 5.f/3.75f }, //5.f/2.50f },
    {  96000.f, 5.f/3.75f }, //5.f/2.345f },
    { 176400.f, 5.f/3.75f }, //5.f/1.635f },
    { 192000.f, 5.f/3.75f }, //5.f/1.575f },
    { 352800.f, 5.f/3.68f }, //5.f/1.35f },
    { 384000.f, 5.f/3.62f }, //5.f/1.335f },
    { 705600.f, 5.f/2.60f }, //5.f/1.28f },
    { 768000.f, 5.f/2.445f }, //5.f/1.27f },
};


inline gainForDb(float dB) {
    return pow(10, dB * 0.1); // 10^(db/10)
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
//inline getInput(int param_idx) { 
//    return inputs[param_idx]; 
//}
//
//inline getPosition() { 
//    return clampZeroOne(params[POSITION_KNOB_PARAM].getValue() + getAttenuvert(POSITION_ATTENUVERTER_PARAM, POSITION_CV_INPUT); 
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

    DoubleLinkList<FftFrame>  fftFramePool;
    CircularBuffer<FftFrame>  fftFrameHistory{500};
    int iMaxHistoryFrames;

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

	bool bSemitone = false;
	dsp::SchmittTrigger semitoneTrigger;

    AlignedBuffer anaMagnitude{MAX_FFT_FRAME_SIZE};
    AlignedBuffer anaFrequency{MAX_FFT_FRAME_SIZE};

    AlignedBuffer synMagnitude{MAX_FFT_FRAME_SIZE};
    AlignedBuffer synFrequency{MAX_FFT_FRAME_SIZE};

    // Convert [0,10] range to [-1,1]
    // Return 0 if not connected 
    //inline float getBipolarCvInput(int input_id) {
    //    if (inputs[input_id].isConnected()) {
    //        return (inputs[input_id].getVoltage() -5.f) / 5.f;
    //    }
    //    return 0.f;
    //}


    void applyFftConfiguration() {
        // TODO: consider doing this with single flag - reconfigRequired
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

        bool bConfigurationRequired = false;

// could do this with single if a or b or c check 
        if (iSelectedFftFrameSize != iFftFrameSize) {
            //iFftFrameSize = iSelectedFftFrameSize;
            bConfigurationRequired = true;
        } 

        if (iSelectedOversample != iOversample) {
            //iOversample = iSelectedOversample;
            bConfigurationRequired = true;
        } 

        if (fSelectedSampleRate != fActiveSampleRate) {
            //fActiveSampleRate = fSelectedSampleRate;
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
        iMaxHistoryFrames = 500;  // default 

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
        fftFramePool.drain();
        fftWorkspace.clear();
        lastPhase.clear();
        sumPhase.clear();
      
        if (pComplexFftEngine != NULL) {
            delete pComplexFftEngine;
        }
        pComplexFftEngine = new rack::dsp::ComplexFFT(iFftFrameSize);

        DEBUG("--- Initialize FFT ---");
        DEBUG("inBuffer.size() = %d", inBuffer.size());
        DEBUG("outBuffer.size() = %d", outBuffer.size());
        DEBUG("FftFrameHistory capacity = %d", fftFrameHistory.capacity());
        DEBUG("FftFrameHistory available = %d", fftFrameHistory.available());
        //DEBUG("phaseExpected = %f", phaseExpected);
        DEBUG(" fftTemp,   size %d, numBins %d, address %p", fftTemp.size(), fftTemp.numBins(), fftTemp.values);
        DEBUG(" InBuffer,  size %d, address %p", inBuffer.size(), inBuffer.values);
        DEBUG(" OutBuffer, size %d, address %p", outBuffer.size(), outBuffer.values);
        DEBUG(" Sample Rate = %f", fActiveSampleRate);
    }

//    int computeMaxHistoryFrames(float fLengthSeconds) {
//        float fFramesPerSecond = (float(iOversample) * fActiveSampleRate) / float(iFftFrameSize);
//
//        fLengthSeconds = clamp(fLengthSeconds, 0.f, 10.f);  // TODO: constant 
//        
//        iMaxHistoryFrames =  myCeil(fFramesPerSecond * fLengthSeconds);
//        if (iMaxHistoryFrames <= 0) {
//            iMaxHistoryFrames = 1;
//        }
//    } 

    // if sample rate changes, drain the history - the values are likely to be noise at the new sample rate 
    // if the length changes, calculate a new max length and drain just enough frames to leave 'length' number in the history
    void adjustFrameHistoryLength() {
        float fFramesPerSecond = (float(iOversample) * fActiveSampleRate) / float(iFftFrameSize);

        float fLengthSeconds = params[LENGTH_KNOB_PARAM].getValue();
        fLengthSeconds += params[LENGTH_ATTENUVERTER_PARAM].getValue() * getCvInput(LENGTH_CV_INPUT);    
        fLengthSeconds = clamp(fLengthSeconds, 0.f, 1.f);
        fLengthSeconds *= 10.0; // seconds    // TODO: constant 
        
        iMaxHistoryFrames =  myCeil(fFramesPerSecond * fLengthSeconds);
        if (iMaxHistoryFrames <= 0) {
            iMaxHistoryFrames = 1;
        }
        
        //DEBUG(" Frames per second = %f", fFramesPerSecond);
        //DEBUG(" Length seconds = %f", fLengthSeconds);
        //DEBUG(" Max history frames = %d", iMaxHistoryFrames);

        // Siphon frames from the History to the Pool until the number of history frames
        // is less than of equal to the max
        while (fftFrameHistory.numMembers() > iMaxHistoryFrames) {
            FftFrame *pFftFrame = fftFrameHistory.deQueue();
            fftFramePool.pushTail(pFftFrame);
        }
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
		configParam(LENGTH_KNOB_PARAM, 0.f, 1.f, 0.5f, "Length");

		configParam(POSITION_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(POSITION_KNOB_PARAM, 0.f, 1.f, 0.f, "Position");

		configParam(BLUR_SPREAD_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(BLUR_SPREAD_KNOB_PARAM, 0.f, 1.f, 0.f, "Blur");

		configParam(BLUR_FREQ_WIDTH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(BLUR_FREQ_WIDTH_KNOB_PARAM, 0.f, 1.f, 1.f, "Blur Freq Width");

		configParam(BLUR_FREQ_CENTER_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(BLUR_FREQ_CENTER_KNOB_PARAM, 0.f, 1.f, 0.5f, "Blur Freq Center");

		configParam(BLUR_MIX_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(BLUR_MIX_KNOB_PARAM, 0.f, 1.f, 1.f, "Blur Mix");

		configParam(SEMITONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Semitone");

		configParam(PITCH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(PITCH_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pitch");

		configParam(ROBOT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Robot");

		configParam(FREEZE_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(FREEZE_KNOB_PARAM, 0.f, 1.f, 0.f, "Freeze");

		configParam(GAIN_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "");
		configParam(GAIN_KNOB_PARAM, 0.f, 1.f, 0.5f, "Gain");

        pComplexFftEngine = NULL;
        initialize();
    }

    ~Blur() {
        fftFramePool.drain();
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
        fGain = (fGain - 0.5) * 2; // convert to [-1,1]
        fGain = pow(10.f, fGain * .6f); // +/- 6 db; 

        outputs[AUDIO_OUT_OUTPUT].setVoltage(out * fGain);

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

            // Push the frame into the history list
            // Purge older frames
            if (fftFrameHistory.isFull()) {
                FftFrame * pOldFrame = fftFrameHistory.deQueue();
                fftFramePool.pushTail(pOldFrame);
            }
            fftFrameHistory.enQueue(pFftFrame);

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

        float fPosition = params[POSITION_KNOB_PARAM].getValue();
        float fSpread = params[BLUR_SPREAD_KNOB_PARAM].getValue();
        float fBlurMix = params[BLUR_MIX_KNOB_PARAM].getValue();
        float fBlurFreqWidth = params[BLUR_FREQ_WIDTH_KNOB_PARAM].getValue();
        float fBlurFreqCenter = params[BLUR_FREQ_CENTER_KNOB_PARAM].getValue();
        float fPitchShift = params[PITCH_KNOB_PARAM].getValue();
       
		if (semitoneTrigger.process(params[SEMITONE_BUTTON_PARAM].getValue() + inputs[SEMITONE_CV_INPUT].getVoltage())) {
			bSemitone = !bSemitone;
		}

		if (robotTrigger.process(params[ROBOT_BUTTON_PARAM].getValue() + inputs[ROBOT_CV_INPUT].getVoltage())) {
			bRobot = !bRobot;
		}

        fPosition += params[POSITION_ATTENUVERTER_PARAM].getValue() * getCvInput(POSITION_CV_INPUT);    
        fPosition = clamp(fPosition, 0.f, 1.f);

        fSpread += params[BLUR_SPREAD_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_SPREAD_CV_INPUT);    
        fSpread = clamp(fSpread, 0.f, 1.f);
        if (fSpread <= 0.5) {
            fSpread = (fSpread / 0.5f) * 0.1f; // small increments left of center 
        } else {
            fSpread = fSpread - (0.5f - 0.1f);
        }
        fSpread *= fMaxFrameIndex;

        fBlurMix += params[BLUR_MIX_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_MIX_CV_INPUT);    
        fBlurMix = clamp(fBlurMix, 0.f, 1.f);

        fBlurFreqWidth += params[BLUR_FREQ_WIDTH_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_FREQ_WIDTH_CV_INPUT);    
        fBlurFreqWidth = clamp(fBlurFreqWidth, 0.f, 1.f);

        fBlurFreqCenter += params[BLUR_FREQ_CENTER_ATTENUVERTER_PARAM].getValue() * getCvInput(BLUR_FREQ_CENTER_CV_INPUT);    
        fBlurFreqCenter = clamp(fBlurFreqCenter, 0.f, 1.f);

        fPitchShift += params[PITCH_ATTENUVERTER_PARAM].getValue() * getCvInput(PITCH_CV_INPUT);    
        fPitchShift = clamp(fPitchShift, 0.f, 1.f);

        //DEBUG("-- Blur Freq Width param = %f", fBlurFreqWidth);
        ///DEBUG("   Blur Freq Center param = %f", fBlurFreqCenter);
        
        // TODO: do this once on sample rate changes 
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
        float lowerFreq = pow(10, lowerExponent);
        float upperFreq = pow(10, upperExponent);

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
        //DEBUG("   History Buffer: size = %d", fftFrameHistory.size);
        //DEBUG("                   rear  = %d", fftFrameHistory.rear);
        //DEBUG("                   front = %d", fftFrameHistory.front);
        //DEBUG("              population = %d", fftFrameHistory.population);
        //DEBUG("-- outputFrame (size, numBins) = (%d, %d)", outputFrame.size(), outputFrame.numBins());

        // DEBUG 
        //memcpy(outputFrame.values, fftFrameHistory.peekAt(0)->values, outputFrame.size() * sizeof(float)); // TODO: DEBUG -- delete this 
        //return;

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
            
            //if (k == 0) {
            //    DEBUG("  [%d] random = %f", k, fRandomValue);
            //    DEBUG("       Selected = %f", fSelected);
            //    DEBUG("       index (before, after) = (%d, %d)", iIndexBefore, iIndexAfter);
            //}

            FftFrame * pFrameBefore = fftFrameHistory.peekAt(iIndexBefore);
            FftFrame * pFrameAfter = fftFrameHistory.peekAt(iIndexAfter);

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
                magn *= gainForDb(6.); // approx scaling factor at 44100.0 sampling rate 
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
            int semitone; 
            // convert [-1..1] -> [-36..36]
            fPitchShift = (fPitchShift - 0.5f) * 2.f;  // -1..1
            semitone = int(fPitchShift * 36.0); 
            fPitchShift = pow(2.0, double(semitone)/12.0);
        }
        else
        {
            fPitchShift *= 2.f;  // 0 .. 1 .. 2    
            //if (fPitchShift > 1.0) {
            //    fPitchShift = 1.0 + ((fPitchShift - 1.f) * 3.f);  // 0 .. 1..2..3    
            //}
        }

    	synMagnitude.clear();
        synFrequency.clear();
        for (int k = 0; k <= iFftFrameSize/2; k++) { 
			int index = k*fPitchShift;
			if (index <= iFftFrameSize/2) { 
				synMagnitude.values[index] += anaMagnitude.values[k];
				synFrequency.values[index] = anaFrequency.values[k] * fPitchShift; 
			} 
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

// EXPERIMENT 
//lastPhase.values[k] = phase;

/********* End PITCH SHIFT *******/            

            //float fDryMagnitude = pFrameDry->values[2*k];
            //float fDryPhase = pFrameDry->values[2*k+1];
    
            //outputFrame.values[2*k] = (fValue * fBlurMix) + (fDryMagnitude * (1.0 - fBlurMix));
            //outputFrame.values[2*k+1] = (phase * fBlurMix) + (fDryPhase * (1.0 - fBlurMix));            

            outputFrame.values[2*k] = magn;
            outputFrame.values[2*k+1] = phase;            
        }        
    }

};

struct FftSizeSubMenu : MenuItem {
	Blur * module;

    struct FFTSizeSubItem : MenuItem {
		Blur* module;
		int fftSize;
		void onAction(const event::Action& e) override {
            DEBUG("FFT Sub Item .. set FFT Size %d", fftSize);
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

struct OversampleSubMenu : MenuItem {
	Blur * module;

    struct OversampleSubItem : MenuItem {
		Blur* module;
		int oversample;
		void onAction(const event::Action& e) override {
            DEBUG("Oversample .. set Oversample %d", oversample);
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