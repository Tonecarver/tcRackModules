#ifndef _tc_range_h_
#define _tc_range_h_ 1

template <class T>
class Range {
    T mMinValue;
    T mMaxValue;
public:
    Range(const T minVal, const T maxVal) {
        setRange(minVal, maxVal);
    }
   
    void setRange(const T minVal, const T maxVal) { 
        if (minVal <= maxVal) {
            mMinValue = minVal; 
            mMaxValue = maxVal;
        } else { 
            mMinValue = maxVal; // swap min/max
            mMaxValue = minVal;
        }
    }

    void setMinVal(const T minVal) { 
        setRange(minVal, mMaxValue);
    }

    void setMaxVal(const T maxVal) { 
        setRange(mMinValue, maxVal);
    }

    inline T getMinValue() const {
        return mMinValue;
    }

    inline T getMaxValue() const {
        return mMaxValue;
    }

    inline T getMiddleValue() const {
        return mMinValue + (size() / 2);
    }    

    inline T size() const {
        return (mMaxValue - mMinValue) + 1;
    }    

    inline bool includes(const T val) const { 
        return (mMinValue <= val) && (val <= mMaxValue);
    } 

    T clamp(const T val) const {
        return std::min( std::max( val, mMinValue ), mMaxValue );
    }

}; 

#endif // _tc_range_h_
