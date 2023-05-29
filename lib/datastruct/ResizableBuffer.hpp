#ifndef _resizable_buffer_h_
#define _resizable_buffer_h_  1

#include <assert.h>
#include <memory.h>  // memset()

template<class T>
struct ResizableBuffer
{
public:
    ResizableBuffer()
       : mpBuffer(0)
       , mCapacity(0)
    {
    }

    virtual ~ResizableBuffer()
    {
       delete [] mpBuffer;
    }

    T   * getBuffer() const  { return mpBuffer; }
    int   getCapacity() const { return mCapacity; }

    T    getAt(int idx) const { return mpBuffer[ idx ]; }
    void putAt(int idx, T member) { mpBuffer[ idx ] = member; }

    // discard and regrow to make buffer this capacity 
    void ensureCapacity(int capacityRequired)
    {
       if (capacityRequired > mCapacity)
       {
          int newCapacity = capacityRequired + CAPACITY_OVERGROWTH;
          T * pNewBuffer = new T[ newCapacity ];

          if (pNewBuffer != 0)
          {
             discard();
             mpBuffer  = pNewBuffer;
             mCapacity = newCapacity;
          }
       }
    }

    // Discard dynamically allocated buffers
    // leaves capacity at 0 
    void discard()
    {
       delete [] mpBuffer;
       mpBuffer = 0;
       mCapacity = 0;
    }

    // Set all samples to 0.0
    void zero()
    {
       if (mpBuffer)
       {
          memset(mpBuffer, 0, sizeof mpBuffer[0] * mCapacity);
       }
    }

private:
    T    * mpBuffer;  // ptr to dynamically allocated sample buffer 
    int    mCapacity; // physical capacity of the buffer (number of samples)

    enum {
       CAPACITY_OVERGROWTH = 32
    };

};

#endif // _resizable_buffer_h_
