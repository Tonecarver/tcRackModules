#include "rack.hpp"
#include "ArpPlayer.hpp"
#include "ArpTerm.hpp"
// #include "../../lib/midi/MidiConstants.h"

ArpPlayer::ArpPlayer()
  // : mMidiPitchPool()
   : mNoteTable()
   , mNoteSortOrder(SortingOrder::SORT_ORDER_LOW_TO_HIGH)
   , mExecutionStateStack(1024) // large initial capacity to make growth (memory allocation) unlikely
   , mIsExecuting(true)
   , mNoteEnabled(true)
   , mNoteDelaySamples(0)
   , mNoteChannel(0)
   , mArpNoteRange(0,120) // TODO: const
   , mHarmonyNoteRange(0, 120) // TODO const  
   //   , mHarmonyRelativeVelocity(0)
   , mNoteStride(1)
   , mArpOctaveOffsetMin(0) 
   , mArpOctaveOffsetMax(0) 
   , mHarmonyOctaveOffsetMin(0)
   , mHarmonyOctaveOffsetMax(0)
//    , mHarmonyOctaveTranspose(0)    // TODO: delete this 
   , mOctaveIncludesPerfect(false)
//   , mArpOctaveOffset(0.f)
   // , mNominalNoteLengthSamples(22050)
   , mIsReflecting(false)
   , mpArpNoteWriter(0)
//   , mStaggerSamples(0)
//   , mVelocityMode(VelocityMode::VELOCITY_MODE_PER_PATTERN)
//   , mNoteRelativeVelocity(0)
   , mNoteProbability(1.0)
   , mHarmonyProbability(1.0)
//   , mKeysOffStrategy(KeysOffStrategy::KEYS_OFF_MAINTAIN_CONTEXT)
{
   pushInitialContext();

   // ArpStepper
   mArpStepper.setAlgorithm(StepperAlgo::STEPPER_ALGO_CYCLE_UP);
   mArpStepper.setMinMaxValues(-24, 24);
   mArpStepper.setNominalValue(0);
   mArpStepper.setStepSize(1);

   //Experiment // TODO: keep semitone ? 
   mSemitoneIncrementStepper.setAlgorithm(StepperAlgo::STEPPER_ALGO_CYCLE_UP);
   mSemitoneIncrementStepper.setMinMaxValues(-24, 24);
   mSemitoneIncrementStepper.setNominalValue(0);
   mSemitoneIncrementStepper.setStepSize(1);

   for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
   {
      mHarmonyIntervalSteppers[i].setAlgorithm(StepperAlgo::STEPPER_ALGO_CYCLE_UP);
      mHarmonyIntervalSteppers[i].setMinMaxValues(-24, 24);
      mHarmonyIntervalSteppers[i].setNominalValue(0);
      mHarmonyIntervalSteppers[i].setStepSize(1);
      mHarmonyEnabled[i] = true;
      //mHarmonyVelocityMultiplier[i] = 1.0;
      // mHarmonyChannel[i] = 0; // 0 = asReceived, 1..16 = channel // is this used ?
      // mHarmonyDelaySamples[i] = 0;
   }

   // mGlissSpeedBounder.setBoundaryStrategy(BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_UP);
   // mGlissSpeedBounder.setDirection(BoundaryDirection::BOUNDARY_DIRECTION_INCREASE);
   // mGlissSpeedBounder.setMinValue(0.0);
   // mGlissSpeedBounder.setMaxValue(1.0);
   // mGlissSpeedBounder.setNominalValue(0.25);
   // mGlissSpeedBounder.setStepSize(0.1);
}

void ArpPlayer::resetToInitialContext()
{
   mExecutionStateStack.clear();
   mIsExecuting = true;
   mExecutionStateStack.push(true); 

   mIsReflecting = false;

   drainContextStack(); 
   pushInitialContext();
}

void ArpPlayer::onEndOfPattern()
{
   // DEBUG( "Arp Executor: on END of PATTERN" );

   // options: 
   //    toggle play direction
   //    ??
}

void ArpPlayer::onNewSequence()
{
   // Do not clear the key state here 
   // Let existing key presses remain across pattern/rule loads 
   // /*mMidiPitchPool.clear();*/ 

   mIsReflecting = false;

   mExecutionStateStack.clear();
   mIsExecuting = true;
   mExecutionStateStack.push(true); // set enclosing execution context

   drainContextStack(); 
   pushInitialContext();
}

void ArpPlayer::perform(ExpandedTerm * pTerm)
{
   if (pTerm == 0)
   {
      DEBUG( " Term is null, perform a REST" );
      doPlayRest();
      return;
   }

   ArpTermType termType = (ArpTermType) pTerm->getTermType();

 // DEBUG( "Arp Executor: Term   %s   isExecuting = %d", arpTermTypeNames[ termType ], isExecutionEnabled() );

   // If term type can affect CONTROL FLOW then execute it always
   // Otherwise execute only when execution is enabled 
   if (termType == ArpTermType::TERM_EXPRESSION_BEGIN)
   {
      doExpressionBegin(pTerm);
   }
   else if (termType == ArpTermType::TERM_EXPRESSION_END)
   {
      doExpressionEnd(pTerm);
   }
   else if (termType == ArpTermType::TERM_IF)
   {
      doIf();
   }
   else if (termType == ArpTermType::TERM_ELSE)
   {
      doElse();
   }
   else if (isExecutionEnabled())
   {
      doActionTerm(pTerm);

      if (pTerm->isStepTerminator())
      {
         setStepComplete(true);
      }
   }
}

void ArpPlayer::doActionTerm(ExpandedTerm * pTerm)
{
   assert(pTerm != 0);

   switch (pTerm->getTermType())
   {
   case ArpTermType::TERM_NOTE:
      doPlayNote();
      break;
   case ArpTermType::TERM_REST:
      doPlayRest();
      break;
   // case ArpTermType::TERM_CHORD:
   //    doPlayChord();
   //    break;

   // case ArpTermType::TERM_GLISS:
   //    doPlayGliss();
   //    break;

   // case ArpTermType::TERM_GLISS_SPEED_UP:
   //    doGlissSpeedUp();
   //    break;
   // case ArpTermType::TERM_GLISS_SPEED_DOWN:
   //    doGlissSpeedDown();
   //    break;
   // case ArpTermType::TERM_GLISS_SPEED_RANDOM:
   //    doGlissSpeedRandom();
   //    break;
   // case ArpTermType::TERM_GLISS_SPEED_NOMINAL:
   //    doGlissSpeedNominal();
   //    break;
   // case ArpTermType::TERM_GLISS_SPEED_ASSIGN:
   //    doGlissSpeedAssign(pTerm);
   //    break;

   // case ArpTermType::TERM_GLISS_DIR_UP:
   //    doGlissDirectionUp();
   //    break;
   // case ArpTermType::TERM_GLISS_DIR_DOWN:
   //    doGlissDirectionDown();
   //    break;
   // case ArpTermType::TERM_GLISS_DIR_RANDOM:
   //    doGlissDirectionRandom();
   //    break;
   // case ArpTermType::TERM_GLISS_DIR_NOMINAL:
   //    doGlissDirectionNominal();
   //    break;
   // case ArpTermType::TERM_GLISS_DIR_ASSIGN:
   //    doGlissDirectionAssign(pTerm);
   //    break;

   case ArpTermType::TERM_NOTE_UP:
      doNoteUp();
      break;
   case ArpTermType::TERM_NOTE_DOWN:
      doNoteDown();
      break;
   case ArpTermType::TERM_NOTE_RANDOM:
      doNoteRandom();
      break;
   case ArpTermType::TERM_NOTE_NOMINAL:
      doNoteNominal();
      break;
   case ArpTermType::TERM_NOTE_ASSIGN:
      doNoteAssign(pTerm);
      break;

   // case ArpTermType::TERM_VELOCITY_UP:
   //    doVelocityUp();
   //    break;
   // case ArpTermType::TERM_VELOCITY_DOWN:
   //    doVelocityDown();
   //    break;
   // case ArpTermType::TERM_VELOCITY_RANDOM:
   //    doVelocityRandom();
   //    break;
   // case ArpTermType::TERM_VELOCITY_NOMINAL:
   //    doVelocityNominal();
   //    break;
   // case ArpTermType::TERM_VELOCITY_ASSIGN:
   //    doVelocityAssign(pTerm);
   //    break;

   case ArpTermType::TERM_SEMITONE_UP:
      doSemitoneUp();
      break;
   case ArpTermType::TERM_SEMITONE_DOWN:
      doSemitoneDown();
      break;
   case ArpTermType::TERM_SEMITONE_RANDOM:
      doSemitoneRandom();
      break;
   case ArpTermType::TERM_SEMITONE_NOMINAL:
      doSemitoneNominal();
      break;
   case ArpTermType::TERM_SEMITONE_ASSIGN:
      doSemitoneAssign(pTerm);
      break;

   case ArpTermType::TERM_OCTAVE_UP:
      doOctaveUp();
      break;
   case ArpTermType::TERM_OCTAVE_DOWN:
      doOctaveDown();
      break;
   case ArpTermType::TERM_OCTAVE_RANDOM:
      doOctaveRandom();
      break;
   case ArpTermType::TERM_OCTAVE_NOMINAL:
      doOctaveNominal();
      break;
   case ArpTermType::TERM_OCTAVE_ASSIGN:
      doOctaveAssign(pTerm);
      break;

#if ARP_PLAYER_NUM_HARMONIES >=1
   case ArpTermType::TERM_HARMONY_1_UP:
      doHarmonyUp(0);
      break;
   case ArpTermType::TERM_HARMONY_1_DOWN:
      doHarmonyDown(0);
      break;
   case ArpTermType::TERM_HARMONY_1_RANDOM:
      doHarmonyRandom(0);
      break;
   case ArpTermType::TERM_HARMONY_1_NOMINAL:
      doHarmonyNominal(0);
      break;
   case ArpTermType::TERM_HARMONY_1_ASSIGN:
      doHarmonyAssign(0,pTerm);
      break;
#endif 

#if ARP_PLAYER_NUM_HARMONIES >=2
   case ArpTermType::TERM_HARMONY_2_UP:
      doHarmonyUp(1);
      break;
   case ArpTermType::TERM_HARMONY_2_DOWN:
      doHarmonyDown(1);
      break;
   case ArpTermType::TERM_HARMONY_2_RANDOM:
      doHarmonyRandom(1);
      break;
   case ArpTermType::TERM_HARMONY_2_NOMINAL:
      doHarmonyNominal(1);
      break;
   case ArpTermType::TERM_HARMONY_2_ASSIGN:
      doHarmonyAssign(1,pTerm);
      break;
#endif 

#if ARP_PLAYER_NUM_HARMONIES >=3
   case ArpTermType::TERM_HARMONY_3_UP:
      doHarmonyUp(2);
      break;
   case ArpTermType::TERM_HARMONY_3_DOWN:
      doHarmonyDown(2);
      break;
   case ArpTermType::TERM_HARMONY_3_RANDOM:
      doHarmonyRandom(2);
      break;
   case ArpTermType::TERM_HARMONY_3_NOMINAL:
      doHarmonyNominal(2);
      break;
   case ArpTermType::TERM_HARMONY_3_ASSIGN:
      doHarmonyAssign(2,pTerm);
      break;
#endif 

#if ARP_PLAYER_NUM_HARMONIES >=4
   case ArpTermType::TERM_HARMONY_4_UP:
      doHarmonyUp(3);
      break;
   case ArpTermType::TERM_HARMONY_4_DOWN:
      doHarmonyDown(3);
      break;
   case ArpTermType::TERM_HARMONY_4_RANDOM:
      doHarmonyRandom(3);
      break;
   case ArpTermType::TERM_HARMONY_4_NOMINAL:
      doHarmonyNominal(3);
      break;
   case ArpTermType::TERM_HARMONY_4_ASSIGN:
      doHarmonyAssign(3,pTerm);
      break;
#endif

   case ArpTermType::TERM_UP_DOWN_INVERT:
      doUpDownInvert();
      break;
   case ArpTermType::TERM_UP_DOWN_NOMINAL:
      doUpDownNominal();
      break;

   case ArpTermType::TERM_PUSH:
      if (isScanningForward())
      {
         doContextPush();
      }
      else
      {
         // DEBUG( " scanning reverse: convert PUSH to POP" );
         doContextPop();
      }
      break;
   case ArpTermType::TERM_POP:
      if (isScanningForward())
      {
         doContextPop();
      }
      else
      {
         doContextPush();
      }
      break;

   //case ArpTermType::TERM_GROUP_BEGIN:
   //   if (isScanningForward())
   //   {
   //      doGroupBegin();
   //   }
   //   else
   //   {
   //      doGroupEnd();
   //   }
   //   break;
   //case ArpTermType::TERM_GROUP_END:
   //   if (isScanningForward())
   //   {
   //      doGroupEnd();
   //   }
   //   else
   //   {
   //      doGroupBegin();
   //   }
   //   break;

   //case ArpTermType::TERM_IF: // done above in Control checks 
   //   doIf();
   //   break;
   //case ArpTermType::TERM_ELSE:
   //   doElse();
   //   break;
   case ArpTermType::TERM_PROBABILITY: 
      doProbability(pTerm);
      break;
   case ArpTermType::TERM_CONDITION:
      doCondition(pTerm);
      break;
   //case ArpTermType::TERM_EXPRESSION_BEGIN:
   //   doExpressionBegin(pTerm);
   //   break;
   //case ArpTermType::TERM_EXPRESSION_END:
   //   doExpressionEnd(pTerm);
   //   break;
   case ArpTermType::TERM_FUNCTION:
      doFunction(pTerm);
      break;
   case ArpTermType::TERM_REWRITABLE:
      //DEBUG( " orphaned Rewritable: %s (ignore)", pTerm->getThumbprint() );
      // do ?? anything 
      break;

   default:
    DEBUG( " ** Error: unexpected term type: %d (ignore)", pTerm->getTermType());
#ifdef INCLUDE_DEBUG_FUNCTIONS      
      DEBUG( " **        term thumbprint = %s (ignore)", pTerm->getThumbprint().c_str() );
#endif       
      break;
   }
}

void ArpPlayer::doNoteUp()
{
   int noteIndex    = mContextStack.top()->getNoteIndex();
   int octaveOffset = mContextStack.top()->getOctaveOffset();

   noteIndex = mArpStepper.setAndStepForward(noteIndex);

   mContextStack.top()->setNoteIndex(noteIndex);
   mContextStack.top()->setOctaveOffset(octaveOffset);

   // DEBUG(" doNoteUp: increased to %d, octave %d", noteIndex, octaveOffset );

   // //( "Arp Executor: Note UP" );
   // if (isIncrementDownward())
   // {
   //    decreaseNoteIndex();
   // }
   // else
   // {
   //    increaseNoteIndex();
   // }
}

void ArpPlayer::doNoteDown()
{
   int noteIndex    = mContextStack.top()->getNoteIndex();
   int octaveOffset = mContextStack.top()->getOctaveOffset();

   noteIndex = mArpStepper.setAndStepReverse(noteIndex);

   mContextStack.top()->setNoteIndex(noteIndex);
   mContextStack.top()->setOctaveOffset(octaveOffset);

   // DEBUG(" doNoteDown: decreased to %d, octave %d", noteIndex, octaveOffset );
   // //DEBUG( "Arp Executor: Note DOWN" );
   // if (isIncrementDownward())
   // {
   //    increaseNoteIndex();
   // }
   // else
   // {
   //    decreaseNoteIndex();
   // }
}

void ArpPlayer::doNoteRandom()
{
   //DEBUG( "Arp Executor: Note RANDOM" );
   // int noteIndex = mExpressionContext.getRandomRangeInteger(0, mMidiPitchPool.getNumNotes() - 1);
   int noteIndex = mExpressionContext.getRandomRangeInteger(0, mNoteTable.getNumNotes() - 1);
   mContextStack.top()->setNoteIndex(noteIndex);
}

void ArpPlayer::doNoteNominal()
{
   //DEBUG( "Arp Executor: Note NOMINAL" );
   mContextStack.top()->setNoteIndex(0);
}

void ArpPlayer::doNoteAssign(ExpandedTerm * pTerm)
{
   // DEBUG( "Arp Executor: Note ASSIGN" );
   int noteIndex = (int) evaluateExpression(pTerm);
   mContextStack.top()->setNoteIndex(noteIndex);
}

/**
 * noteOut = twelseToneScale.snapToScale(noteIn)
 * if (noteOut < 0)
 *     return -1 // discard 
 * noteOut = arpNoteRange.snapToRange(noteOut)
 * if (noteOut < 0)
 *     return -1 // discard 
 * 
 * NoteRange::snapToRange(int noteIn) const {
 *    int noteOut = noteIn;
 *    while (noteOut < mNoteMin) {
 *       noteOut += 12;
 *    }
 *    while (noteOut > mNoteMax) {
 *       noteOut -= 12;
 *    }
 *    return (isInRange(mNoteOut) ? noteOut : -1);
 * }
 * 
*/

void ArpPlayer::doPlayNote()
{
   //DEBUG( "Arp Executor: Play NOTE" );
   ArpContext * pContext = mContextStack.top();
   int noteIndex = pContext->getNoteIndex();
   int octaveOffset = pContext->getOctaveOffset();
   int semitoneOffset = pContext->getSemitoneOffset();

   int numNotes = mNoteTable.getNumNotes();
   if (numNotes == 0)
   {
      // DEBUG( "doPlayNote: no keys pressed, index = %d, size = %d", noteIndex, numNotes );
      pContext->setNoteIndex(0);
      return;
   }

   // DEBUG(" Player step: doPlayNote() noteIndex %d, numNotes %d, octave offset %d", noteIndex, numNotes, octaveOffset);

   // The note index could be out of bounds if the number of keys 
   // has decreased since the index in the context was saved.
   // If the index is out of bounds then adjust it to be in bounds.
   if (noteIndex < 0)
   {
      // DEBUG( "doPlayNote: note index is out of bounds (negative), index = %d, size = %d", noteIndex, mNoteTable.getNumNotes() );
      // traveling in downward direction 
      noteIndex = numNotes - 1;
      pContext->setNoteIndex(noteIndex);
   }

   // Determine which octave the note falls into
   // int numOctaves = (mArpOctaveOffsetMax - mArpOctaveOffsetMin) + 1;
   octaveOffset = mArpOctaveOffsetMin + (noteIndex / numNotes);
   // if (mOctaveIncludesPerfect && noteIndex == mArpStepper.getMaxValue()) { 
   //    octaveOffset += 1;
   // } 
   noteIndex = noteIndex % numNotes;
   // DEBUG( "doPlayNote: computed = note index to %d, octaveOffset to %d", noteIndex, octaveOffset);

//    else if (noteIndex >= numNotes)
//    {
//       DEBUG( "doPlayNote: octave offset %d, min %d, max %d", octaveOffset, mArpOctaveOffsetMin, mArpOctaveOffsetMax);
//       octaveOffset = clamp(mArpOctaveOffsetMin + (noteIndex / numNotes), mArpOctaveOffsetMin, mArpOctaveOffsetMax);
//       if (mOctaveIncludesPerfect && noteIndex == mArpStepper.getMaxValue()) { 
//          octaveOffset += 1;
//       } 
//       noteIndex = noteIndex % numNotes;
//       DEBUG( "doPlayNote: note index is out of bounds: updated index to %d, octaveOffset to %d", noteIndex, octaveOffset);

//       // TODO: handle octave perfect  .. 
// //       pContext->setOctaveOffset(octaveOffset);
//    }

   int noteValue;
   int noteChannel;

   if (isPlayOrderAsReceived())
   {
      noteValue = mNoteTable.getNotesArrivalOrder().getAt( noteIndex )->getNoteNumber();
   }
   else if (isPlayOrderLowToHigh())
   {
      noteValue = mNoteTable.getNotesLowToHighOrder().getAt( noteIndex )->getNoteNumber();
   }
   else if (isPlayOrderHighToLow())
   {
      noteValue = mNoteTable.getNotesHighToLowOrder().getAt( noteIndex )->getNoteNumber();
   }
   else // random
   {
      // favor a random note index that does not match the current note index 
      int newNoteIndex = round(getRandomZeroToOne() * float(numNotes - 1)); // -1 to make room for rounding up 
      if (newNoteIndex == noteIndex) { 
         newNoteIndex = round(getRandomZeroToOne() * float(numNotes - 1)); // -1 to make room for rounding up 
      }
      noteIndex = newNoteIndex;
      noteValue = mNoteTable.getNotesArrivalOrder().getAt( noteIndex )->getNoteNumber();
      pContext->setNoteIndex(noteIndex); // TODO: store noteindex in context? 
   }
   // DEBUG(" Player step: doPlayNote() noteValue[%d] is %d, octaveOffsetMin %d, octaveOffsetMax %d, ctx.octaveOffset %d, semitoneOffset %d", noteIndex, noteValue, mArpOctaveOffsetMin, mArpOctaveOffsetMax, octaveOffset, semitoneOffset);

   int noteSnapped = mTwelveToneScale.snapToScale(noteValue);
   if (noteSnapped < 0) {
      // DEBUG("OUT of Scale -- Discarded");
      return;
   }

   int noteOut = clamp(noteSnapped + (12 * octaveOffset), 0, 120); // TODO: const
   // DEBUG(" Player step: doPlayNote() noteValue after octave offset is %d", noteOut);


   //int notePicked = noteSnapped; 
   // if (mTwelveToneScale.isSnapToScale()) {
   //    notePicked = mArpNoteRange.snapToRange(noteSnapped);
   // } else if (mTwelveToneScale.isDiscard() && (! mArpNoteRange.includes(noteSnapped))) {
   //    notePicked = -1;
   // }
//   
//   int degree = noteSnapped % 12;
//  int notePicked = noteSnapped + (octaveOffset * 12) + (mArpOctaveOffset * 12);
   //DEBUG(" Player step: doPlayNote() --- experiment -- noteSnapped %d, notePicked %d", noteSnapped, notePicked);

   // if (notePicked < 0) {
   //    DEBUG("OUT of RANGE -- Discarded -- range is %d .. %d", mArpNoteRange.getMinValue(),  mArpNoteRange.getMaxValue());
   //    return;
   // }

   // noteValue += (12 * mArpOctaveOffset);
   // noteValue += (12 * octaveOffset);
   // DEBUG(" Player step: doPlayNote() noteValue after octave adjust = %d", noteValue);

   // DEBUG(" Player step: doPlayNote() arp note range = %d .. %d", mArpNoteRange.getMinValue(), mArpNoteRange.getMaxValue());
// TODO: this relationship between semitone and root/solo note is why 
// the "root/solo" note follows the harmony octave range when a 'semitone' 
// term ($s...) appears in the rules.
// "Semitone" settings adjust the value of the 'solo' note


   // int noteOut = noteValue;
   // noteOut = notePicked;


   // TODO: handle semitone .. and pick a better name

   // If semitone offset is non-zero then this note is treated as a Harmony note 
   // and is subject to the constraints of the Harmony range ... (at least for now)
   if ((semitoneOffset != 0) && (semitoneOffset != 1))
   {
      // int harmonyNote = 0;
      // // If snap-to-scale, interpret semitone offset as a scale interval offset
      // // otherwise interpret as chromatic offset 
      // if (mTwelveToneScale.isSnapToScale())
      // {
      //    // harmonyNote = mTwelveToneScale.getNoteAtInterval(noteValue, semitoneOffset); // TODO: delete 
      //    harmonyNote = mTwelveToneScale.applyInterval(noteValue, semitoneOffset, mHarmonyNoteRange);
      //    DEBUG(" Player step: doPlayNote(), is Semitone: SNAP TO SCALE, harmony note = %d", harmonyNote);
      // }
      // else
      // {
      //    harmonyNote += semitoneOffset;
      //    DEBUG(" Player step: doPlayNote(), is Semitone: not snapped, harmony note = %d", harmonyNote);
      // }
      //noteValue = clampHarmonyNote(harmonyNote, noteValue);
      //DEBUG(" Player step: doPlayNote(), semitone: after clamp, noteValue is %d", noteValue);
      noteOut = mTwelveToneScale.getNoteAtInterval(noteSnapped, semitoneOffset);
      // DEBUG(" Player step: doPlayNote(), semitone %d: note out is now %d", semitoneOffset, noteOut);
   }
   // else 
   // {
      // DEBUG(" Player step: doPlayNote(), not a semitone: snap it? %d", mTwelveToneScale.isSnapToScale());
      // // not a harmony note, but may still be snapped to scale 
      // if (mTwelveToneScale.isSnapToScale())
      // {
      //    //noteValue = mTwelveToneScale.getNoteAtInterval(noteValue, semitoneOffset); // TODO: delete
      //    noteValue = mTwelveToneScale.applyInterval(noteValue, semitoneOffset, mHarmonyNoteRange);
      //    // noteValue = mTwelveToneScale.snapToScale(noteValue);
      // }
      // DEBUG(" Player step: doPlayNote(), not a semitone: noteValue is %d", noteValue);

   //    noteOut = mTwelveToneScale.applyInterval(noteValue, semitoneOffset, mArpNoteRange);
   //    DEBUG(" Player step: doPlayNote(), NOT semitone: note value is now %d", noteValue);
   // }

   if (noteOut < 0) {
      // DEBUG(" Player step: doPlayNote(), note is discarded");
      return;
   }

//    int keyPressChannel = 0; // pKeyPress->getChannelNumber(); 


   // int noteLengthSamples = int(float(mNominalNoteLengthSamples) * pContext->getNoteLengthMultiplier());

// TODO: move mNoteEnabled check to top of method ?

   noteChannel = 0; // solo note always goes out channel 0 

   if (mNoteEnabled && isNoteProbable())
   {
      // DEBUG(" Player step: doPlayNote(), play note %d, channel %d", noteOut, noteChannel);
//      DEBUG("-- noteEnabled: keyPressChannel %d, note channel %d", keyPressChannel, mNoteChannel);
//      int channel = computeChannelNumber(keyPressChannel, mNoteChannel);
//      DEBUG("-- noteEnabled: result chan = %d", channel);
      int velocityValue = 1; // computeVelocity(pContext, pKeyPress);  // TODO: remove velocity 
      // sendNote(noteChannel, noteValue, velocityValue, noteLengthSamples, mNoteDelaySamples);
      sendNote(noteChannel, noteOut, velocityValue);
   }
   else 
   {
      // DEBUG(" Player step: doPlayNote(), note %d not played, enabled %d", noteValue, mNoteEnabled);
   }


   // send harmony notes, if enabled
   for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
   {
      //DEBUG(" harmony[%d] enabled = %d", i, isHarmonyEnabled(i));
      if (isHarmonyEnabled(i) && isHarmonyProbable())
      {
         //DEBUG(" harmony [%d] is probable.", i);
         int harmonyInterval = pContext->getHarmonyInterval(i);
         // DEBUG(" harmony [%d] interval is %d", i, harmonyInterval);
         if (harmonyInterval != 0)
         {
            // DEBUG(" Player step: doPlayNote(), harmony[%d] interval = %d", i, harmonyInterval);

            // int harmonyNote = mTwelveToneScale.getNoteAtInterval(noteValue, harmonyInterval); // TODO: delete 
            // int harmonyNote = mTwelveToneScale.applyInterval(noteValue, harmonyInterval, mHarmonyNoteRange);

            // Harmpony is based on original note 
            int harmonyNote = mTwelveToneScale.getNoteAtInterval(noteSnapped, harmonyInterval);
            if (harmonyNote < 0) {
               // DEBUG(" Player step: doPlayNote() --- experiment -- harmony[%d], note %d .. .OUT OF RANGE, skip it", i, harmonyNote);
               continue; // hack .. fix the logic 
            }
            harmonyNote += (octaveOffset * 12);
            // DEBUG(" Player step: doPlayNote() --- experiment -- harmony[%d], noteSnapped %d, harmonyNote %d", i, noteSnapped, harmonyNote);
            if (harmonyNote > 0)
            {
                 // DEBUG(" send Harmony [%d] note before clamp = %d. ", i, harmonyNote);
                harmonyNote = clampHarmonyNote(harmonyNote, noteSnapped);  // TODO: is this needed ? 
                 // DEBUG(" send Harmony [%d] note  after clamp = %d", i, harmonyNote);
               if (harmonyNote != noteValue)
               {
                  int velocityValue = 1; // computeHarmonyVelocity(i, pContext, pKeyPress);

                  // int channel = computeChannelNumber(keyPressChannel, mHarmonyChannel[i]);
                  int channel = i + 1; // TODO: ?? use incrementing counter to keep harmony channels compressed ??  
                  //DEBUG(" send Harmony [%d], chan %d, note %d, delaySamples %d", i, channel, harmonyNote, getHarmonyDelaySamples(i));
                  // DEBUG(" Player step: doPlayNote(), send Harmony [%d], chan %d, note %d", i, channel, harmonyNote);

                  // sendNote(channel, harmonyNote, velocityValue, noteLengthSamples, getHarmonyDelaySamples(i));
                  sendNote(channel, harmonyNote, velocityValue);
               }
            }
         }
      }
   }
}

void ArpPlayer::doPlayRest()
{
   //DEBUG( "Arp Executor: Play REST" );
   // do nothing
}

// void ArpPlayer::doPlayChord()
// {
//    //DEBUG( "Arp Executor: Play CHORD" );
//    sendAllNotes(0);
// }

// void ArpPlayer::setStaggerSamples(int staggerSamples)
// {
//    mStaggerSamples = staggerSamples;
//    if (mStaggerSamples < 0)
//    {
//       mStaggerSamples = 0;
//    }
// }

// void ArpPlayer::doPlayGliss()
// {
//    //DEBUG( "Arp Executor: Play GLISS" );
//    ArpContext * pContext = getCurrentContext();
//    float speed = pContext->getGlissSpeed();
//    bool isDirUpward = pContext->isGlissDirectionUpward();
//    int staggerSamples = int(speed * float(mNominalNoteLengthSamples));
//    sendAllNotes(staggerSamples, isDirUpward);
// }

// void ArpPlayer::doGlissSpeedUp()
// {
//    //DEBUG( "Arp Executor: Gliss SPEED UP" );
//    ArpContext * pContext = getCurrentContext();
//    float speed = pContext->getGlissSpeed();
//    speed = mGlissSpeedBounder.setAndIncrement(speed);
//    pContext->setGlissSpeed(speed);
// }

// void ArpPlayer::doGlissSpeedDown()
// {
//    //DEBUG( "Arp Executor: Gliss SPEED DOWN" );
//    ArpContext * pContext = getCurrentContext();
//    float speed = pContext->getGlissSpeed();
//    speed = mGlissSpeedBounder.setAndDecrement(speed);
//    pContext->setGlissSpeed(speed);
// }

// void ArpPlayer::doGlissSpeedRandom()
// {
//    //DEBUG( "Arp Executor: Gliss SPEED RANDOM" );
//    float speed = mExpressionContext.getRandomZeroOne();
//    getCurrentContext()->setGlissSpeed(speed);
// }

// void ArpPlayer::doGlissSpeedNominal()
// {
//    //DEBUG( "Arp Executor: Gliss SPEED NOMINAL" );
//    float speed = mGlissSpeedBounder.getNominalValue();
//    getCurrentContext()->setGlissSpeed(speed);
// }

// void ArpPlayer::doGlissSpeedAssign(ExpandedTerm * pTerm)
// {
//    //DEBUG( "Arp Executor: Gliss SPEED ASSIGN" );
//    float speed = evaluateExpression(pTerm);
//    speed = mGlissSpeedBounder.clamp(speed);
//    getCurrentContext()->setGlissSpeed(speed);
// }

// void ArpPlayer::doGlissDirectionUp()
// {
//    //DEBUG( "Arp Executor: Gliss DIRECTION UP" );
//    ArpContext * pContext = getCurrentContext();
//    bool isDirUpward = pContext->isGlissDirectionUpward();
//    pContext->setGlissDirectionUpward( ! isDirUpward );
// }

// void ArpPlayer::doGlissDirectionDown()
// {
//    //DEBUG( "Arp Executor: Gliss DIRECTION DOWN" );
//    ArpContext * pContext = getCurrentContext();
//    bool isDirUpward = pContext->isGlissDirectionUpward();
//    pContext->setGlissDirectionUpward( ! isDirUpward );
// }

// void ArpPlayer::doGlissDirectionRandom()
// {
//    //DEBUG( "Arp Executor: Gliss DIRECTION RANDOM" );
//    float value = mExpressionContext.getRandomZeroOne();
//    getCurrentContext()->setGlissDirectionUpward( value > 0.5 );
// }

// void ArpPlayer::doGlissDirectionNominal()
// {
//    //DEBUG( "Arp Executor: Gliss DIRECTION NOMINAL" );
//    getCurrentContext()->setGlissDirectionUpward( true );
// }

// void ArpPlayer::doGlissDirectionAssign(ExpandedTerm * pTerm)
// {
//    //DEBUG( "Arp Executor: Gliss DIRECTION ASSIGN" );
//    float value = evaluateExpression(pTerm);
//    getCurrentContext()->setGlissDirectionUpward( value > 0.5 );
// }

// void ArpPlayer::doVelocityUp()
// {
//    //DEBUG( "Arp Executor: Velocity UP" );
//    ArpContext * pContext = mContextStack.top();
//    int velocityIncrement = pContext->getVelocityIncrement();
//    velocityIncrement++;
//    velocityIncrement = mVelocityManager.clamp(velocityIncrement);
//    pContext->setVelocityIncrement(velocityIncrement);

// }

// void ArpPlayer::doVelocityDown()
// {
//    //DEBUG( "Arp Executor: Velocity DOWN" );
//                                                                 // todo rename velocityIncrement to velocitySubrange or something 
//    ArpContext * pContext = mContextStack.top();
//    int velocityIncrement = pContext->getVelocityIncrement();
//    velocityIncrement--;
//    velocityIncrement = mVelocityManager.clamp(velocityIncrement);
//    pContext->setVelocityIncrement(velocityIncrement);
// }

// void ArpPlayer::doVelocityRandom()
// {
//    //DEBUG( "Arp Executor: Velocity RANDOM" );
//    int velocityIncrement = mExpressionContext.getRandomRangeInteger(0, mVelocityManager.getNumDivisions() - 1);
//    mContextStack.top()->setVelocityIncrement(velocityIncrement);
// }

// void ArpPlayer::doVelocityNominal()
// {
//    //DEBUG( "Arp Executor: Velocity NOMINAL" );
//    ArpContext * pContext = mContextStack.top();
//    pContext->setVelocityIncrement( mVelocityManager.getNominalDivisionNumber() );
// }

// void ArpPlayer::doVelocityAssign(ExpandedTerm * pTerm)
// {
//    //DEBUG( "Arp Executor: Velocity ASSIGN" );
//    int velocityIncrement = (int) evaluateExpression(pTerm);
//    velocityIncrement = clamp(velocityIncrement, 0, mVelocityManager.getNumDivisions());
//    mContextStack.top()->setVelocityIncrement(velocityIncrement);
// }

void ArpPlayer::doSemitoneUp()
{
   //DEBUG( "Arp Executor: Semitone UP" );
   int semitoneOffset = mContextStack.top()->getSemitoneOffset();
   semitoneOffset = mSemitoneIncrementStepper.setAndStepForward(semitoneOffset);
   mContextStack.top()->setSemitoneOffset( semitoneOffset );
}

void ArpPlayer::doSemitoneDown()
{
   //DEBUG( "Arp Executor: Semitone DOWN" );
   int semitoneOffset = mContextStack.top()->getSemitoneOffset();
   semitoneOffset = mSemitoneIncrementStepper.setAndStepReverse(semitoneOffset);
   mContextStack.top()->setSemitoneOffset( semitoneOffset );
}

void ArpPlayer::doSemitoneRandom()
{
   //DEBUG( "Arp Executor: Semitone RANDOM" );
   int semitoneOffset = mExpressionContext.getRandomRangeInteger(mSemitoneIncrementStepper.getMinValue(), mSemitoneIncrementStepper.getMaxValue());
   mContextStack.top()->setSemitoneOffset(semitoneOffset);
}

void ArpPlayer::doSemitoneNominal()
{
   //DEBUG( "Arp Executor: Semitone NOMINAL" );
   int semitoneIncrement = mSemitoneIncrementStepper.getNominalValue();
   mContextStack.top()->setSemitoneOffset( semitoneIncrement );
}

void ArpPlayer::doSemitoneAssign(ExpandedTerm * pTerm)
{
   //DEBUG( "Arp Executor: Semitone ASSIGN" );
   int semitoneOffset = (int) evaluateExpression(pTerm);
   if (semitoneOffset >= 0) {
      semitoneOffset = mSemitoneIncrementStepper.clamp(semitoneOffset);
   } else {
      semitoneOffset = -(mSemitoneIncrementStepper.clamp(-semitoneOffset));
   }
   mContextStack.top()->setSemitoneOffset(semitoneOffset);
}

void ArpPlayer::doOctaveUp()
{
   //DEBUG( "Arp Executor: Octave UP" );
   int octaveOffset = mContextStack.top()->getOctaveOffset();
   octaveOffset = clamp(octaveOffset + 1, mArpOctaveOffsetMin, mArpOctaveOffsetMax);
   mContextStack.top()->setOctaveOffset(octaveOffset);
}

void ArpPlayer::doOctaveDown()
{
   //DEBUG( "Arp Executor: Octave DOWN" );
   int octaveOffset = mContextStack.top()->getOctaveOffset();
   octaveOffset = clamp(octaveOffset -1, mArpOctaveOffsetMin, mArpOctaveOffsetMax);
   mContextStack.top()->setOctaveOffset(octaveOffset);
}

void ArpPlayer::doOctaveRandom()
{
   //DEBUG( "Arp Executor: Octave RANDOM" );
   int octaveOffset = mExpressionContext.getRandomRangeInteger(mArpOctaveOffsetMin, mArpOctaveOffsetMax);
   mContextStack.top()->setOctaveOffset(octaveOffset);
}

void ArpPlayer::doOctaveNominal()
{
   //DEBUG( "Arp Executor: Octave NOMINAL" );
   mContextStack.top()->setOctaveOffset(0);
}

void ArpPlayer::doOctaveAssign(ExpandedTerm * pTerm)
{
   //DEBUG( "Arp Executor: Octave ASSIGN" );
   int octaveOffset = (int) evaluateExpression(pTerm);
   octaveOffset = clamp(octaveOffset, mArpOctaveOffsetMin, mArpOctaveOffsetMin);
   mContextStack.top()->setOctaveOffset(octaveOffset);
}

void ArpPlayer::doHarmonyUp(int harmonyId)
{
   // DEBUG( "Arp Executor: Harmony %d UP", harmonyId );
   if (isValidHarmonyId(harmonyId))
   {
      int harmonyInterval = mContextStack.top()->getHarmonyInterval(harmonyId);
      harmonyInterval = mHarmonyIntervalSteppers[ harmonyId ].setAndStepForward(harmonyInterval);
      mContextStack.top()->setHarmonyInterval( harmonyId, harmonyInterval );
   }
}

void ArpPlayer::doHarmonyDown(int harmonyId)
{
   //DEBUG( "Arp Executor: Harmony %d DOWN", harmonyId );
   if (isValidHarmonyId(harmonyId))
   {
      int harmonyInterval = mContextStack.top()->getHarmonyInterval(harmonyId);
      harmonyInterval = mHarmonyIntervalSteppers[ harmonyId ].setAndStepReverse(harmonyInterval);
      mContextStack.top()->setHarmonyInterval(harmonyId, harmonyInterval);
   }
}

void ArpPlayer::doHarmonyRandom(int harmonyId)
{
   //DEBUG( "Arp Executor: Harmony %d RANDOM", harmonyId );
   if (isValidHarmonyId(harmonyId))
   {
      int harmonyInterval = mExpressionContext.getRandomRangeInteger(
         mHarmonyIntervalSteppers[ harmonyId ].getMinValue(), 
         mHarmonyIntervalSteppers[ harmonyId ].getMaxValue());
      mContextStack.top()->setHarmonyInterval(harmonyId, harmonyInterval);
   }
}

void ArpPlayer::doHarmonyNominal(int harmonyId)
{
   // DEBUG( "Arp Executor: Harmony %d NOMINAL", harmonyId );
   if (isValidHarmonyId(harmonyId))
   {
      int harmonyInterval = mHarmonyIntervalSteppers[ harmonyId ].getNominalValue();
      mContextStack.top()->setHarmonyInterval(harmonyId, harmonyInterval );
   }
}

void ArpPlayer::doHarmonyAssign(int harmonyId, ExpandedTerm * pTerm)
{
   // DEBUG( "Arp Executor: Harmony %d ASSIGN", harmonyId );
   if (isValidHarmonyId(harmonyId))
   {
      int harmonyInterval = (int) evaluateExpression(pTerm);
      if (harmonyInterval >= 0) {
         harmonyInterval = mHarmonyIntervalSteppers[ harmonyId ].clamp(harmonyInterval);
      } else {
         harmonyInterval = -(mHarmonyIntervalSteppers[ harmonyId ].clamp(-harmonyInterval));
      }
      mContextStack.top()->setHarmonyInterval(harmonyId, harmonyInterval);
   }
}

void ArpPlayer::doUpDownInvert()
{
   // DEBUG( "Arp Executor: UpDown INVERT" );
   mIsReflecting = (! mIsReflecting);
}

void ArpPlayer::doUpDownNominal()
{
   // DEBUG( "Arp Executor: UpDown NOMINAL" );
   mIsReflecting = false;
}

void ArpPlayer::doContextPush()
{
   //DEBUG( "Arp Executor: Context PUSH" );
   ArpContext * pContext = mFreePoolArpContext.allocate();
   pContext->copyFrom( mContextStack.top() );
   mContextStack.push(pContext);
}

void ArpPlayer::doContextPop()
{
   //DEBUG( "Arp Executor: Context POP" );
   if (mContextStack.size() <= 1)
   {
      // cannot pop the initial context - misy have at least one context at all times 
      // log something 
      return; 
   }

   ArpContext * pContext = mContextStack.pop();
   mFreePoolArpContext.retire(pContext);
}

//void ArpPlayer::doGroupBegin()
//{
//   //DEBUG( "Arp Executor: Group BEGIN" );
//   // nothing to do
//}
//
//void ArpPlayer::doGroupEnd()
//{
//   //DEBUG( "Arp Executor: Group END" );
//   // nothing to do
//}

void ArpPlayer::doIf()
{
   //DEBUG( "Do IF" );
   // do nothing ...
}

void ArpPlayer::doElse()
{
   //DEBUG( "Do ELSE" );
   toggleExecutionState();
}

void ArpPlayer::doExpressionBegin(ExpandedTerm * pTerm)
{
   //DEBUG( "Do EXPRESSION BEGIN" );
   if (isScanningForward())
   {
      pushExecutionState(pTerm);
   }
   else
   {
      popExecutionState();
   }
}

void ArpPlayer::doExpressionEnd(ExpandedTerm * pTerm)
{
   //DEBUG( "Do EXPRESSION END" );
   if (isScanningForward())
   {
      popExecutionState();
   }
   else
   {
      ExpandedTerm * pBeginTerm = findExpressionBeginTerm(pTerm);
      // assert(pTerm);
      pushExecutionState(pBeginTerm);
   }
}

void ArpPlayer::doFunction(ExpandedTerm * pTerm)
{
   // DEBUG( "Do FUNCTION, name = %s", pTerm->getName().c_str() );

   // BARE function ... is this used ?
   // maybe if @incr(symbol) and @decr(symbol), @randomize($sym) ... were supported 

   std::string functionName = pTerm->getName();

   FunctionDescription * pFunctionDescription = getFunctionDescriptionByName( functionName );
   if (pFunctionDescription == 0)
   {
      DEBUG( "ERROR: no such function: %s", functionName.c_str() );
      return;
   }
   ArgumentValuesList * pArgValues = pTerm->getActualParameterValues();
   mExpressionContext.callFunction(pFunctionDescription, (*pArgValues));
}

void ArpPlayer::doProbability(ExpandedTerm * pTerm)        //  <<<<<<<<<<< is this still applicable ?
{
   // DEBUG( "Arp Executor: PROBABILITY" ); 
   Expression * pCondition = pTerm->getCondition(); 
   if (pCondition == 0)
   {
      // todo, log, something is very wrong 
      // DEBUG( "Probability term has null condition ! %s", pTerm->getName().c_str() );
      return;
   }

   bool isTrue = pCondition->isTrue(&mExpressionContext);
   // DEBUG( "Probability results:  %d", isTrue );

   mIsExecuting = isTrue;
   // set execution state based on pTerm->value() >= random.zeroToOne()

}

void ArpPlayer::doCondition(ExpandedTerm * pTerm)        // TODO:  <<<<<<<<<<< is this still applicable ?
{
   DEBUG( "Arp Executor: Do CONDITION" );
   // set execution state based on evaluation of condition ?? 
}


float ArpPlayer::evaluateExpression(ExpandedTerm * pTerm)
{
   assert(pTerm != 0);

   Expression * pExpression = pTerm->getCondition(); 
   if (pExpression == 0)
   {
      DEBUG( "ERROR: EvaluateExpression called with Term that has no expression, return 0, term = %s", pTerm->getName().c_str() );
      return 0.0;
   }

   float value = pExpression->getValue(&mExpressionContext);
   return value;
}


ExpandedTerm * ArpPlayer::findExpressionBeginTerm(ExpandedTerm * pExpressionEndTerm)
{
   assert(pExpressionEndTerm != 0);

   int nestedCount = 0;

   // Scan backwards throgh term list 
   // hack - should use a nice iterator or something for this 
   ExpandedTerm * pTerm = (ExpandedTerm *) pExpressionEndTerm->getPrevNode();
   while (pTerm != 0)
   {
      ArpTermType termType = (ArpTermType) pTerm->getTermType();
      if (termType == ArpTermType::TERM_EXPRESSION_END)
      {
         nestedCount++; 
      }
      else if (termType == ArpTermType::TERM_EXPRESSION_BEGIN)
      {
         if (nestedCount == 0)
         {
            return pTerm; // << match 
         }
         nestedCount--; 
      }

      pTerm = (ExpandedTerm *) pTerm->getPrevNode();
   }

   return 0; // no match (Should never get here)
}


void ArpPlayer::pushExecutionState(ExpandedTerm * pTerm)
{
   assert(pTerm != 0);

   if (isExecutionEnabled()) 
   {
      float value = evaluateExpression(pTerm);

      // DEBUG( "Push Execution State -- value is %lf", value );
         
      mIsExecuting = (value > 0.0);

      mExecutionStateStack.push(mIsExecuting);
   }
   else
   {
      mExecutionStateStack.push(false);
   }
}

void ArpPlayer::popExecutionState()
{
   assert(mExecutionStateStack.getNumMembers() > 1);

   mExecutionStateStack.pop();
   mIsExecuting = mExecutionStateStack.peek();
}

void ArpPlayer::toggleExecutionState()
{
   assert(mExecutionStateStack.getNumMembers() > 1);

   // else behavior
   //   current execution state   outer/enclosing state     result state
   //         true                    false                    false      << should never happen that current state is TRUE while enclosingis FALSE
   //         true                    true                     false     (toggle)
   //         false                   false                    false     (enclosing state is false)
   //         false                   true                     true      (toggle)
   //
   mIsExecuting = mExecutionStateStack.pop();
   mIsExecuting = (! mIsExecuting) && mExecutionStateStack.peek();
   mExecutionStateStack.push(mIsExecuting);
}



// ============================================================
// Helper methods


void ArpPlayer::drainContextStack()
{
   while (mContextStack.isEmpty() == false)
   {
      ArpContext * pContext = mContextStack.pop();
      mFreePoolArpContext.retire(pContext);
   }
}

void ArpPlayer::pushInitialContext()
{
   assert(mContextStack.isEmpty());

   ArpContext * pContext = mFreePoolArpContext.allocate();
   pContext->setNoteIndex(0);
   pContext->setOctaveOffset(0);
   pContext->setSemitoneOffset(0);
   for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
   {
      pContext->setHarmonyInterval(i, 0);
   }
   //pContext->setVelocityIncrement(mVelocityManager.getNominalDivisionNumber());  //<< todo: handle velocity adjustments
   // pContext->setGlissDirectionUpward(true);
   // pContext->setGlissSpeed( mGlissSpeedBounder.getNominalValue() );

   mContextStack.push(pContext);
}


// void ArpPlayer::setToLowestNote(int & noteIndex, int & octaveOffset)
// {
//    octaveOffset = mArpOctaveOffsetMin; 
//    noteIndex    = 0;
//    // DEBUG("-- Set to lowest note: octaveOffset %d, noteIndex %d", octaveOffset, noteIndex);
// }

// void ArpPlayer::setToLowestNotePlusOne(int & noteIndex, int & octaveOffset, int numNotes)
// {
//    octaveOffset = mArpOctaveOffsetMin; 
//    noteIndex    = 1;
//    if (noteIndex >= numNotes)
//    {
//       noteIndex = 0;
//    }
//    // DEBUG("-- Set to lowest note Plus One: octaveOffset %d, noteIndex %d", octaveOffset, noteIndex);
// }

// void ArpPlayer::setToHighestNote(int & noteIndex, int & octaveOffset, int numNotes)
// {
//    // reset to highest note
//    if (mOctaveIncludesPerfect)
//    {
//       octaveOffset = mArpOctaveOffsetMax + 1; 
//       noteIndex    = 0;
//       // DEBUG("-- Set to highest note: includes perfect: octaveOffset %d, noteIndex %d", octaveOffset, noteIndex);
//    }
//    else
//    {
//       octaveOffset = mArpOctaveOffsetMax;
//       noteIndex    = (numNotes - 1);
//       if (noteIndex < 0)
//       {
//          noteIndex = 0; 
//       }
//       // DEBUG("-- Set to highest note: not perfect: octaveOffset %d, noteIndex %d", octaveOffset, noteIndex);
//    }
// }

// void ArpPlayer::setToHighestNoteMinusOne(int & noteIndex, int & octaveOffset, int numNotes)
// {
//    octaveOffset = mArpOctaveOffsetMax;
//    if (mOctaveIncludesPerfect)
//    {
//       noteIndex = (numNotes - 1);
//    }
//    else
//    {
//       noteIndex  = (numNotes - 1) - 1;
//    }
//    if (noteIndex < 0)
//    {
//       noteIndex = 0; 
//    }
//    // DEBUG("-- Set to highest note Minus One: not perfect: octaveOffset %d, noteIndex %d", octaveOffset, noteIndex);
// }


// void ArpPlayer::increaseNoteIndex()
// {
//    int noteIndex    = mContextStack.top()->getNoteIndex();
//    int octaveOffset = mContextStack.top()->getOctaveOffset();

//    noteIndex += getNoteStride();

//    int numNotes = mNoteTable.getNumNotes(); 

// DEBUG("IncreaseNoteIndex: noteIndex %d, numNotes %d, octaveOffset %d, octaveMin %d, octaveMax %d", noteIndex, numNotes, octaveOffset, mArpOctaveOffsetMin, mArpOctaveOffsetMax);
//    //if (noteIndex >= numNotes)
//    if ((noteIndex >= numNotes) || (octaveOffset > mArpOctaveOffsetMax)) //in case prev octave offset was due to including the perfect octave  
//    {
//       octaveOffset++;
//       if (octaveOffset > mArpOctaveOffsetMax)
//       {
//          if ((octaveOffset == (mArpOctaveOffsetMax + 1)) && mOctaveIncludesPerfect)
//          {
//             // leave octave offset as is
//             noteIndex = 0; 
//          }
//          else if (isCycleUpwardOnOutOfBounds())
//          {
//             setToLowestNote(noteIndex, octaveOffset);
//          }
//          else if (isCycleDownwardOnOutOfBounds())
//          {
//             toggleIncrementDirection();
//             setToHighestNote(noteIndex, octaveOffset, numNotes);
//          }
//          else if (isFlipOnOutOfBounds())
//          {
//             octaveOffset = 0;            // nominal octave 
//             noteIndex    = 0;            // nominal note index 
//             toggleIncrementDirection();  // toggle direction
//          }
//          // else if (isStayAtEdgeOnOutOfBounds())  // TODO: is stayAtEdge useful ?? 
//          // {
//          //    setToHighestNote(noteIndex, octaveOffset, numNotes);
//          // }
//          else if (isResetToNominalOnOutOfBounds())
//          {
//             octaveOffset = 0;
//             noteIndex    = 0;
//          }
//          else if (isReflectLingerOnOutOfBounds())
//          {
//             toggleIncrementDirection();
//             setToHighestNote(noteIndex, octaveOffset, numNotes);
//          }
//          else if (isReflectOnOutOfBounds())
//          {
//             toggleIncrementDirection();
//             setToHighestNoteMinusOne(noteIndex, octaveOffset, numNotes);
//          }
//          else if (isDiscardOnOutOfBounds())
//          {
//             toggleIncrementDirection();
//             setToHighestNote(noteIndex, octaveOffset, numNotes);
//          }
//          else
//          {
//             DEBUG("Player Increase: bad Boundary Strategy: %d", mOverstepAction);
//             mOverstepAction = BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_UP;
//          }
//       }
//       else
//       {
//          noteIndex = 0; // first note in next octave
//       }
//    }

//    mContextStack.top()->setNoteIndex(noteIndex);
//    mContextStack.top()->setOctaveOffset(octaveOffset);
//    DEBUG(" NoteIdex: increased to %d, octave %d", noteIndex, octaveOffset );
// }


// void ArpPlayer::decreaseNoteIndex()
// {
//    int noteIndex    = mContextStack.top()->getNoteIndex();
//    int octaveOffset = mContextStack.top()->getOctaveOffset();

//    noteIndex -= getNoteStride();

//    if (noteIndex < 0)
//    {
//       int numNotes = mNoteTable.getNumNotes(); 

//       octaveOffset--;
//       if (octaveOffset < mArpOctaveOffsetMin)
//       {
//          if (isCycleUpwardOnOutOfBounds())
//          {
//             toggleIncrementDirection();
//             setToLowestNote(noteIndex, octaveOffset);
//          }
//          else if (isCycleDownwardOnOutOfBounds())
//          {
//             setToHighestNote(noteIndex, octaveOffset, numNotes);
//          }
//          else if (isFlipOnOutOfBounds())
//          {
//             octaveOffset = 0;            // nominal octave 
//             noteIndex    = 0;            // nominal note index 
//             toggleIncrementDirection();  // toggle direction
//          }
//          // else if (isStayAtEdgeOnOutOfBounds()) // TODO: is stayAtEdge useful ? 
//          // {
//          //    octaveOffset = mOctaveOffsetMin; 
//          //    noteIndex += getNoteStride();  // undo the decrement to leave the note index where it was
//          // }
//          else if (isResetToNominalOnOutOfBounds())
//          {
//             octaveOffset = 0;
//             noteIndex    = 0;
//          }
//          else if (isReflectLingerOnOutOfBounds())
//          {
//             toggleIncrementDirection();
//             setToLowestNote(noteIndex, octaveOffset);
//          }
//          else if (isReflectOnOutOfBounds())
//          {
//             toggleIncrementDirection();
//             setToLowestNotePlusOne(noteIndex, octaveOffset, numNotes);
//          }
//          else if (isDiscardOnOutOfBounds())
//          {
//             toggleIncrementDirection();
//             setToLowestNote(noteIndex, octaveOffset);
//          }
//          else
//          {
//             DEBUG("Player Decrease: bad Boundary Strategy: %d", mOverstepAction);
//             mOverstepAction = BoundaryStrategy::BOUNDARY_STRATEGY_CYCLE_DOWN;
//          }
//       }
//       else
//       {
//          noteIndex = numNotes - 1; // last note in prev octave
//          if (noteIndex < 0)
//          {
//             noteIndex = 0;
//          }
//       }
//    }

//    mContextStack.top()->setNoteIndex(noteIndex);
//    mContextStack.top()->setOctaveOffset(octaveOffset);

//    DEBUG(" NoteIndex: decreased to %d, octave %d", noteIndex, octaveOffset );
// }

//void ArpPlayer::sendNote(int channelNumber, int noteNumber, int velocityValue, int noteLengthSamples, int delaySamples)
void ArpPlayer::sendNote(int channelNumber, int noteNumber, int velocityValue)
{
   //if (mNoteProbability < mExpressionContext.getRandomZeroOne())
   //{
   //   return;
   //}

   // if (noteLengthSamples <= 0)
   // {
   //    // noteLengthSamples = 100; // << ?? pass 0, ?? use const 
   //    return;
   // }

   if (velocityValue <= 0)
   {
      return;
   }

   noteNumber = clamp(noteNumber, 0, 127);       // < todo: use const 
   channelNumber = clamp(channelNumber, 0, 15);  // < todo: use const 
   velocityValue = clamp(velocityValue, 0, 127); // < todo: use const 

   // // ------------------------------------------
   // // Snap to Scale 
   // noteNumber = mTwelveToneScale.snapToScale(noteNumber);   // << todo: duplicate work? remove this  ?? 
   // if (noteNumber < 0)
   // {
   //    DEBUG( "note is outlier: discarding" );
   //    return; // outlier
   // }

   // TODO: change this to collect the output notes and have the main process() 
   // method pull the values and assign to single output[] with polyphony and/or
   // multiple individual outputs[], one per harmony note  

   if (mpArpNoteWriter)
   {
      // DEBUG( "send note to output stream: chan %d, note %d, velocity %d", channelNumber, noteNumber, velocityValue);
      mpArpNoteWriter->noteOn(channelNumber, noteNumber, velocityValue);
   }
}

// void ArpPlayer::sendAllNotes(int staggerSamples, bool isDirUpward)
// {
//    ArpContext * pContext = getPlayableContext();
//    if (pContext == 0)
//    {
//       // nothing to play
//       return;
//    }

//    if (mNoteEnabled == false || isNoteProbable() == false)
//    {
//       return;
//    }

//    int noteIndex = pContext->getNoteIndex();
//    int octaveOffset = pContext->getOctaveOffset();
// //   int velocityIncrement = pContext->getVelocityIncrement();
//    int semitoneOffset = pContext->getSemitoneOffset();

//    // The note index could be out of bounds if the number of keys 
//    // has decreased since the index in the context was saved.
//    // If the index is out of bounds then adjust it to be in bounds.
//    // int numNotes = mMidiPitchPool.getNumNotes();
//    int numNotes = mNoteTable.getNumNotes();
//    // DEBUG("ArpPlayer::sendAllNotes(), numNotes = %d", numNotes);
//    if (noteIndex >= numNotes)
//    {
//       noteIndex = 0;
//       pContext->setNoteIndex(0);
//    }

//    int prevNote = 0;
//    if (isDirUpward == false)
//    {
//       prevNote = 120; // TODO: const, MAX_VOCT_NOTE   // NUM_MIDI_NOTES;
//    }

//    int delay = 0; 

//    for (int i = 0; i < numNotes; i++)
//    {
//       // int noteValue = mMidiPitchPool.getNotesLowToHighOrder()[ noteIndex ];
//       int noteValue = mNoteTable.getNotesLowToHighOrder().getAt( noteIndex )->getNoteNumber();
//       noteValue += semitoneOffset;
//       noteValue += (12 * octaveOffset);

//       if (isDirUpward)
//       {
//          while (noteValue < prevNote)
//          {
//             noteValue += 12; // increase octave to force notes to emit in ascending order 
//          }
//       }
//       else
//       {
//          while (noteValue > prevNote)
//          {
//             noteValue -= 12; // decrease octave to force notes to emit in descending order 
//          }
//       }

//       prevNote = noteValue;

//       int channelValue = i; // pKeyPress->getChannelNumber(); 

//       int velocityValue = 1; // computeVelocity(pContext, pKeyPress);

//       // TODO: use fast float to int here 
//       // int noteLengthSamples = int(float(mNominalNoteLengthSamples) * pContext->getNoteLengthMultiplier());

//       // todo: snap to scale here 
//       //    remove it from sendNote() 

//       // sendNote(channelValue, noteValue, velocityValue, noteLengthSamples, delay);
//       sendNote(channelValue, noteValue, velocityValue);
//       delay += staggerSamples;

//       if (isDirUpward)
//       {
//          noteIndex++; 
//          if (noteIndex >= numNotes)
//          {
//             noteIndex = 0;
//          }
//       }
//       else // isDownward
//       {
//          noteIndex--; 
//          if (noteIndex < 0)
//          {
//             noteIndex = numNotes - 1;
//          }
//       }
//    }

// }

ArpContext * ArpPlayer::getPlayableContext()
{
   ArpContext * pContext = mContextStack.top();

   // The note index could be out of bounds if the number of keys 
   // has decreased since the index in the context was saved.
   // If the index is out of bounds then adjust it to be in bounds.

   // int numNotes = mMidiPitchPool.getNumNotes();
   int numNotes = mNoteTable.getNumNotes();
   if (numNotes == 0)
   {
      //DEBUG( "getPlayableContext: no keys pressed, size = %d", mNoteTable.getNumNotes() );
      pContext->setNoteIndex(0);
      return 0; // nothing to play
   }

   int noteIndex = pContext->getNoteIndex();
   if (noteIndex < 0)
   {
      // DEBUG( "getPlayableContext: note index is out of bounds (negative), index = %d, size = %d", noteIndex, mNoteTable.getNumNotes() );
      // traveling in downward direction 
      noteIndex = numNotes - 1;
      pContext->setNoteIndex(noteIndex);
   }
   else if (noteIndex >= numNotes)
   {
      // DEBUG( "getPlayableContext: note index is out of bounds (positive), index = %d, size = %d", noteIndex, mNoteTable.getNumNotes() );
      noteIndex = 0;
      pContext->setNoteIndex(noteIndex);
   }

   return pContext;
}

int ArpPlayer::computeChannelNumber(int channelNumber, int configuredChannelNumber)
{
   // DEBUG("ArpPlayer::computeChannelNumber(), chan %d, configuredChannelNumber %d", channelNumber, configuredChannelNumber);
   if (configuredChannelNumber == 0)
   {
      return channelNumber;
   }
   return configuredChannelNumber - 1; // convert 1..16 to 0..15
}

// int ArpPlayer::computeVelocity(ArpContext * pContext, KeyPress * pKeyPress)
// {
//    assert(pContext != 0); 
//    assert(pKeyPress != 0); 

//    // adjust the context if the velocity increment is out of bounds
//    int velocityIncrement = clampVelocityIncrement(pContext);

//    int velocityValue;

//    if (mVelocityMode == VelocityMode::VELOCITY_MODE_AS_PLAYED)
//    {
//       velocityValue = pKeyPress->getNoteVelocity();
//       velocityIncrement = mVelocityManager.getDivisionNumberForVelocity(velocityValue);
//       if (velocityIncrement == 0)
//       {
//          return velocityValue; 
//       }
//    }

//    velocityIncrement = mVelocityManager.clamp(velocityIncrement + mNoteRelativeVelocity);
//    velocityValue = mVelocityManager.getVelocity(velocityIncrement);
//    return velocityValue;
// }

// int ArpPlayer::computeHarmonyVelocity(int harmonyIdx, ArpContext * pContext, KeyPress * pKeyPress)
// {
//    assert(pContext != 0); 
//    assert(pKeyPress != 0); 

//    // adjust the context if the velocity increment is out of bounds
//    int velocityIncrement = clampVelocityIncrement(pContext);
   
//    int velocityValue;

//    if (mVelocityMode == VelocityMode::VELOCITY_MODE_AS_PLAYED)
//    {
//       velocityValue = pKeyPress->getNoteVelocity();
//       velocityIncrement = mVelocityManager.getDivisionNumberForVelocity(velocityValue);
//    }

//    velocityIncrement = mVelocityManager.clamp(velocityIncrement + mHarmonyRelativeVelocity);
//    velocityValue = mVelocityManager.getVelocity(velocityIncrement) * mHarmonyVelocityMultiplier[ harmonyIdx ];
//    return velocityValue;
// }

// int ArpPlayer::clampVelocityIncrement(ArpContext * pContext)
// {
//    int velocityIncrement = pContext->getVelocityIncrement();

//    if (velocityIncrement >= mVelocityManager.getNumDivisions())
//    {
//       velocityIncrement = mVelocityManager.getNumDivisions() - 1;
//       pContext->setVelocityIncrement(velocityIncrement);
//    }
//    else if (velocityIncrement < 0) // this should be checked when the incrememnt is updated 
//    {
//       velocityIncrement = 0; 
//       pContext->setVelocityIncrement(velocityIncrement);
//    }

//    return velocityIncrement;
// }


int ArpPlayer::clampHarmonyNote(int harmonyNote, int baseNote)
{
   int baseDegree = baseNote % 12; 
   int harmonyDegree = harmonyNote % 12; 

   int delta = harmonyDegree - baseDegree; // difference relative to same octave

   int minNoteValue = (baseNote + delta) + (12 * mHarmonyOctaveOffsetMin);
   int maxNoteValue = (baseNote + delta) + (12 * mHarmonyOctaveOffsetMax);

   if (mOctaveIncludesPerfect)
   {
      maxNoteValue += 12;
   }

   int noteOut = harmonyNote;
   while (noteOut < minNoteValue) {
      noteOut += 12;
   }
   while (noteOut > maxNoteValue) {
      noteOut -= 12;
   }
   return noteOut;
}


void ArpPlayer::updateArpStepper()
{
   int numNotes = mNoteTable.getNumNotes();
   
   // DEBUG("updateArpStepper: arpOctaveOffsetMin = %d", mArpOctaveOffsetMin);
   // DEBUG("updateArpStepper: arpOctaveOffsetMax = %d", mArpOctaveOffsetMax);

   int numOctaves = (mArpOctaveOffsetMax - mArpOctaveOffsetMin) + 1;
   int minValue = 0;
   int maxValue = (numNotes * numOctaves) - 1; // zero based
   if (mOctaveIncludesPerfect) {
      maxValue++;
   }

   // DEBUG("updateArpStepper, MIN = %d, MAX = %d", minValue, maxValue);
   mArpStepper.setMinMaxValues(minValue, maxValue);
}


void ArpPlayer::updateIntervalSteppers()
{
   // compute the min and max values based on the number of active degrees in the scale 

   int numNotesPerOctave = mTwelveToneScale.getNumNotes();
   if (mTwelveToneScale.getOutlierStrategy() == ScaleOutlierStrategy::OUTLIER_PASS_THRU)
   {
      numNotesPerOctave = 12; 
   }

   // DEBUG("updateIntervalSteppers, harmonyOctaveOffsetMin = %d", mHarmonyOctaveOffsetMin);
   // DEBUG("updateIntervalSteppers, harmonyOctaveOffsetMax = %d", mHarmonyOctaveOffsetMax);

   int span = (mHarmonyOctaveOffsetMax - mHarmonyOctaveOffsetMin) + 1; // TODO: is this +1 correct here?
   int minValue = (mHarmonyOctaveOffsetMin * numNotesPerOctave);
   int maxValue = minValue + (numNotesPerOctave * span);

   if (mHarmonyOctaveOffsetMin >= 0)
   {
      minValue++;
   }

   if (mHarmonyOctaveOffsetMax < 0)
   {
      maxValue--;
   }

   if (mOctaveIncludesPerfect)
   {
      maxValue++;
   }

   // DEBUG("updateIntervalBounders, MIN = %d, MAX = %d", minValue, maxValue);

   mSemitoneIncrementStepper.setMinMaxValues(minValue, maxValue);

   for (int i = 0; i < ARP_PLAYER_NUM_HARMONIES; i++)
   {
      mHarmonyIntervalSteppers[ i ].setMinMaxValues(minValue,maxValue);
   }
}

