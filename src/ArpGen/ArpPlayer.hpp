#pragma once
#ifndef _arp_player_h_
#define _arp_player_h_ 1

#include "ArpPlayerConstants.hpp"

//enum ArpPlayerConstants 
//{ 
//   ARP_PLAYER_NUM_HARMONIES = 4 
//};


#include "../../lib/scale/TwelveToneScale.hpp"
#include "../../lib/arpeggiator/Stepper.hpp"
// #include "MidiPitchPool.hpp"
// #include "LiveMidiEvent.h"
// #include "FunctionType.h"
// #include "ArpTerm.h"
// #include "KeyState.h"
// #include "VelocityManager.h"

#include "NoteTable.hpp"

#include "LSystem/LSystemExecutor.hpp"
#include "LSystem/LSystemProduction.hpp"

#include "../../lib/datastruct/InstanceStack.hpp"
#include "../../lib/datastruct/Stack.hpp"
#include "../../lib/datastruct/PtrArray.hpp"
#include "../../lib/datastruct/FreePool.hpp"

//#include "debugtrace.h"

#include "SortingOrder.hpp"



enum KeysOffStrategy
{
   KEYS_OFF_MAINTAIN_CONTEXT,  // maintain context and play position when no keys are active
   KEYS_OFF_RESET_CONTEXT,     // reset to beginning and clear context when nokeys are active
   //
   kNUM_KEYS_OFF_STRATEGIES
};

inline bool isValidHarmonyId(int harmonyId) 
{
   return ((0 <= harmonyId) && (harmonyId < ARP_PLAYER_NUM_HARMONIES));
}



// enum VelocityMode
// {
//    VELOCITY_MODE_AS_PLAYED,
//    VELOCITY_MODE_PER_PATTERN,
//    // VELOCITY_MODE_FIXED,  / ?? 
//    // 
//    kNUM_VELOCITY_MODE
// };



// bridge to midi output queue 
class ArpNoteWriter
{
public:
   //virtual void noteOn(int channelNumber, int noteNumber, int velocity, int lengthSamples, int delaySamples) = 0;
   virtual void noteOn(int channelNumber, int noteNumber, int velocity) = 0;
}; 

class ArpContext : public ListNode
{
public:
   ArpContext()
      : mNoteIndex(0)
      , mOctaveOffset(0)
      , mSemitoneOffset(0)
      // , mVelocityIncrement(0)
      // , mNoteLengthMultiplier(1.0)
      // , mGlissSpeed(0.25) 
      // , mGlissDirectionUpward(true) 
   {
      clearHarmonyIntervals();
   }

   virtual ~ArpContext()
   {
   }

   void copyFrom(ArpContext * pOther)
   {
      (*this) = (*pOther);
      //mNoteIndex = pOther->mNoteIndex;
      //mOctaveOffset = pOther->mOctaveOffset;
      //mSemitoneOffset = pOther->mSemitoneOffset;
      //mVelocityIncrement = pOther->mVelocityIncrement;
      //mNoteLengthMultiplier = pOther->mNoteLengthMultiplier;
      //mHarmonyInterval = pOther->mHarmonyInterval;
      //gliss
   }

   void setNoteIndex(int idx)  { mNoteIndex = idx; }
   int  getNoteIndex() const   { return mNoteIndex; }

   void setOctaveOffset(int offset)  { mOctaveOffset = offset; }
   int  getOctaveOffset() const      { return mOctaveOffset; }

   void setSemitoneOffset(int offset)  { mSemitoneOffset = offset; }
   int  getSemitoneOffset() const      { return mSemitoneOffset; }

   // void setVelocityIncrement(int increment)  { mVelocityIncrement = increment; }
   // int  getVelocityIncrement()            { return mVelocityIncrement; }

   // float getNoteLengthMultiplier() { return mNoteLengthMultiplier; }
   // void  setNoteLengthMultiplier(float multiplier) { mNoteLengthMultiplier = multiplier; }

   int getHarmonyInterval(int harmonyId) const 
   { 
      if (isValidHarmonyId(harmonyId))
      {
         return mHarmonyIntervals[ harmonyId ]; 
      }
      return 0;
   }

   void setHarmonyInterval(int harmonyId, int interval) 
   { 
      if (isValidHarmonyId(harmonyId))
      {
         mHarmonyIntervals[ harmonyId ] = interval;
      }
   }

   // void  setGlissSpeed(float speedZeroOne)  { mGlissSpeed = speedZeroOne;   }
   // float getGlissSpeed() const    {      return mGlissSpeed;    }

   // void setGlissDirectionUpward(bool beUpward)   {      mGlissDirectionUpward = beUpward;   }
   // bool isGlissDirectionUpward() const    {      return mGlissDirectionUpward;   }

   void clear()
   {
      mNoteIndex = 0;
      mOctaveOffset = 0;
      mSemitoneOffset = 0;
//      mVelocityIncrement = 0;
//      mNoteLengthMultiplier = 1.0;
      clearHarmonyIntervals();
      // mGlissSpeed = 0.25;
      // mGlissDirectionUpward = true;
   }

private:
   int mNoteIndex;        // note index = index into the active notes 
   int mOctaveOffset;     // octave multiplier 
   int mSemitoneOffset;   // semitone offset 
//   int mVelocityIncrement; // velocity 
//    float mNoteLengthMultiplier; // note length scaler
   int mHarmonyIntervals[ ARP_PLAYER_NUM_HARMONIES ];
   // float mGlissSpeed;           // 0 .. 1
   // bool mGlissDirectionUpward;

   void clearHarmonyIntervals()
   {
      for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
      {
         mHarmonyIntervals[ i ] = 0;
      }
   }
};




class ArpPlayer : public LSystemExecutor
{
public:

   ArpPlayer();

   virtual ~ArpPlayer()
   {
   }

   void setNoteWriter(ArpNoteWriter * pArpNoteWriter)
   {
      mpArpNoteWriter = pArpNoteWriter;
   }

   // ==============================================================
   //    Note 

   // process note plays normally (increment all note/octave indexes etc.) but suppress note emit
   // if noteEnabled == false
   void setNoteEnabled(bool beEnabled)
   {
      mNoteEnabled = beEnabled;
   }

   bool isNoteEnabled()
   {
      return mNoteEnabled;
   }

	void setNotes(int numNotes, int * pNotes) {
      mNoteTable.setNotes(numNotes, pNotes);
      if (mNoteTable.isEmpty()) {
         if (isResetOnKeysEmpty()) {
            resetScanToBeginning();
            resetToInitialContext();            
         }
      }
      updateArpStepper();
   }

   // sets the step size when increasing the semitone INTERVAL
   void setIntervalStepSize(int stepSize) {
      mSemitoneIncrementStepper.setStepSize(stepSize);
      for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
      {
         mHarmonyIntervalSteppers[i].setStepSize(stepSize);
      }
   }

   int getIntervalStepSize() const {
      return mSemitoneIncrementStepper.getStepSize();
   } 

   // void setNoteDelaySamples(int delaySamples)
   // {
   //    mNoteDelaySamples = delaySamples;
   // }

   // int getNoteDelaySamples()
   // {
   //    return mNoteDelaySamples;
   // }

   // void setNoteChannel(int channel)
   // {
   //    mNoteChannel = channel;
   // }

   // int getNoteChannel()
   // {
   //    return mNoteChannel;
   // }

   // ==============================================================
   //    Note length 

   // void setNominalNoteLengthSamples(int noteLengthSamples)
   // {
   //    if (noteLengthSamples >= 0)
   //    {
   //       mNominalNoteLengthSamples = noteLengthSamples;
   //    }
   // }

   // int getNominalNoteLengthSamples()
   // {
   //    return mNominalNoteLengthSamples;
   // }

   // ==============================================================
   //    Note play order 

   void setPlayOrder(SortingOrder sortingOrder)
   {
      mNoteSortOrder = sortingOrder;
   }

   bool isPlayOrderAsReceived() const
   {
      return mNoteSortOrder == SortingOrder::SORT_ORDER_AS_RECEIVED;
   }

   bool isPlayOrderLowToHigh() const
   {
      return mNoteSortOrder == SortingOrder::SORT_ORDER_LOW_TO_HIGH;
   }

   bool isPlayOrderHighToLow() const
   {
      return mNoteSortOrder == SortingOrder::SORT_ORDER_HIGH_TO_LOW;
   }

   // bool isPlayOrderRandom() const
   // {
   //    return mNoteSortOrder == SortingOrder::SORT_ORDER_RANDOM;
   // }


   // ==============================================================
   //    Note stride

   void setNoteStride(int stride)
   { 
      mNoteStride = stride; // ? TODO member variable not needed 
      mArpStepper.setStepSize(stride);
   }

   int getNoteStride() const
   { 
      return mNoteStride; 
   }


   // // ==============================================================
   // //    Note Relative Velocity (manual offset to natural velocity)

   // void setNoteRelativeVelocity(int velocityIncrement)
   // {
   //    mNoteRelativeVelocity = velocityIncrement;
   // }

   // ==============================================================
   //    Note Probability

   void setNoteProbability(float probabilityZeroOne)
   {
      mNoteProbability = probabilityZeroOne;
   }

   // // ==============================================================
   // //    Note Range

   // void configureNoteRanges() { 
   //    // if relative to tonic, center = tonic 
   //    // if free, center = mOctaveCenter
   //    int centerNote = mTwelveToneScale.getTonicPitch();
   //    int noteMin = clamp((centerNote + (mOctaveOffsetMin * 12)), 0, 120); // TODO const 
   //    int noteMax = clamp((centerNote + (mArpOctaveSpan * 12) + 11), 0, 120); // TODO const 
   //    if (mOctaveIncludesPerfect) {
   //       noteMax += 1;
   //    }
   //    //mArpNoteRange.setRange(noteMin, noteMax);
   //    mArpNoteRange.setRange(0, 120);  // TODO: either use this or get rid of it .. setting to 0..120 makes all notes in range
   // }

   // // ==============================================================
   // //    Harmony Note Range

   // void configureHarmonyNoteRanges() { 
   //    // if relative to tonic, center = tonic 
   //    // if free, center = mOctaveCenter
   //    int centerNote = mTwelveToneScale.getTonicPitch();
   //    int noteMin = clamp((centerNote + (mHarmonyOctaveOffsetMin * 12)), 0, 120); // TODO const 
   //    int noteMax = clamp((centerNote + (mHarmonyOctaveOffsetMax * 12) + 11), 0, 120); // TODO const 
   //    if (mOctaveIncludesPerfect) {
   //       noteMax += 1;
   //    }
   //    // mHarmonyNoteRange.setRange(noteMin, noteMax);
   //    mHarmonyNoteRange.setRange(0, 120);  // TODO: either use this or get rid of it .. setting to 0..120 makes all notes in range
   // }

/**
 * TODO: delete comments
 * TODO: specify octave using width and offset from tonic 
 * octave width = 0, 1, 2, ... 
 * octave center = -n .. 0 .. n 
 *     0 = scale.tonic()
 *     n = number of octaves to shift from tonic
 * 
 * setOctave(width, tonicOffset)
 *    tonic = scale.getTonicPitch()
 *    noteLow = tonic + (tonicOffset * 12)
 *    noteHigh = noteLow + (octaveWidth * 12) + 11 // if octaveWidth range starts at 1, then "noteHigh = noteLow + (octaveWidth * 12) - 1"
 *    if perfectIncluded
 *        noteHogh += 1
 *    noteLow = clamp(noteLow, 0, 120)
 *    notehigh = clamp(notehigh, 0, 120)
 *    octaveRange.setRange(noteLow, noteHigh)
*/

   // ==============================================================
   //    Note Range

   // void setArpNoteRange(int lowNote, int highNote) {
   //    mArpNoteRange.setRange(lowNote, highNote);
   // }

   // ==============================================================
   //    Harmony Note Range

   void setHarmonyNoteRange(int lowNote, int highNote) {
      mHarmonyNoteRange.setRange(lowNote, highNote);
   }

   // // ==============================================================
   // //    Octave Range

   void setArpOctaveRange(int minValue, int maxValue)
   {
      if (minValue != mArpOctaveOffsetMin || maxValue != mArpOctaveOffsetMax) {
         mArpOctaveOffsetMin = minValue;
         mArpOctaveOffsetMax = maxValue;      
         updateArpStepper();
      }
   }

   void setOctaveIncludesPerfect(bool includePerfect)
   {
      if (mOctaveIncludesPerfect != includePerfect)
      {
         mOctaveIncludesPerfect = includePerfect;
         updateArpStepper();
         updateIntervalSteppers();
      }
   }

//    void setOctaveOffset(float octaveOffset)
//    {
//       if (mArpOctaveOffset != octaveOffset)
//       {  
// //          DEBUG("ARP Octave Transpose changed: was %f, is %f", mArpOctaveOffset, octaveOffset);
//          mArpOctaveOffset = octaveOffset;
//          //  TODO: updateNoteRanges(); ??
//       }
//    }


   // // ==============================================================
   // //    Harmony Octave Range

   void setHarmonyOctaveRange(int minValue, int maxValue)  // TODO: change to SPAN 
   {
      if (minValue > maxValue)
      {
         int temp = minValue; // swap  
         minValue = maxValue;
         maxValue = temp; 
      }

      if ((mHarmonyOctaveOffsetMin != minValue) || (mHarmonyOctaveOffsetMax != maxValue))
      {  
       // DEBUG("Harmony Octave Range changed: min was %d, is %d", mHarmonyOctaveOffsetMin, minValue);
       // DEBUG("Harmony Octave Range changed: max was %d, is %d", mHarmonyOctaveOffsetMax, maxValue);
         mHarmonyOctaveOffsetMin = minValue;
         mHarmonyOctaveOffsetMax = maxValue;
         updateIntervalSteppers();
//          configureHarmonyNoteRanges();
      }
      // NO: min and max are the INTERVAL spans, add seperate method for this
      // mHarmonyIntervalSteppers[x].setMinMaxValues(mHarmonyOctaveOffsetMin, mHarmonyOctaveOffsetMax);
      // TODO: investigate this 
   }

//    void setHarmonyOctaveTranspose(int octaveTranspose)
//    {
//       if (mHarmonyOctaveTranspose != octaveTranspose)
//       {  
// //          DEBUG("Harmony Octave Transpose changed: was %d, is %d", mHarmonyOctaveTranspose, octaveTranspose);
//          mHarmonyOctaveTranspose = octaveTranspose;
//          updateIntervalSteppers();  // TODO: is this needed here ? do this on note range change 
// //          configureHarmonyNoteRanges();
//       }
//       // NO: min and max are the INTERVAL spans, add seperate method for this
//       // mHarmonyIntervalSteppers[x].setMinMaxValues(mHarmonyOctaveOffsetMin, mHarmonyOctaveOffsetMax);
//       // TODO: investigate this 
//    }

   void setHarmonyOvershootStrategy(StepperAlgo algorithm)
   {
      for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
      {
         mHarmonyIntervalSteppers[i].setAlgorithm(algorithm);
      }
   }

   void setHarmonyEnabled(int harmonyId, bool beEnabled)
   {
      if (isValidHarmonyId(harmonyId))
      {
         mHarmonyEnabled[ harmonyId ] = beEnabled;
      }
   }

   bool isHarmonyEnabled(int harmonyId)
   {
      if (isValidHarmonyId(harmonyId))
      {
         return mHarmonyEnabled[ harmonyId ];
      }
      return false;
   }

   void randomizeHarmonies() 
   {
      float numIntervals = mHarmonyIntervalSteppers[0].getNumSteps();
      int minInterval = mHarmonyIntervalSteppers[0].getMinValue();

      StackIterator<ArpContext> iter(mContextStack);
      while (iter.hasMore()) {
         ArpContext * pArpContext = iter.getNext();

         int intervalSeen[ ARP_PLAYER_NUM_HARMONIES ];
         for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++) {
            int interval = minInterval + int(getRandomZeroToOne() * numIntervals);
            for (int k = 0; k < i; k++) {
               if (interval == intervalSeen[k]) {
                  interval = minInterval + int(getRandomZeroToOne() * numIntervals);
               }
            }
            intervalSeen[i] = interval;
            pArpContext->setHarmonyInterval(i, interval);
         }
      }
   }


   // void setHarmonyVelocityMultiplier(int harmonyId, float velocityMultiplier)
   // {
   //    if (isValidHarmonyId(harmonyId))
   //    {
   //       mHarmonyVelocityMultiplier[ harmonyId ] = velocityMultiplier;
   //    }
   // }

   // float getHarmonyVelocityMultiplier(int harmonyId)
   // {
   //    if (isValidHarmonyId(harmonyId))
   //    {
   //       return mHarmonyVelocityMultiplier[ harmonyId ];
   //    }
   //    return 0.0;
   // }

   // void setHarmonyChannel(int harmonyId, int channelId) // 0..16, 0 = asReceived
   // {
   //    if (isValidHarmonyId(harmonyId))
   //    {
   //       mHarmonyChannel[ harmonyId ] = channelId;
   //    }
   // }

   // int getHarmonyChannel(int harmonyId) // 0..16, 0 = asReceived
   // {
   //    if (isValidHarmonyId(harmonyId))
   //    {
   //       return mHarmonyChannel[ harmonyId ];
   //    }
   //    return 0;
   // }


   // void setHarmonyRelativeVelocity(int velocityIncrement)
   // {
   //    mHarmonyRelativeVelocity = velocityIncrement;
   // }

   // void setHarmonyDelay(int harmonyId, int delaySamples)
   // {
   //    if (isValidHarmonyId(harmonyId))
   //    {
   //       mHarmonyDelaySamples[ harmonyId ] = delaySamples;
   //    }
   // }

   // int getHarmonyDelaySamples(int harmonyId)
   // {
   //    if (isValidHarmonyId(harmonyId))
   //    {
   //       return mHarmonyDelaySamples[ harmonyId ];
   //    }
   //    return 0;
   // }

   // ==============================================================
   //    Harmony Probability

   void setHarmonyProbability(float probabilityZeroOne)
   {
      mHarmonyProbability = probabilityZeroOne;
   }


   // // ==============================================================
   // //    Velocity

   // void setNumVelocityDivisions(int numDivisions)
   // {
   //    mVelocityManager.setNumDivisions(numDivisions);
   // }

   // void setNominalVelocity(int nominalVelocity)
   // {
   //    mVelocityManager.setNominalVelocity(nominalVelocity);
   // }

   // void setVelocityMode(VelocityMode mode)
   // {
   //    mVelocityMode = mode; 
   // }

   // ==============================================================
   //    Gliss (strum)

   // void setGlissSpeedMinimum(float speedZeroOne)
   // {
   //    mGlissSpeedBounder.setMinValue(  clamp(speedZeroOne, 0.0, 1.0) );
   // }

   // void setGlissSpeedMaximum(float speedZeroOne)
   // {
   //    mGlissSpeedBounder.setMaxValue(  clamp(speedZeroOne, 0.0, 1.0) );
   // }

   // void setGlissSpeedNominal(float speedZeroOne)
   // {
   //    mGlissSpeedBounder.setNominalValue(  clamp(speedZeroOne, 0.0, 1.0) );
   // }

   // // ==============================================================
   // //    Key press Mode 

   // KeyPressMode getKeyPressMode()
   // {
   //    return mMidiPitchPool.getMode();
   // }

   // void setKeyPressMode(KeyPressMode mode)
   // {
   //    mMidiPitchPool.setMode(mode); 
   // }

   // ==============================================================
   //    Note Overstep action 

   void setArpStepperStyle(StepperAlgo style)
   {
      mArpStepper.setAlgorithm(style);

      // // This works .. but seems odd to change immediate play direction for an 'overshoot' stretegy 
      // // if ((boundsType == BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_DOWN && mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_UP)
      // //  || (boundsType == BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_UP   && mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_DOWN)) {
      // //    toggleIncrementDirection();
      // // }
      // mOverstepAction = style;
   }

   // StepperAlgo getArpStepperStyle() const
   // {
   //    return mOverstepAction;
   // }

   // bool isCycleUpwardOnOutOfBounds() const
   // {
   //    return mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_UP;
   // }

   // bool isCycleDownwardOnOutOfBounds() const
   // {
   //    return mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_DOWN;
   // }

   // bool isReflectOnOutOfBounds() const
   // {
   //    return mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_REFLECT;
   // }

   // bool isReflectLingerOnOutOfBounds() const
   // {
   //    return mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_REFLECT_LINGER;
   // }

   // bool isFlipOnOutOfBounds() const
   // {
   //    return mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_FLIP;
   // }

   // // bool isStayAtEdgeOnOutOfBounds() const
   // // {
   // //    return mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_STAY_AT_EDGE;
   // // }

   // bool isResetToNominalOnOutOfBounds() const
   // {
   //    return mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_RESET_TO_NOMINAL;
   // }

   // bool isDiscardOnOutOfBounds() const
   // {
   //    return mOverstepAction == BoundaryStrategy::BOUNDARY_STRATEGY_DISCARD_OVERSHOOT;
   // }

   // ==============================================================
   //     Musical Scale

   void setRootPitch(int pitch)
   {
      mTwelveToneScale.setTonicPitch(pitch);
      //configureNoteRanges();
      //configureHarmonyNoteRanges();
   }

   int getRootPitch() const
   {
      return mTwelveToneScale.getTonicPitch();
   }

   void setScaleDefinition(const ScaleDefinition * pScaleDefinition)
   {
      if (pScaleDefinition != 0)
      {
       // DEBUG("ArpPlayer: set scale: %s", pScaleDefinition->name.c_str());
         mTwelveToneScale.setScale(pScaleDefinition);
         updateArpStepper();
         updateIntervalSteppers();
//         configureNoteRanges();
//         configureHarmonyNoteRanges();
      }
   }

   const TwelveToneScale & getTwelveToneScale() const {
      return mTwelveToneScale;
   }

   bool hasScale() const
   {
      return mTwelveToneScale.isConfigured();
   }

   void setScaleOutlierStrategy(ScaleOutlierStrategy strategy)
   {
      if (mTwelveToneScale.getOutlierStrategy() != strategy)
      {
         mTwelveToneScale.setOutlierStrategy(strategy);
         updateArpStepper();
         updateIntervalSteppers();
         //configureNoteRanges(); // ? 
         //configureHarmonyNoteRanges(); // ? 
      }
   }

   ScaleOutlierStrategy getScaleOutlierStrategy() const
   {
      return mTwelveToneScale.getOutlierStrategy();
   }

   // ==============================================================
   //    Semitone Offset 

   void setSemitoneOvershootStrategy(StepperAlgo algorithm)
   {
      mSemitoneIncrementStepper.setAlgorithm(algorithm);
   }

   // // ==============================================================
   // //   Key/Note events

   // void addNote(int channel, int midiPitch) {
   //    mNoteTable.noteOn(channel, midiPitch);
   // }

   // void removeNote(int channel) {
   //    mNoteTable.noteOff(channel);
   // }

   // void updateNote(int channel, int midiPitch) { 
   //    mNoteTable.setPitch(channel, midiPitch);
   // }

   // // Update the key state to reflect the events in the queue
   // // queue is returned unchanged.
   // void updateKeyState(List<LiveMidiEvent> & queue)
   // {
   //    // may need to change this to handle immediate mode ??
   //    // for each member in the queue
   //    //   update key state(pEvent) 
   //    //   if mode == immediate 
   //    //      if event is note off 
   //    //         if keyState.isOff(noteNumber, channelNumber) // not being held, etc 
   //    //            streamWriter->sendImmediateNoteOff(event->noteNUmber, eventChannelNumber, frameOffset)
   //    // endfor 

   //    // -- or --
   //    // keyState.noteOffPendingQueue
   //    // on key update
   //    //   for each member of the queue
   //    //      update key state 
   //    //      if note is not active && mode = IMMEDIATE 
   //    //          cancelPendingQueue.append( New noteEvent(channel, note, .. ))
   //    //   end
   //    //
   //    // on arpPlayer.step()
   //    //    if mode == immediate 
   //    //       for each event in the cancel pending list
   //    //         noteStream.forceNoteOff(channel, note, frameOffset)
   //    //    clearCancelPendingList()
   //    //    advance() 
   //    //
   //    // So ArpPlayer needs:
   //    //   SyncMode (Immediate, NotImmediate)
   //    //   CancelPendingQueue<ArpNote>()
   //    //   FreePool<ArpNote>

   //    mMidiPitchPool.updateKeyState(queue);

   //    if (mMidiPitchPool.isEmpty())
   //    {
   //       if (isResetOnKeysEmpty())
   //       {
   //          resetScanToBeginning();
   //          resetToInitialContext();
   //       }
   //    }
   // }

   // void clearKeysPressed()
   // {
   //    mMidiPitchPool.clear();
   // }

   // int getNumKeyPresses()
   // {
   //    return mMidiPitchPool.getNumNotes();
   // }

   // PtrArray<KeyPress> & getKeysPressedArrivalOrder()
   // {
   //    return mMidiPitchPool.getArrivalOrder();
   // }
   
   // PtrArray<KeyPress> & getKeysPressedLowToHighOrder()
   // {
   //    return mMidiPitchPool.getLowToHighOrder();
   // }

   void setKeysOffStrategy(KeysOffStrategy strategy)
   {
      mKeysOffStrategy = strategy;
   }

   KeysOffStrategy getKeysOffStrategy() const 
   {
      return mKeysOffStrategy;
   }

   bool isResetOnKeysEmpty() const
   {
      return mKeysOffStrategy == KeysOffStrategy::KEYS_OFF_RESET_CONTEXT; 
   }

#ifdef INCLUDE_DEBUG_FUNCTIONS
   // // for debug and unit test 
   // void setNoteOn(MidiMessage & midiMessage)
   // {
   //    LiveMidiEvent liveMidiEvent; 
   //    liveMidiEvent.mFrameOffset = 0;
   //    liveMidiEvent.mMidiMessage = midiMessage;
   //    mMidiPitchPool.addNoteOn(&liveMidiEvent);
   // }

   // void setNoteOff(MidiMessage & midiMessage)
   // {
   //    LiveMidiEvent liveMidiEvent; 
   //    liveMidiEvent.mFrameOffset = 0;
   //    liveMidiEvent.mMidiMessage = midiMessage;
   //    mMidiPitchPool.addNoteOff(&liveMidiEvent);
   // }
#endif // INCLUDE_DEBUG_FUNCTIONS

   ExpressionContext & getExpressionContext()
   {
      return mExpressionContext; // context for expression evaluation
   }

   void reset()
   {
      resetToInitialContext();
   }

   // ==============================================================
   //   LSystemExecutor callbacks 

   virtual void onEndOfPattern() override;
   virtual void onNewSequence() override;
   virtual void perform(ExpandedTerm * pTerm) override;

private:
   NoteTable                      mNoteTable;  // active input notes 
   // KeyState             mKeyState;  // active key presses 
   //MidiPitchPool<MAX_POLYPHONY>   mMidiPitchPool; // active input pitches (midi notes)
   SortingOrder                   mNoteSortOrder;  // order to traverse notes: (as-played, low-to-high)

   // Context stack
   // Holds context that is pushed/popped for [ and ] markers
   Stack<ArpContext>    mContextStack; 
   FreePool<ArpContext> mFreePoolArpContext;

   ArpContext * getCurrentContext() const
   {
      return mContextStack.top();
   }

   ArpContext * getPlayableContext();

   void drainContextStack();
   void pushInitialContext();

   // Expression evaluation context
   // variables, etc
   ExpressionContext     mExpressionContext;

   // Execution State 
   // can be disabled after exression evaluation to implement
   // a kind of flow/branch control logic.
   BooleanStack         mExecutionStateStack; 
   bool                 mIsExecuting; // true if condition result enables execution, false to skip to end of "branch"
   bool isExecutionEnabled()    const { return (mIsExecuting == true);  }
   bool isExecutionSuppressed() const { return (mIsExecuting == false); }

   // // Velocity 
   // VelocityManager       mVelocityManager; 
   // VelocityMode          mVelocityMode; 
   // int                   mNoteRelativeVelocity; // offset to natural note velocity increment

   // Base Note 
   bool          mNoteEnabled; // process note normally (increment all note/octave indexes etc.) but suppress note emit if not enabled
   int           mNoteDelaySamples;
   int           mNoteChannel; // 0..16, 0 = asReceived

   NoteRange     mArpNoteRange;     // note range for arpeggiated input notes
   NoteRange     mHarmonyNoteRange; // note range for generated harmony notes 

   // Arp
   Stepper       mArpStepper;

   // Semitone 
   Stepper       mSemitoneIncrementStepper;

   // Harmonies 
   Stepper       mHarmonyIntervalSteppers[ ARP_PLAYER_NUM_HARMONIES ];
   bool          mHarmonyEnabled[  ARP_PLAYER_NUM_HARMONIES ];
   // int           mHarmonyChannel[ ARP_PLAYER_NUM_HARMONIES ]; // 0..16, 0 = asReceived
//   float        mHarmonyVelocityMultiplier[ ARP_PLAYER_NUM_HARMONIES ];
//   int           mHarmonyRelativeVelocity;   // harmony velocity increment = base note increment + harmony relative offset 
//   int           mHarmonyDelaySamples[ ARP_PLAYER_NUM_HARMONIES ];

   // // Gliss (strum) 
   // Bounder<float>  mGlissSpeedBounder;

   // Musical Scale, along with snap-to-scale options 
   TwelveToneScale  mTwelveToneScale;

   int mNoteStride;       // the number of notes to move on each increment/decrement

   int mArpOctaveOffsetMin;   // 1..N
   int mArpOctaveOffsetMax;   // 1..N

   int mHarmonyOctaveOffsetMin;
   int mHarmonyOctaveOffsetMax;

//   int mHarmonyOctaveTranspose;

   bool mOctaveIncludesPerfect; // include 'perfect' octave in octave range

   // int mNominalNoteLengthSamples; 

//   BoundaryStrategy   mOverstepAction; // action to take when note index exceeds available notes and octaves 
   bool               mIsReflecting;   // true to invert sense of up/down
   void toggleIncrementDirection() { mIsReflecting = (! mIsReflecting); }
//   bool isIncrementUpward() const { return mIsReflecting == false; }
//   bool isIncrementDownward() const { return mIsReflecting; }

   ArpNoteWriter * mpArpNoteWriter;  // note output stream 
   // int mStaggerSamples; // number of samples between notes when strumming/glissing

   // Strategy for reacting to empty key state (no key presses)
   KeysOffStrategy  mKeysOffStrategy;

   // Randomness
   float mNoteProbability;     // 0..1
   float mHarmonyProbability;  // 0..1

   bool isNoteProbable()
   {
      return (mNoteProbability >= mExpressionContext.getRandomZeroOne());
   }

   bool isHarmonyProbable()
   {
      return (mHarmonyProbability >= mExpressionContext.getRandomZeroOne());
   }

//   void increaseNoteIndex();
//   void decreaseNoteIndex();
//   void setToLowestNote(int & noteIndex, int & octaveOffset);
//   void setToLowestNotePlusOne(int & noteIndex, int & octaveOffset, int numNotes);
//   void setToHighestNote(int & noteIndex, int & octaveOffset, int numNotes);
//   void setToHighestNoteMinusOne(int & noteIndex, int & octaveOffset, int numNotes);

   //void sendNote(int channelNumber, int noteNumber, int velocityValue, int noteLengthSamples, int delaySamples);
   void sendNote(int channelNumber, int noteNumber, int velocityValue);
   // void sendAllNotes(int staggerSamples, bool isDirectionUpward = true);

   void updateIntervalSteppers();
   void updateArpStepper();

   // KeyPress * getKeyPressAtIndex(int noteIndex)
   // {
   //    if (isPlayOrderAsReceived())
   //    {
   //       return mMidiPitchPool.getArrivalOrder()[ noteIndex ];
   //    }
   //    else // low to high 
   //    {
   //       return mMidiPitchPool.getLowToHighOrder()[ noteIndex ];
   //    }
   // }

   void resetToInitialContext();


   // Action implementor methods
   void doActionTerm(ExpandedTerm * pTerm);
   void doNoteUp();
   void doNoteDown();
   void doNoteRandom();
   void doNoteNominal();
   void doNoteAssign(ExpandedTerm * pTerm);
   void doPlayNote();
   void doPlayRest();
   // void doPlayChord();
   // void setStaggerSamples(int staggerSamples);
   // void doPlayGliss();
   // void doGlissSpeedUp();
   // void doGlissSpeedDown();
   // void doGlissSpeedRandom();
   // void doGlissSpeedNominal();
   // void doGlissSpeedAssign(ExpandedTerm * pTerm);
   // void doGlissDirectionUp();
   // void doGlissDirectionDown();
   // void doGlissDirectionRandom();
   // void doGlissDirectionNominal();
   // void doGlissDirectionAssign(ExpandedTerm * pTerm);
   // void doVelocityUp();
   // void doVelocityDown();
   // void doVelocityRandom();
   // void doVelocityNominal();
   // void doVelocityAssign(ExpandedTerm * pTerm);
   void doSemitoneUp();
   void doSemitoneDown();
   void doSemitoneRandom();
   void doSemitoneNominal();
   void doSemitoneAssign(ExpandedTerm * pTerm);
   void doOctaveUp();
   void doOctaveDown();
   void doOctaveRandom();
   void doOctaveNominal();
   void doOctaveAssign(ExpandedTerm * pTerm);
   void doHarmonyUp(int harmonyId);
   void doHarmonyDown(int harmonyId);
   void doHarmonyRandom(int harmonyId);
   void doHarmonyNominal(int harmonyId);
   void doHarmonyAssign(int harmonyId, ExpandedTerm * pTerm);
   void doUpDownInvert();
   void doUpDownNominal();
   void doContextPush();
   void doContextPop();
   //void doGroupBegin();
   //void doGroupEnd();
   void doIf();
   void doElse();
   void doExpressionBegin(ExpandedTerm * pTerm);
   void doExpressionEnd(ExpandedTerm * pTerm);
   void doFunction(ExpandedTerm * pTerm);
   void doProbability(ExpandedTerm * pTerm);
   void doCondition(ExpandedTerm * pTerm);

   float evaluateExpression(ExpandedTerm * pTerm);

   // Search backwards to locate the EXPRESSION_BEGIN term that matches the 
   // given EXPRESSION_END term 
   ExpandedTerm * findExpressionBeginTerm(ExpandedTerm * pExpressionEndTerm);
   void pushExecutionState(ExpandedTerm * pTerm); 
   void popExecutionState(); 
   void toggleExecutionState(); 

   int computeChannelNumber(int channelNumber, int configuredChannelNumber);
   // int computeVelocity(ArpContext * pContext, KeyPress * pKeyPress);
   // int computeHarmonyVelocity(int harmonyIdx, ArpContext * pContext, KeyPress * pKeyPress);
   // int clampVelocityIncrement(ArpContext * pContext);

   int clampHarmonyNote(int harmonyNote, int baseNote);


   // ODO: delete commented code 
   // BoundaryStrategy convertLSystemBoundsStrategytoBoundaryStrategy(LSystemBoundsType strategy)
   // {
   //    // TODO make this a table lookup .. 
   //    switch (strategy)
   //    {
   //    case BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_UP:
   //       return BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_UP;
   //    case LSystemBoundsType::BOUNDS_CYCLE_DOWN:
   //       return BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_DOWN;
   //    case LSystemBoundsType::BOUNDS_REFLECT:
   //       return BoundaryStrategy::BOUNDARY_STRATEGY_REFLECT;
   //    case LSystemBoundsType::BOUNDS_REFLECT_LINGER:
   //       return BoundaryStrategy::BOUNDARY_STRATEGY_REFLECT_LINGER;
   //    case LSystemBoundsType::BOUNDS_FLIP:
   //       return BoundaryStrategy::BOUNDARY_STRATEGY_FLIP;
   //    // case LSystemBoundsType::BOUNDS_STAY_AT_EDGE:
   //    //    return BoundaryStrategy::BOUNDARY_STRATEGY_STAY_AT_EDGE;
   //    case LSystemBoundsType::BOUNDS_RESET_TO_NOMINAL:
   //       return BoundaryStrategy::BOUNDARY_STRATEGY_RESET_TO_NOMINAL;
   //    case LSystemBoundsType::BOUNDS_DISCARD:
   //       return BoundaryStrategy::BOUNDARY_STRATEGY_DISCARD_OVERSHOOT;
   //    default:
   //       DEBUG( ">> Unexpected value for LSystem Bounds Strategy: %d", strategy );
   //       return BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_UP;
   //    }
   // }

};

#endif // __arp_player_h_
