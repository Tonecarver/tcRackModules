#ifndef _tc_stepper_h_
#define _tc_stepper_h_  1

#include "../../src/plugin.hpp" // for DEBUG 
#include "../random/FastRandom.hpp"
#include "../datastruct/Range.hpp"

enum StepperAlgo
{
   STEPPER_ALGO_CYCLE_UP,
   STEPPER_ALGO_CYCLE_DOWN,
   STEPPER_ALGO_REFLECT,
   STEPPER_ALGO_REFLECT_LINGER,
   STEPPER_ALGO_STAIRS_UP,
   STEPPER_ALGO_STAIRS_DOWN,
   STEPPER_ALGO_STAIRS_UP_DOWN,
   STEPPER_ALGO_FLIP,
   STEPPER_ALGO_CONVERGE,
   STEPPER_ALGO_DIVERGE,
   STEPPER_ALGO_SPRING,  // 
   STEPPER_ALGO_PINKY_UP,
   STEPPER_ALGO_PINKY_DOWN,
   STEPPER_ALGO_PINKY_UP_DOWN,
   STEPPER_ALGO_THUMB_UP,
   STEPPER_ALGO_THUMB_DOWN,
   STEPPER_ALGO_THUMB_UP_DOWN,
   STEPPER_ALGO_RANDOM_WALK,
   STEPPER_ALGO_RANDOM,
   
   // STEPPER_ALGO_STAY_AT_EDGE,
   // STEPPER_ALGO_RESET_TO_NOMINAL,
   ///STEPPER_ALGO_DISCARD_OVERSHOOT,
   //
   kNum_STEPPER_ALGOS
};

// table of Stepper names 
extern const char * stepperAlgoNames[StepperAlgo::kNum_STEPPER_ALGOS];

class Stepper
{
   enum StepDirection {
      FORWARD, 
      REVERSE,
   }; 

public:
   Stepper(int minValue = 0, int maxValue = 1)
      : mRange(minValue, maxValue)
      , mValue(minValue)
      , mPrevValue(minValue)
      , mNominalValue(minValue)
      , mRepeatCount(0)
      , mAlgorithm(StepperAlgo::STEPPER_ALGO_CYCLE_UP)
      , mStepDirection(StepDirection::FORWARD)
      , mStepSize(1)
      , mAscending(true)
      , mStepEven(false)
   {
      prepareAlgorithm();
   }

   int getValue() const             { return mValue; }
   int getMinValue() const          { return mRange.getMinValue(); }
   int getMaxValue() const          { return mRange.getMaxValue(); }
   int getNumSteps() const          { return mRange.size();        }
   int clamp(int value) const       { return mRange.clamp(value);  }
   int getStepSize() const          { return mStepSize;            }
   int getNominalValue() const      { return mNominalValue;        }  
   StepperAlgo getAlgorithm() const { return mAlgorithm;           }
   bool isForward() const     { return mStepDirection == StepDirection::FORWARD; }
   bool isReverse() const     { return mStepDirection == StepDirection::REVERSE; }
   
   void setMinMaxValues(int minValue, int maxValue)
   {
      if (minValue != mRange.getMinValue() || maxValue != mRange.getMaxValue()) {
         mRange.setRange(minValue, maxValue);
         // DEBUG("STEPPER: set range %d to %d", mRange.getMinValue(), mRange.getMaxValue());
         mRange.clamp(mValue);
         mRange.clamp(mPrevValue);
         mRange.clamp(mNominalValue);
      }
   }

   void setStepSize(int stepSize)      
   { 
      if (stepSize > 0 && stepSize != mStepSize)
      {
         // DEBUG("Stepper: set step size %d", stepSize);
         mStepSize = stepSize;   
      }
   }

   void setAlgorithm(StepperAlgo algo)  
   { 
      if (algo != mAlgorithm) {
         mAlgorithm = algo;
         prepareAlgorithm();
      }
   }

   void setNominalValue(int value)  
   { 
      mNominalValue = value;   
   }

   int setAndStepForward(int value)
   {
      // DEBUG("STEPPER: set and step forward: value in = %d, repeat count = %d", value, mRepeatCount);
      mValue = value;
      return forward();
   }

   int setAndStepReverse(int value)
   {
      mValue = value;
      return reverse();
   }

   int forward()
   {
      if (mStepDirection == StepDirection::REVERSE) {
         mStepDirection = StepDirection::FORWARD;
         mAscending ^= true;
         mRepeatCount = 0;
      }

      step_forward();
      // DEBUG("STEPPER: forward(), returning value %d", mValue);
      return mValue;
   }

   int reverse()
   {
      if (mStepDirection == StepDirection::FORWARD) {
         mStepDirection = StepDirection::REVERSE;
         mAscending ^= true;
         mRepeatCount = 0;
      }

      step_reverse();      
      return mValue;
   }


   void resetToNominalValue()
   {
      // mValue = mNominalValue; 
      mValue = mRange.getMinValue();
   }

   void toggleDirection()
   {
      mStepDirection = 
         (mStepDirection == StepDirection::FORWARD ? StepDirection::REVERSE : StepDirection::FORWARD);
   }

private:
   Range<int>    mRange; // min and max values 
   int           mValue;
   int           mPrevValue;
   int           mNominalValue;
   int           mRepeatCount;
   StepperAlgo   mAlgorithm;
   StepDirection mStepDirection;
   int           mStepSize;
   bool          mAscending;    // ascenging or descending phase for algos that have phases: reflect, up-then-down, etc 
   bool          mStepEven;     // toggle to indicate alternating steps: even, odd
   FastRandom    mRandom;


   void resetToMinValue()
   {
      mValue = mRange.getMinValue(); 
   }

   void resetToMinValuePlusOne()
   {
      mValue = mRange.getMinValue() + 1; 
      if (mValue > mRange.getMaxValue())
      {
         mValue = mRange.getMaxValue();
      }
   }

   void resetToMaxValue()
   {
      mValue = mRange.getMaxValue(); 
   }

   void resetToMaxValueMinusOne()
   {
      mValue = mRange.getMaxValue() - 1; 
      if (mValue < mRange.getMinValue())
      {
         mValue = mRange.getMinValue();
      }
   }

   void resetToMiddleValue()
   {
      mValue = mRange.getMiddleValue();
   }

   void resetToRandomValue() {
      mValue = mRange.getMinValue() + round(mRandom.generateZeroToOne() * (mRange.size() - 1));
   }

   void step_up() {
      mValue += mStepSize;
      if (mValue > mRange.getMaxValue()) {
         resetToMinValue();
      } 
   }

   void step_down() {
      mValue -= mStepSize;
      if (mValue < mRange.getMinValue()) {
         resetToMaxValue();
      } 
   }

   void step_reflect() {
      if (mAscending) {
         mValue += mStepSize;
         if (mValue > mRange.getMaxValue()) {
            resetToMaxValueMinusOne();
            mAscending = false;
         } 
      } else { // descending
         mValue -= mStepSize;
         if (mValue < mRange.getMinValue()) {
            resetToMinValuePlusOne();
            mAscending = true;
         } 
      } 
   }

   void step_reflect_linger() { 
      if (mAscending) {
         mValue += mStepSize;
         if (mValue >= mRange.getMaxValue()) {
            if (mRepeatCount == 0) {
               resetToMaxValue();
               mRepeatCount = 1;
               mAscending = false;
            }
         }
      } else { // descending 
         mValue -= mStepSize;
         if (mValue <= mRange.getMinValue()) {
            if (mRepeatCount == 0) {
               resetToMinValue();
               mRepeatCount = 1;
               mAscending = true;
            }
         }
      }
   }

   void step_stairs_up() {
      if (mStepEven) {
         mValue += (mStepSize + 1);
         if (mValue > mRange.getMaxValue()) {
            resetToMinValue();
            mStepEven = false; // next toggle enables step even, results in step up from min
         }         
      } else { // step odd 
         mValue -= mStepSize;
         if (mValue < mRange.getMinValue()) {
            resetToMinValue();
         }         
      }
   }

   void step_stairs_down() {
      if (mStepEven) {
         mValue += mStepSize;  
         if (mValue > mRange.getMaxValue()) {
            resetToMaxValue();
         }         
      } else { // step odd 
         mValue -= (mStepSize + 1);
         if (mValue < mRange.getMinValue()) {
            resetToMaxValue();
            mStepEven = true; // next toggle enables step odd, result is step down from max
         }         
      }
   }

   void step_stairs_up_down() {
      if (mAscending) {
         if (mStepEven) {
            mValue += (mStepSize + 1);
            if (mValue > mRange.getMaxValue()) {
               resetToMaxValue();
               mAscending = false;
            }         
         } else { // step odd 
            mValue -= mStepSize;
            if (mValue < mRange.getMinValue()) {
               resetToMinValue();
            }         
         }
      } else { // descending
         if (mStepEven) {
            mValue += mStepSize;  
            if (mValue > mRange.getMaxValue()) {
               resetToMaxValue();
            }         
         } else { // step odd 
            mValue -= (mStepSize + 1);
            if (mValue < mRange.getMinValue()) {
               resetToMinValue();
               mAscending = true;
               mStepEven = false; // next toggle enables step even, result is step up from min
            }         
         }
      }
   }

   void step_flip_up() {
      if (mAscending) {
         mValue += mStepSize;
         if (mValue > mRange.getMaxValue()) {
            resetToMiddleValue();
            mAscending = false;
         }
      } else { // descending
         mValue -= mStepSize;
         if (mValue < mRange.getMinValue()) {
            resetToMiddleValue();
            mAscending = true;
         }
      }
   }

   void step_flip_down() {
      int middleValue = mRange.getMiddleValue();
      if (mAscending) {
         mValue += mStepSize;
         if (mValue > middleValue) {
            resetToMaxValue();
            mAscending = false;
         }
      } else { // descending
         mValue -= mStepSize;
         if (mValue < middleValue) {
            resetToMinValue();
            mAscending = true;
         }
      }
   }

   void step_converge() {
      int middleValue = mRange.getMiddleValue();
      int delta = mValue - middleValue;
      if (delta == 0) { // at middle 
         resetToMaxValue();
      } else if (delta > 0)  { // above middle
         mValue = mRange.getMinValue() + (mRange.getMaxValue() - mValue);
      } else { // below middle
         mValue = mRange.getMaxValue() - ((mValue - mRange.getMinValue()) + 1);
      }
   }

   void step_diverge() {
      int middleValue = mRange.getMiddleValue();
      int delta = mValue - middleValue;
      if (delta == 0) { // at middle 
         mValue -= mStepSize;
      } else if (delta > 0)  { // above middle
         mValue = middleValue - (delta + 1);
         if (mValue < mRange.getMinValue()) {
            mValue = middleValue;
         }
      } else { // below middle
         mValue = (middleValue - delta);
         if (mValue > mRange.getMaxValue()) {
            mValue = middleValue;
         }
      }
   }

   void step_converge_diverge() {
      int middleValue = mRange.getMiddleValue();
      int delta = mValue - middleValue;
      if (mAscending && delta == 0) { // converging, at middle 
         mAscending = false; 
      }

      if (mAscending) { // converving towards middle 
         if (delta > 0)  { // above middle
            mValue = mRange.getMinValue() + (mRange.getMaxValue() - mValue);
         } else { // below middle
            mValue = mRange.getMaxValue() - ((mValue - mRange.getMinValue()) + 1);
         }
      } else { // descending phase, diverging towards min, max 
         if (delta == 0) { // at middle 
            mValue -= mStepSize;
         } else if (delta > 0)  { // above middle
            mValue = middleValue - (delta + 1);
            if (mValue < mRange.getMinValue()) {
               resetToMinValue();
               mAscending = true;
            }
         } else { // below middle
            mValue = (middleValue - delta);
            if (mValue > mRange.getMaxValue()) {
               resetToMaxValue();
               mAscending = true;
            }
         }
      }
   }

   void step_random_walk() { 
      mValue += int(mRandom.generateZeroToOne() > 0.5f ? 1 : -1) * mStepSize;
      if (! mRange.includes(mValue)) {
         mValue = mRange.getMiddleValue() + mRandom.generatePlusMinusOne(); 
         mValue = mRange.clamp(mValue);
      }
   }

   void step_pinky_up() { 
      if (mStepEven) {
         resetToMaxValue();
      } else { // step odd 
         mValue = mPrevValue + mStepSize;
         if (mValue >= mRange.getMaxValue()) {
            resetToMinValue();
         }
         mPrevValue = mValue;
      }
   }

   void step_pinky_down() { 
      if (mStepEven) {
         resetToMaxValue();
      } else { // descending 
         mValue = mPrevValue - mStepSize;
         if (mValue < mRange.getMinValue()) {
            resetToMaxValueMinusOne();
         }
         mPrevValue = mValue;
      }
   }

   void step_pinky_up_down() { 
      if (mStepEven) {
         resetToMaxValue();
      } else { // odd step 
         if (mAscending) {
            mValue = mPrevValue + mStepSize;
            if (mValue >= mRange.getMaxValue()) {
               resetToMaxValueMinusOne();
               mAscending = false;
            }
         } else { // descending 
            mValue = mPrevValue - mStepSize;
            if (mValue < mRange.getMinValue()) {
               resetToMinValue();
               mAscending = true;
            }
         }
         mPrevValue = mValue;
      }
   }

   void step_thumb_up() { 
      if (mStepEven) {
         resetToMinValue();
      } else { // step odd
         mValue = mPrevValue + mStepSize;
         if (mValue > mRange.getMaxValue()) {
            resetToMinValuePlusOne();
         }
         mPrevValue = mValue;
      }
   }

   void step_thumb_down() { 
      if (mStepEven) {
         resetToMinValue();
      } else { // step odd 
         mValue = mPrevValue - mStepSize;
         if (mValue <= mRange.getMinValue()) {
            resetToMaxValue();
         }
         mPrevValue = mValue;
      }
   }

   void step_thumb_up_down() { 
      if (mStepEven) {
         resetToMinValue();
      } else { // odd step 
         if (mAscending) {
            mValue = mPrevValue + mStepSize;
            if (mValue > mRange.getMaxValue()) {
               resetToMaxValueMinusOne();
               mAscending = false;
            }
         } else { // descending phase 
            mValue = mPrevValue - mStepSize;
            if (mValue <= mRange.getMinValue()) {
               resetToMinValuePlusOne();
               mAscending = true;
            }
         }
         mPrevValue = mValue;
      }
   }


   void prepareAlgorithm() {
      mStepEven = true; // initialize step toggle 
      mRepeatCount = 1; // prepare sets the next value, so use it before invoking the step_xxxx() function
      mAscending = true;

      // DEBUG("STEPPER: prep algo %d, %s, range %d to %d", mAlgorithm, stepperAlgoNames[mAlgorithm], mRange.getMinValue(), mRange.getMaxValue());

      switch (mAlgorithm) {
         case StepperAlgo::STEPPER_ALGO_CYCLE_UP:
            resetToMinValue();
            break;
         case StepperAlgo::STEPPER_ALGO_CYCLE_DOWN:
            resetToMaxValue();
            break;
         case StepperAlgo::STEPPER_ALGO_REFLECT:
            resetToMinValue();
            break;
         case StepperAlgo::STEPPER_ALGO_REFLECT_LINGER:
            resetToMinValue();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_UP:
            resetToMinValue();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_DOWN:
            resetToMaxValue();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_UP_DOWN:
            resetToMinValue();
            break;
         case StepperAlgo::STEPPER_ALGO_FLIP:
            resetToMiddleValue();  // << TODO: nominal value ? 
            break;
         case StepperAlgo::STEPPER_ALGO_CONVERGE:
            resetToMaxValue();
            break;
         case StepperAlgo::STEPPER_ALGO_DIVERGE:
            resetToMiddleValue();
            break;
         case StepperAlgo::STEPPER_ALGO_SPRING:
            resetToMaxValue();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_UP:
            resetToMaxValue();
            mPrevValue = mRange.getMinValue();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_DOWN:
            resetToMaxValue();
            mPrevValue = mRange.getMinValue();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_UP_DOWN:
            resetToMaxValue();
            mPrevValue = mRange.getMinValue();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_UP:
            resetToMinValue();
            mPrevValue = mRange.getMaxValue();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_DOWN:
            resetToMinValue();
            mPrevValue = mRange.getMaxValue();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_UP_DOWN:
            resetToMinValue();
            mPrevValue = mRange.getMaxValue();
            break;
         case StepperAlgo::STEPPER_ALGO_RANDOM_WALK:
            resetToMiddleValue();
            break;
         case StepperAlgo::STEPPER_ALGO_RANDOM:
            resetToRandomValue();
            break;
//         case StepperAlgo::STEPPER_ALGO_RESET_TO_NOMINAL:
//            break;
//         case StepperAlgo::STEPPER_ALGO_DISCARD_OVERSHOOT:
//            break;
      // stay at edge 
         default:
            DEBUG("Unhandled algo in prepareAlgorithm(), %d", mAlgorithm);
      }
   }

   void step_forward() {

      if (mRepeatCount > 0) {
         mRepeatCount--;
         return; // leave mValue as it is 
      }

      mStepEven ^= true;

      switch (mAlgorithm) {
         case StepperAlgo::STEPPER_ALGO_CYCLE_UP:
            step_up();
            break;
         case StepperAlgo::STEPPER_ALGO_CYCLE_DOWN:
            step_down();
            break;
         case StepperAlgo::STEPPER_ALGO_REFLECT:
            step_reflect();
            break;
         case StepperAlgo::STEPPER_ALGO_REFLECT_LINGER:
            step_reflect_linger();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_UP:
            step_stairs_up();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_DOWN:
            step_stairs_down();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_UP_DOWN:
            step_stairs_up_down();
            break;
         case StepperAlgo::STEPPER_ALGO_FLIP:
            step_flip_up();
            break;
         case StepperAlgo::STEPPER_ALGO_CONVERGE:
            step_converge();
            break;
         case StepperAlgo::STEPPER_ALGO_DIVERGE:
            step_diverge();
            break;
         case StepperAlgo::STEPPER_ALGO_SPRING:
            step_converge_diverge();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_UP:
            step_pinky_up();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_DOWN:
            step_pinky_down();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_UP_DOWN:
            step_pinky_up_down();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_UP:
            step_thumb_up();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_DOWN:
            step_thumb_down();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_UP_DOWN:
            step_thumb_up_down();
            break;
         case StepperAlgo::STEPPER_ALGO_RANDOM_WALK:
            step_random_walk();
            break;
         case StepperAlgo::STEPPER_ALGO_RANDOM:
            resetToRandomValue();
            break;
//         case StepperAlgo::STEPPER_ALGO_RESET_TO_NOMINAL:
//            break;
//         case StepperAlgo::STEPPER_ALGO_DISCARD_OVERSHOOT:
//            break;
      // stay at edge 
         default:
            DEBUG("Unhandled algo in step forward(), %d", mAlgorithm);
      }
   }

   void step_reverse() {

      if (mRepeatCount > 0) {
         mRepeatCount--;
         return; // leave mValue as it is 
      }

      mStepEven ^= true;

      switch (mAlgorithm) {
         case StepperAlgo::STEPPER_ALGO_CYCLE_UP:
            step_down();
            break;
         case StepperAlgo::STEPPER_ALGO_CYCLE_DOWN:
            step_up();
            break;
         case StepperAlgo::STEPPER_ALGO_REFLECT:
            step_reflect();
            break;
         case StepperAlgo::STEPPER_ALGO_REFLECT_LINGER:
            step_reflect_linger();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_UP:
            step_stairs_down();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_DOWN:
            step_stairs_up();
            break;
         case StepperAlgo::STEPPER_ALGO_STAIRS_UP_DOWN:
            step_stairs_up_down();
            break;
         case StepperAlgo::STEPPER_ALGO_FLIP:
            step_flip_down();
            break;
         case StepperAlgo::STEPPER_ALGO_CONVERGE:
            step_diverge();
            break;
         case StepperAlgo::STEPPER_ALGO_DIVERGE:
            step_converge();
            break;
         case StepperAlgo::STEPPER_ALGO_SPRING:
            step_converge_diverge();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_UP:
            step_pinky_down();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_DOWN:
            step_pinky_up();
            break;
         case StepperAlgo::STEPPER_ALGO_PINKY_UP_DOWN:
            step_pinky_up_down();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_UP:
            step_thumb_down();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_DOWN:
            step_thumb_up();
            break;
         case StepperAlgo::STEPPER_ALGO_THUMB_UP_DOWN:
            step_thumb_up_down();
            break;
         case StepperAlgo::STEPPER_ALGO_RANDOM_WALK:
            step_random_walk();
            break;
         case StepperAlgo::STEPPER_ALGO_RANDOM:
            resetToRandomValue();
            break;
//         case StepperAlgo::STEPPER_ALGO_RESET_TO_NOMINAL:
//            break;
//         case StepperAlgo::STEPPER_ALGO_DISCARD_OVERSHOOT:
//            break;
      // stay at edge 
         default:
            DEBUG("Unhandled algo in step reverse(), %d", mAlgorithm);
      }
   }


};

#endif //