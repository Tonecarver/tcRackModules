#ifndef _fft_frame_hpp_
#define _fft_frame_hpp_

#include <math.hpp>
#include <dsp/fft.hpp>

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


#endif // _fft_frame_hpp_