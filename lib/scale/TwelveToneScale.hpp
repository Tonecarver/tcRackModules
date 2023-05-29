#pragma once
#ifndef _tc_twelve_tone_scale_h_
#define _tc_twelve_tone_scale_h_  1

#include "Scales.hpp"
#include "../datastruct/Range.hpp"

#include <string>
#include <cstring> // memset 

struct NoteRange : public Range<int>
{
   NoteRange(int noteMin, int noteMax) : Range(noteMin, noteMax) 
   {
   }

   int snapToRange(int noteIn) const {  // TODO: not used yet 
      int noteOut = noteIn;
      while (noteOut < getMinValue()) {
         noteOut += 12;  // TODO: const 
      }
      while (noteOut > getMaxValue()) {
         noteOut -= 12;  // TODO: const
      }
     return (includes(noteOut) ? noteOut : -1);
   }
};

enum ScaleOutlierStrategy
{
   OUTLIER_SNAP_TO_SCALE, 
   OUTLIER_PASS_THRU,  // allow all notes
   OUTLIER_DISCARD, 
   //
   kNUM_OUTLIER_STRATEGIES
};
extern const char * scaleOutlierStrategyNames[ScaleOutlierStrategy::kNUM_OUTLIER_STRATEGIES];


enum TwelveToneNotes { 
   NOTE_C = 0, 
   NOTE_C_SHARP, 
   NOTE_D,
   NOTE_D_SHARP, 
   NOTE_E,
   NOTE_F,
   NOTE_F_SHARP, 
   NOTE_G,
   NOTE_G_SHARP, 
   NOTE_A,
   NOTE_A_SHARP, 
   NOTE_B,
   //
   kNUM_TWELVE_TONE_NOTES
}; 


class TwelveToneScale
{
public:
    enum { NUM_DEGREES_PER_SCALE = 12 };

   TwelveToneScale(ScaleDefinition * pScaleDefinition = 0)
      : mTonicPitch(0) // TODO: const = C
      , mpScaleDefinition(pScaleDefinition)
      , mOutlierStrategy(ScaleOutlierStrategy::OUTLIER_SNAP_TO_SCALE)
   {
      buildTables();
   }

   void setOutlierStrategy(ScaleOutlierStrategy strategy)
   {
      mOutlierStrategy = strategy;
      buildTables();
   }

   ScaleOutlierStrategy getOutlierStrategy() const
   {
      return mOutlierStrategy;
   }

   bool isSnapToScale() const
   {
      return mOutlierStrategy == ScaleOutlierStrategy::OUTLIER_SNAP_TO_SCALE;
   }

   bool isPassThru() const
   {
      return mOutlierStrategy == ScaleOutlierStrategy::OUTLIER_PASS_THRU;
   }

   bool isDiscard() const
   {
      return mOutlierStrategy == ScaleOutlierStrategy::OUTLIER_DISCARD;
   }

   int getTonicPitch() const
   {
      return mTonicPitch;
   }

   void setTonicPitch(int pitch)
   {
      mTonicPitch = pitch;
      buildTables();
   }

   int getNumNotes() const
   {
      return (mpScaleDefinition != 0 
         ? (mOutlierStrategy == ScaleOutlierStrategy::OUTLIER_PASS_THRU ? 12 : mpScaleDefinition->numDegrees) 
         : 0 );
   }

   void setScale(const ScaleDefinition * pScaleDefinition)
   {
      // assert != 0
      mpScaleDefinition = pScaleDefinition;
      buildTables();
   }

   bool isConfigured() const
   {
      return (mpScaleDefinition != 0) && (mpScaleDefinition->getNumDegrees() >0);
   }

   // return -1 if not mapped 
   int snapToScale(int midiNoteNumber) const
   {
      return getNoteAtInterval(midiNoteNumber, 0);
   }


   // musical interval is scale-based
   //  0 = unison 
   //  1 = unison
   //  2 = second 
   //  3 = 3rd
   // -3 = third down 
   //
   //  major/minor size of intervals (major 3rd vs minor 3rd) is based on the scale 
   //  and relative to the starting note. 
  int getNoteAtInterval(int midiNoteNumber, int musicalInterval) const; // TODO: remove 

   // // return -1 if not mapped 
   // int applyInterval(int midiNoteNumber, int musicalInterval, const NoteRange & noteRange) const; 

   int numDegrees() const
   {
      return (mpScaleDefinition != 0 ? mpScaleDefinition->getNumDegrees() : 0);
   }


   // degree is 0..11
   bool isPitchEnabledRelativeToC(int pitchZeroToEleven) const {
      return mPitchEnabled[pitchZeroToEleven];
   }

   inline static int midiPitchToDegree(int midiPitch) {
      return midiPitch % 12;
   }

   inline static int midiPitchToOctave(int midiPitch) {
      return midiPitch / 12;
   }

   inline static const char * noteName(int midiPitch) {
      return mNoteNames[midiPitch % NUM_DEGREES_PER_SCALE];
   }

private:
   int mTonicPitch;
   const ScaleDefinition * mpScaleDefinition;
   ScaleOutlierStrategy  mOutlierStrategy;

   bool mPitchEnabled[NUM_DEGREES_PER_SCALE]; // pitchEnabled[ midiNote % 12  ] = true/false
   int mSnap[NUM_DEGREES_PER_SCALE];          // outNote = inNote + mSnap[ inNote ];

   struct Interval {
      int nextDegree;
      int delta;
   }; 

   Interval mIntervalUp[NUM_DEGREES_PER_SCALE];    // index of next active scale note upwards 
   Interval mIntervalDown[NUM_DEGREES_PER_SCALE];  // index of next active scale note downwards

   // index[ 0 ] == C 
   static const char * mNoteNames[NUM_DEGREES_PER_SCALE];

   bool isMember(int idx) const
   {
      return mPitchEnabled[idx];
   }

   bool isOutlier(int idx) const
   {
      return mPitchEnabled[idx] == false;
   }

   void buildTables();
   int findNearestMember(int start) const;
   int findIntervalUp(int start) const;
   int findIntervalDown(int start) const;


#ifdef INCLUDE_DEBUG_FUNCTIONS
   void showTables(char * hint = 0);
#endif // INCLUDE_DEBUG_FUNCTIONS
}; 

#endif // _tc_twelve_tone_scale_h_
