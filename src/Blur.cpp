#include "plugin.hpp"
#include <math.hpp>
#include <dsp/fft.hpp>
//#define MAX_FFT_FRAME_SIZE  (8096)
const int DFLT_FFT_FRAME_SIZE = 2048;

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

			// TODO: add a 'soft-limit' to the maximum number of items in the ring
			// so that the history length can be dynamically adjusted 
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
                return NULL;
        }

		T * peekAt(size_t iIndex) 
		{
			int idx = (rear - iIndex);
			if (idx < 0) {
				idx += size;
			}
			return data[idx];
		}

        void deleteMembers() {
			while (! isEmpty()) {
				T * pItem = deQueue();
				delete pItem;
			}
		}

    // private:
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
		void makeWindow() {
			for (int k = 0; k < numValues; k++) {
				values[k] = -.5 * cos(2.*M_PI*(float)k/(float)numValues) + .5; // Hamming window 
				//values[k] = .5 * (1. - cos(2.*M_PI*(float)k/(float)numValues));  // Hann window
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
		return (((float) xor128() / (float)UINT32_MAX) - 0.5f) * M_PI;
	}
	
	//float generate() { 
	//	return (float) xor128() / (float)UINT32_MAX;
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
	{  48000.f, 5.f/3.56f },
	{  88200.f, 5.f/2.50f },
	{  96000.f, 5.f/2.345f },
	{ 176400.f, 5.f/1.635f },
	{ 192000.f, 5.f/1.575f },
	{ 352800.f, 5.f/1.35f },
	{ 384000.f, 5.f/1.335f },
	{ 705600.f, 5.f/1.28f },
	{ 768000.f, 5.f/1.27f },
};

//inline lerp(float a, float b, float fraction) {
//	return a + fraction * (b - a);
//}

//inline clampZeroOne(float f) {
//	return clamp(f, 0.f, 1.f);
//}
//
//inline getAttenuvert(int param_idx, int cv_idx) { 
//	return (params[param_idx].getValue() * (inputs[cv_idx].getVoltage()/10.f)); 
//}
//
//inline getInput(int param_idx) { 
//	return inputs[param_idx]; 
//}
//
//inline getPosition() { 
//	return clampZeroOne(params[POSITION_KNOB_PARAM].getValue() + getAttenuvert(POSITION_ATTENUVERTER_PARAM, POSITION_CV_INPUT); 
//}

struct Blur : Module {
	enum ParamIds {
		LENGTH_ATTENUVERTER_PARAM,
		LENGTH_KNOB_PARAM,
		POSITION_ATTENUVERTER_PARAM,
		POSITION_KNOB_PARAM,
		MOD_RATE_ATTENUVERTER_PARAM,
		MOD_RATE_KNOB_PARAM,
		MOD_DEPTH_ATTENUVERTER_PARAM,
		MOD_DEPTH_KNOB_PARAM,
		SPREAD_ATTENUVERTER_PARAM,
		SPREAD_KNOB_PARAM,
		BIAS_ATTENUVERTER_PARAM,
		BIAS_KNOB_PARAM,
		PHASE_ATTENUVERTER_PARAM,
		PHASE_KNOB_PARAM,
		FREEZE_KNOB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		LENGTH_CV_INPUT,
		POSITION_CV_INPUT,
		MOD_RATE_CV_INPUT,
		MOD_DEPTH_CV_INPUT,
		SPREAD_CV_INPUT,
		BIAS_CV_INPUT,
		PHASE_CV_INPUT,
		FREEZE_CV_INPUT,
		AUDIO_IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		AUDIO_OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		AUDIO_IN_LED_LIGHT,
		NUM_LIGHTS
	};

	DoubleLinkList<FftFrame>  fftFramePool;
	CircularBuffer<FftFrame>  fftFrameHistory{500};
	int iMaxHistoryFrames;
	//float fCursorPosition = 0.5; // 50%
	// modulation rate
	// modulation depth 
	// spread
	// bias
	// phase randomization 

	AlignedBuffer window{DFLT_FFT_FRAME_SIZE};
	AlignedBuffer inBuffer{DFLT_FFT_FRAME_SIZE};
	AlignedBuffer outBuffer{DFLT_FFT_FRAME_SIZE};
	int iInIndex; 
	int iLatency;

    int iFftFrameSize;
	int iOversample;
	int iStepSize; 
    FftFrame fftWorkspace{DFLT_FFT_FRAME_SIZE};
	// This will ultimately be an FFT Frame pulled from the frame pool
	// and added to the history list
    FftFrame fftTemp{DFLT_FFT_FRAME_SIZE};

	//AlignedBuffer lastPhase{DFLT_FFT_FRAME_SIZE/2 + 1};
	//AlignedBuffer sumPhase{DFLT_FFT_FRAME_SIZE/2 + 1};
	//float phaseExpected;

	AlignedBuffer outputAccumulator{DFLT_FFT_FRAME_SIZE * 2};

    //SmbFft smbFft{DFLT_FFT_FRAME_SIZE};
	rack::dsp::ComplexFFT complexFftEngine{DFLT_FFT_FRAME_SIZE};
	float fSampleRateScalingFactor; 

	Random random;

    float fActiveSampleRate; 

    // Convert [0,10] range to [-1,1]
	// Return 0 if not connected 
    //inline float getBipolarCvInput(int input_id) {
	//	if (inputs[input_id].isConnected()) {
	//		return (inputs[input_id].getVoltage() -5.f) / 5.f;
	//	}
	//	return 0.f;
	//}

    // Convert [0,10] range to [0,1]
	// Return 0 if not connected 
    inline float getCvInput(int input_id) {
		if (inputs[input_id].isConnected()) {
			return (inputs[input_id].getVoltage() / 10.0);
		}
		return 0.f;
	}

	// if sample rate changes, drain the history - the values are likely to be noise at the new sample rate 
	// if the length changes, calculate a new max length and drain just enough frames to leave 'length' number in the history
	void adjustFrameHistoryLength() {
		float fFramesPerSecond = (float(iOversample) * fActiveSampleRate) / float(iFftFrameSize);

		float fLengthSeconds = params[LENGTH_KNOB_PARAM].getValue();   // TODO: constant, 10 seconds 
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

		while (fftFrameHistory.numMembers() > iMaxHistoryFrames) {
			FftFrame *pFftFrame = fftFrameHistory.deQueue();
			fftFramePool.pushTail(pFftFrame);
		}
	}

	void updateSampleRate(float sampleRate) {
		fActiveSampleRate = sampleRate; 

		// if sample rate changes, drain the history - the values are likely to be noise at the new sample rate 
		while (fftFrameHistory.numMembers() > 0) {
			FftFrame *pFftFrame = fftFrameHistory.deQueue();
			fftFramePool.pushTail(pFftFrame);
		}

		fSampleRateScalingFactor = 1.f;
		for (size_t k = 0; k < sizeof(scalingFactors)/sizeof(scalingFactors[0]); k++) {
			if (scalingFactors[k].sampleRate == fActiveSampleRate) {
				fSampleRateScalingFactor = scalingFactors[k].scalingFactor;
				break;
			}
		}

		//DEBUG("Sample Rate = %f, scaling factor = %f", fActiveSampleRate, fSampleRateScalingFactor);
	}

    void initialize() { 
		iFftFrameSize = DFLT_FFT_FRAME_SIZE;
		iOversample = 4;
		iStepSize = iFftFrameSize / iOversample;
		iLatency = iFftFrameSize - iStepSize;
		iInIndex = iLatency; 
		window.makeWindow();
		inBuffer.clear();
		outBuffer.clear();
		outputAccumulator.clear();
		
		fActiveSampleRate = 0; // force this to be updated on first invocation of process() 44100.0; 
		iMaxHistoryFrames = 0; // force this to be updated on first invocation of process() 44100.0; 
		fftFrameHistory.deleteMembers();

		fftFramePool.drain();
		fftWorkspace.clear();
		//lastPhase.clear();
		//sumPhase.clear();
		//phaseExpected = 2.* M_PI * (float)iStepSize/(float)iFftFrameSize;

	    //smbFft.setFrameSize(iFftFrameSize);

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

	Blur() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(LENGTH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.0f, "Length CV");
		configParam(LENGTH_KNOB_PARAM, 0.f, 1.f, 0.5f, "Length");

		configParam(POSITION_ATTENUVERTER_PARAM, -1.f, 1.f, 0.0f, "");
		configParam(POSITION_KNOB_PARAM, 0.f, 1.f, 0.f, "Position");
		
		configParam(MOD_RATE_ATTENUVERTER_PARAM, -1.f, 1.f, 0.0f, "");
		configParam(MOD_RATE_KNOB_PARAM, 0.f, 1.f, 0.f, "Mod Rate");
		
		configParam(MOD_DEPTH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.0f, "");
		configParam(MOD_DEPTH_KNOB_PARAM, 0.f, 1.f, 0.f, "Mod Depth");
		
		configParam(SPREAD_ATTENUVERTER_PARAM, -1.f, 1.f, 0.0f, "");
		configParam(SPREAD_KNOB_PARAM, 0.f, 1.f, 0.f, "Spread");
		
		configParam(BIAS_ATTENUVERTER_PARAM, -1.f, 1.f, 0.0f, "");
		configParam(BIAS_KNOB_PARAM, 0.f, 1.f, 0.f, "Bias");
		
		configParam(PHASE_ATTENUVERTER_PARAM, -1.f, 1.f, 0.0f, "");
		configParam(PHASE_KNOB_PARAM, 0.f, 1.f, 0.f, "Phase");
		
		configParam(FREEZE_KNOB_PARAM, 0.f, 1.f, 0.f, "");

		initialize();
	}

	~Blur() {
		fftFramePool.drain();
		fftFrameHistory.deleteMembers();
	}

// From Module parent class 
//	/** Called when the engine sample rate is changed. */
//	virtual void onSampleRateChange() override {}
//	
//	/** Called when user clicks Initialize in the module context menu. */
//	virtual void onReset() override {}
//	
//	/** Called when user clicks Randomize in the module context menu. */
//	virtual void onRandomize() override {}
//	
//	/** Called when the Module is added to the Engine */
//	virtual void onAdd() override {}
//	
//	/** Called when the Module is removed from the Engine */
//	virtual void onRemove() override {}
	
	virtual void process(const ProcessArgs& args) override {

        if (args.sampleRate != fActiveSampleRate) {
			updateSampleRate(args.sampleRate);
		}

		float in = inputs[AUDIO_IN_INPUT].getVoltage();
		inBuffer.values[iInIndex] = in;

		float out = outBuffer.values[ iInIndex - iLatency ];
		outputs[AUDIO_OUT_OUTPUT].setVoltage(out);

		iInIndex++;
		if (iInIndex >= iFftFrameSize) {
			iInIndex = iLatency;
			adjustFrameHistoryLength();
			processFrame();
		}
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
			complexFftEngine.fft(fftWorkspace.values, pFftFrame->values);

			// Remove DC component			
			pFftFrame->values[0] = 0.;

			// Convert Complex real/imag to magnitude/phase
			for (int k = 0; k <= iFftFrameSize/2; k++)
			{
				//if (k <= 2) {
				//	DEBUG(" - after analysis: bin[%d] is %f, %f", k, pFftFrame->values[2*k], pFftFrame->values[2*k+1]);
				//}

				float real = pFftFrame->values[2*k];
				float imag = pFftFrame->values[2*k+1];

				float magnitude = 2. * sqrt(real*real + imag*imag);
				float phase = atan2(imag,real);

				pFftFrame->values[2*k] = magnitude;
				pFftFrame->values[2*k+1] = phase;

				//if (k < 8) {
				//	DEBUG("incoming [%d] mag, phase = %f, %f", k, magnitude, phase);
				//}
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
			//if (k <= 2) {
			//	DEBUG(" before synthesis: bin[%d] is %f, %f", k, fftTemp.values[2*k], fftTemp.values[2*k+1]);
			//}

			float magnitude = fftTemp.values[2*k];
			float phase = fftTemp.values[2*k+1];

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
		complexFftEngine.ifft(fftTemp.values, fftWorkspace.values);
		//complexFftEngine.scale(fftWorkspace.values);

		// Do windowing and add to output accumulator
		float scale = 1.f / (float) ((iFftFrameSize/2) * iOversample);
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
			//	fOutValue -= fTiltFactor;
			//}
			//else { 
			//	fOutValue += fTiltFactor;
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
		float fSpread = params[SPREAD_KNOB_PARAM].getValue();
		//float fPhaseRandomness = params[PHASE_KNOB_PARAM].getValue();
		//DEBUG("- Phase Randomness = %f", fPhaseRandomness);
		
		fPosition += params[POSITION_ATTENUVERTER_PARAM].getValue() * getCvInput(POSITION_CV_INPUT);	
		fPosition = clamp(fPosition, 0.f, 1.f);

        fSpread *= fMaxFrameIndex;

        float fCursor = fPosition * fMaxFrameIndex;

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

			// Slide te spread range to always be inside the range of frames in the frame history
            float fSelected = fCursor + (fSpread  * random.generatePlusMinusOne()); 
			if (fSelected < 0.f) {
				fSelected += fSpread;
			}
			else if (fSelected > fMaxFrameIndex) {
				fSelected -= fMaxFrameIndex;
			}
			//fSelected = clamp(fSelected, 0., fMaxFrameIndex);

			int iIndexBefore = myFloor(fSelected);
			int iIndexAfter = iIndexBefore + 1;

			if (iIndexBefore < 0) {
				iIndexBefore = 0;
			}
			
			if (iIndexAfter >= iNumFrames) {
				iIndexAfter = iNumFrames - 1;
			}

            //if (k == 0) {
			//	DEBUG("  [%d] random = %f", k, fRandomValue);
			//	DEBUG("       Selected = %f", fSelected);
			//	DEBUG("       index (before, after) = (%d, %d)", iIndexBefore, iIndexAfter);
			//}

			FftFrame * pFrameBefore = fftFrameHistory.peekAt(iIndexBefore);
			//if (pFrameBefore == NULL) {
			//	DEBUG("!!! FrameBefore at [%d] is NULL -- num history frames = %d", iIndexBefore, iNumFrames);
			//	DEBUG(" History Buffer: size = %d", fftFrameHistory.size);
			//	DEBUG("                 rear  = %d", fftFrameHistory.rear);
			//	DEBUG("                 front = %d", fftFrameHistory.front);
			//	DEBUG("            population = %d", fftFrameHistory.population);
			//	return;
			//}

			FftFrame * pFrameAfter = fftFrameHistory.peekAt(iIndexAfter);
			//if (pFrameAfter == NULL) {
			//	DEBUG("!!! FrameAfter at [%d] is NULL -- num history frames = %d", iIndexAfter, iNumFrames);
			//	DEBUG(" History Buffer: size = %d", fftFrameHistory.size);
			//	DEBUG("                 rear  = %d", fftFrameHistory.rear);
			//	DEBUG("                 front = %d", fftFrameHistory.front);
			//	DEBUG("            population = %d", fftFrameHistory.population);
			//	return;
			//}

			int fSelectedWhole = myFloor(fSelected);
			float fSelectedFractional = fSelected - float(fSelectedWhole);

            // Linear interpolate: a + t(b-a) where t is [0,1]
			float fValueBefore = pFrameBefore->values[2*k];
			float fValueAfter = pFrameAfter->values[2*k];		
			float fValue = fValueBefore + fSelectedFractional * (fValueAfter - fValueBefore);

			//float phase = phaseAccumulator.values[k] += ((float)k * phaseExpected);
			//phaseAccumulator.values[k] = phase;

			float fPhaseBefore = pFrameBefore->values[2*k+1];
			float fPhaseAfter = pFrameAfter->values[2*k+1];
			float phase = fPhaseBefore + fSelectedFractional * (fPhaseAfter - fPhaseBefore);

            //phase += fPhaseRandomness * random.generatePlusMinusPi();
			//if (fPhaseRandomness >= 1.0f) {
			//	phase = 0; // robot
			//}
            //phase *= 1.0 - fPhaseRandomness;

            outputFrame.values[2*k] = fValue;
			outputFrame.values[2*k+1] = phase;
		}		
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.804, 10.668)), module, Blur::LENGTH_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.055, 10.668)), module, Blur::LENGTH_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.804, 21.017)), module, Blur::POSITION_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.055, 21.017)), module, Blur::POSITION_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.804, 32.461)), module, Blur::MOD_RATE_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.055, 32.461)), module, Blur::MOD_RATE_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.804, 43.552)), module, Blur::MOD_DEPTH_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.055, 43.552)), module, Blur::MOD_DEPTH_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.804, 60.02)), module, Blur::SPREAD_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.055, 60.02)), module, Blur::SPREAD_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.804, 71.111)), module, Blur::BIAS_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.055, 71.111)), module, Blur::BIAS_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.804, 82.307)), module, Blur::PHASE_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.055, 82.307)), module, Blur::PHASE_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(29.399, 97.983)), module, Blur::FREEZE_KNOB_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 10.668)), module, Blur::LENGTH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 21.017)), module, Blur::POSITION_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 32.461)), module, Blur::MOD_RATE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 43.552)), module, Blur::MOD_DEPTH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 60.02)), module, Blur::SPREAD_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 71.111)), module, Blur::BIAS_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.356, 82.307)), module, Blur::PHASE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(41.327, 97.983)), module, Blur::FREEZE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.925, 117.474)), module, Blur::AUDIO_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(62.031, 117.474)), module, Blur::AUDIO_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.833, 108.527)), module, Blur::AUDIO_IN_LED_LIGHT));
	}
};


Model* modelBlur = createModel<Blur, BlurWidget>("Blur");