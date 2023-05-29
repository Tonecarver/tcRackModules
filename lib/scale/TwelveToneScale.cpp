#include "plugin.hpp"
#include "TwelveToneScale.hpp"

// ===============================================

// order must match enum ScaleOutlierStrategy
const char * scaleOutlierStrategyNames[ScaleOutlierStrategy::kNUM_OUTLIER_STRATEGIES] =
{
   "Snap to Scale", // OUTLIER_SNAP_TO_SCALE, 
   "Pass Thru",     // OUTLIER_PASS_THRU,  // allow all notes
   "Discard",       // OUTLIER_DISCARD, 
};

// order must match C4 as midi 60 with offset degree 0 
const char * TwelveToneScale::mNoteNames[TwelveToneScale::NUM_DEGREES_PER_SCALE] =
{
   "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", 
};


int TwelveToneScale::getNoteAtInterval(int midiNoteNumber, int musicalInterval) const
{
   // DEBUG("TwelveTone: getNoteAtInterval, noteNumber %d, interval %d", midiNoteNumber, musicalInterval);
   if (getNumNotes() == 0)
   {
    // DEBUG("TwelveTone: getNoteAtInterval, EMPTY");
      return -1; 
   }

   int degree = midiNoteNumber % 12; 

   if (isDiscard())
   {
      if (isOutlier(degree))
      {
         // DEBUG("TwelveTone: getNoteAtInterval, OUTLIER");
         return -1;
      }
   }

 // DEBUG("TwelveTone: getNoteAtInterval, midiNote %d, interval %d, degree %d, snap[%d] = %d, snappedNote = %d", midiNoteNumber, musicalInterval, degree, degree, mSnap[degree], (midiNoteNumber + mSnap[degree]));

   if (musicalInterval == 0)
   {
    // DEBUG("Final note out after snapping %d", midiNoteNumber + mSnap[ degree ]);
      return midiNoteNumber + mSnap[ degree ]; // snap to scale
   }
   else if (musicalInterval > 0)
   {

// TODO: delete commented code 
      // /**
      //  * TODO: optimize, make a lookup table for this instead of searching 
      //  * every time the method is called. At least a table with indexes to 
      //  * the next active note above and below a given note .. kind of like
      //  * the snap table but used to jump 1 interval up or down. 
      // */
      // int semitoneOffset = 0; 
      // int idx = degree;
      // for (int i = 1; i < musicalInterval; i++) // starts at 1 for unison
      // {
      //    // increment up the scale to find the next interval
      //    for(;;)
      //    {
      //       semitoneOffset++;
      //       idx++;
      //       if (idx >= 12)
      //       {
      //          idx = 0;
      //       }
      //       if (isMember(idx))
      //       {
      //          break; // exit the inner loop
      //       }
      //    }
      // }
      // return midiNoteNumber + semitoneOffset;

      int noteOut = midiNoteNumber; 
      for (int i = 1; i < musicalInterval; i++) // starts at 1 for unison
      {
         // DEBUG("Searching interval up: degree %d, intervalUp[%d] = %d, %d", degree, degree, mIntervalUp[degree].nextDegree, mIntervalUp[degree].delta);
         noteOut += mIntervalUp[degree].delta;
         degree = mIntervalUp[degree].nextDegree;
      }
    // DEBUG("Final note out after intervalUp[] search = %d", noteOut);
      return noteOut;
   }
   else  //(musicalInterval < 0)
   {
// TODO: delete commented code 
      // int semitoneOffset = 0; 
      // int idx = degree;
      // musicalInterval = 0 - musicalInterval; // make positive for loop counter 
      // for (int i = 0; i < musicalInterval; i++)  // for negative intervals, start at 0 for unison
      // {
      //    // increment up the scale to find the next interval
      //    for(;;)
      //    {
      //       semitoneOffset--;
      //       idx--;
      //       if (idx < 0)
      //       {
      //          idx = 11;
      //       }
      //       if (isMember(idx))
      //       {
      //          break; // exit the inner loop 
      //       }
      //    }
      // }
      // DEBUG("Returning note %d, found semitone offset = %d", midiNoteNumber + semitoneOffset, semitoneOffset);
      //return midiNoteNumber + semitoneOffset;

      musicalInterval = 0 - musicalInterval; // make positive for loop counter
      int noteOut = midiNoteNumber;
      for (int i = 0; i < musicalInterval; i++)  // for negative intervals, start at 0 for unison
      {
         //DEBUG("Searching interval down: degree %d, intervalDown[%d] = %d, %d", degree, degree, mIntervalDown[degree].nextDegree, mIntervalDown[degree].delta);
         noteOut += mIntervalDown[degree].delta;
         degree = mIntervalDown[degree].nextDegree;
      }
    // DEBUG("Final note out after intervalDown[] search = %d", noteOut);
      return noteOut;
   }
}

// int TwelveToneScale::applyInterval(int midiNoteNumber, int musicalInterval, const NoteRange & noteRange) const 
// {
//    DEBUG("TwelveTone: getNoteAtInterval, noteNumber %d, interval %d, note range %d .. %d", midiNoteNumber, musicalInterval, noteRange.getMinValue(), noteRange.getMaxValue());
//    if (getNumNotes() == 0)
//    {
//       DEBUG("TwelveTone: getNoteAtInterval, EMPTY");
//       return -1; 
//    }

//    int degree = midiNoteNumber % 12; 

//    if (isDiscard())
//    {
//       if (isOutlier(degree))
//       {
//          DEBUG("TwelveTone: getNoteAtInterval, OUTLIER");
//          return -1;
//       }

//       if (! noteRange.includes(midiNoteNumber))
//       {
//          DEBUG("TwelveTone: getNoteAtInterval, OUT OF RANGE");
//          return -1; // out of range
//       }
//    }

//    DEBUG("TwelveTone: getNoteAtInterval, midiNote %d, interval %d, degree %d, snap[%d] = %d, snappedNote = %d", midiNoteNumber, musicalInterval, degree, degree, mSnap[degree], (midiNoteNumber + mSnap[degree]));

//    int noteOut = midiNoteNumber;

// // TODO: if pass all, reject note if outside range ?? 

//    if (isSnapToScale()) {
//       int snappedNote  = midiNoteNumber + mSnap[degree];   // TODO: how to detect an octave wrap here ?? 
//       if (noteRange.includes(snappedNote)) {
//          noteOut = snappedNote;
//       } else if (noteRange.includes(snappedNote + 12)) {
//          noteOut = snappedNote + 12;
//       } else if (noteRange.includes(snappedNote - 12)) {
//          noteOut = snappedNote - 12;
//       }
//    }

//    if (musicalInterval == 0)
//    {
//       DEBUG("Final note out after snapping %d", noteOut);
//       return noteRange.clamp(noteOut);
//    }
//    else if (musicalInterval > 0)
//    {
//       for (int i = 1; i < musicalInterval; i++) // starts at 1 for unison
//       {
//          // DEBUG("Searching interval up: degree %d, intervalUp[%d] = %d, %d", degree, degree, mIntervalUp[degree].nextDegree, mIntervalUp[degree].delta);
//          int noteUp = noteOut + mIntervalUp[degree].delta;
//          if (! noteRange.includes(noteUp)) {
//             break; // stop when next note up would exceed active range 
//          }
//          noteOut = noteUp;
//          degree = mIntervalUp[degree].nextDegree;
//       }
//       DEBUG("Final note out after intervalUp[] search = %d", noteOut);
//    }
//    else  //(musicalInterval < 0)
//    {
//       musicalInterval = 0 - musicalInterval; // make positive for loop counter
//       for (int i = 0; i < musicalInterval; i++)  // for negative intervals, start at 0 for unison
//       {
//          //DEBUG("Searching interval down: degree %d, intervalDown[%d] = %d, %d", degree, degree, mIntervalDown[degree].nextDegree, mIntervalDown[degree].delta);
//          int noteDown = noteOut + mIntervalDown[degree].delta;
//          if (! noteRange.includes(noteDown)) {
//             break; // stop when next note down would exceed active range 
//          }
//          noteOut = noteDown;
//          degree = mIntervalDown[degree].nextDegree;
//       }
//       DEBUG("Final note out after intervalDown[] search = %d", noteOut);
//    }

//    DEBUG("Final note out before clamp = %d", noteOut);
//    noteOut = noteRange.clamp(noteOut);  // should not need this 
//    DEBUG("Final note out = %d", noteOut);
//    return noteOut;
// }

void TwelveToneScale::buildTables()
{
   // assert scale desc != 0

   /// memset(mDegrees, -1, sizeof(mDegrees));
   memset(mPitchEnabled, 0, sizeof(mPitchEnabled));
   memset(mSnap, 0, sizeof(mSnap));
   memset(mIntervalUp, 0, sizeof(mIntervalUp));
   memset(mIntervalDown, 0, sizeof(mIntervalDown));

   // check #notes <= 12 

   if (mpScaleDefinition != 0)
   {
      // pitches in ScaleDefinition are relative to C
      // adjust to align with active tonic degree 
      int tonicDegree = midiPitchToDegree(mTonicPitch);
      for (int i = 0; i < mpScaleDefinition->getNumDegrees(); i++) 
      {
         int degree =  mpScaleDefinition->getDegree(i);
         int semitones = (degree + tonicDegree) % 12;
         mPitchEnabled[ semitones ] = true;
      }

      if (isSnapToScale())
      {
         for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++) 
         {
            // if (mDegrees[i] < 0)
            if (mPitchEnabled[i] == false)
            {
               mSnap[i] = findNearestMember(i);
            }
         }
      }

      for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++) 
      {
         int degree; 

         degree = findIntervalUp(i);
         mIntervalUp[i].nextDegree = degree;
         mIntervalUp[i].delta = (degree > i) ? (degree - i) : (degree - i) + NUM_DEGREES_PER_SCALE;

         degree = findIntervalDown(i);
         mIntervalDown[i].nextDegree = degree;
         mIntervalDown[i].delta = (degree < i) ? (degree - i) : (degree - i) - NUM_DEGREES_PER_SCALE;
      }

      // for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++) 
      // {
      //    DEBUG("Scale: pitchEnabled[%2d] = %2d", i, mPitchEnabled[i]);
      // }
      // for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++) 
      // {
      //    DEBUG("Scale: IntervalUp[%2d] = %2d, %2d", i, mIntervalUp[i].nextDegree, mIntervalUp[i].delta);
      // }
      // for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++) 
      // {
      //    DEBUG("Scale: IntervalDown[%2d] = %2d, %d", i, mIntervalDown[i].nextDegree, mIntervalDown[i].delta);
      // }

   }

#ifdef INCLUDE_DEBUG_FUNCTIONS
   showTables();
#endif //INCLUDE_DEBUG_FUNCTIONS
}

int TwelveToneScale::findNearestMember(int start) const
{
   int upIndex = start +1; 
   int downIndex = start -1; 
   for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++)
   {
      if (upIndex >= NUM_DEGREES_PER_SCALE)
      {
         upIndex = 0;
      }
      if (downIndex < 0)
      {
         downIndex = NUM_DEGREES_PER_SCALE - 1;
      }
      if (isMember(upIndex) && isMember(downIndex)) 
      {
         int distance_up = (upIndex - start);
         int distance_down = (downIndex - start);
         return abs(distance_up) <= abs(distance_down) ? distance_up : distance_down;
      }
      if (isMember(upIndex))
      {
         return (upIndex - start);
      }
      if (isMember(downIndex))
      {
         return (downIndex - start);
      }
      upIndex++;
      downIndex--;
   }
   return -1; // not found
}

int TwelveToneScale::findIntervalUp(int start) const 
{
   int idx = start;
   for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++)
   {
         idx++;
         if (idx >= NUM_DEGREES_PER_SCALE) {
            idx = 0;
         }
         if (isMember(idx)) {
            return idx; 
         }
   }
   return start; // not found
}

int TwelveToneScale::findIntervalDown(int start) const 
{
   int idx = start;
   for (int i = 0; i < NUM_DEGREES_PER_SCALE; i++)
   {
         idx--;
         if (idx < 0) {
            idx = (NUM_DEGREES_PER_SCALE - 1);
         }
         if (isMember(idx)) {
            return idx; 
         }
   }
   return start; // not found   
}

#ifdef INCLUDE_DEBUG_FUNCTIONS

void TwelveToneScale::showTables(char * hint)
{
   DEBUG( "TwelveToneScale: %s", hint != 0 ? hint : "" );
   DEBUG("  Tonic %d = %s %d", mTonicPitch, twelveToneNoteNames[ midiPitchToDegree(mTonicPitch) ], midiPitchToOctave(mTonicPitch) );
   DEBUG("  Strategy = %s", scaleOutlierStrategyNames[ int(mOutlierStrategy) ] );

   if (mpScaleDefinition == 0)
   {
      DEBUG("  Scale = 0 (null, not defined)" );
   }
   else
   {
      DEBUG("  Scale = %s, numNotes = %d", 
         mpScaleDefinition->name.c_str(),
         mpScaleDefinition->getNumDegrees() );
   }

   DEBUG( "  degrees[] = { %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }", 
      mPitchEnabled[0],
      mPitchEnabled[1],
      mPitchEnabled[2],
      mPitchEnabled[3],
      mPitchEnabled[4],
      mPitchEnabled[5],
      mPitchEnabled[6],
      mPitchEnabled[7],
      mPitchEnabled[8],
      mPitchEnabled[9],
      mPitchEnabled[10],
      mPitchEnabled[11] );

   DEBUG( "  snap[] = { %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }", 
      mSnap[0],
      mSnap[1],
      mSnap[2],
      mSnap[3],
      mSnap[4],
      mSnap[5],
      mSnap[6],
      mSnap[7],
      mSnap[8],
      mSnap[9],
      mSnap[10],
      mSnap[11] );


   for (int i = 0; i < 12; i++)
   {
      if (isMember(i))
      {
         // int noteId = (mTonicPitch + mDegrees[i]) % 12;
         // DEBUG( "  Scale member: octave offset %d, degree %d, %s", i, mDegrees[i], twelveToneNoteNames[ noteId ] );
         // int noteId = (mTonicPitch + i) % 12;
         int noteId = i;
         DEBUG( "  Scale member: degree %d, %s", i, twelveToneNoteNames[ noteId ] );
      }
   }
}
#endif // INCLUDE_DEBUG_FUNCTIONS



