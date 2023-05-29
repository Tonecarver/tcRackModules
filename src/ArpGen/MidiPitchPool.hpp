#ifndef _tc_midi_pitch_pool_h_ 
#define _tc_midi_pitch_pool_h_   1

#include "../../lib/datastruct/FreePool.hpp"
#include "../../lib/datastruct/List.hpp"
#include "../../lib/datastruct/PtrArray.hpp"

#include <cstring>

// #include "debugtrace.h"

// redefine for VCV, simplified 

template <int MAX_NOTES> 
struct MidiPitchPool { 

   int mNotesArrivalOrder[ MAX_NOTES ];
   int mNotesLowToHigh[ MAX_NOTES ];
   int mNumNotes;                        // actual number of notes 

   MidiPitchPool() {
      clear();
   }

   void clear() { 
      mNumNotes = 0;
      memset(mNotesArrivalOrder, 0, sizeof(mNotesArrivalOrder));
      memset(mNotesLowToHigh, 0, sizeof(mNotesLowToHigh));
   }

   bool isEmpty() const { return mNumNotes == 0; }
   bool isFull() const { return mNumNotes == MAX_NOTES; }

   int getNumNotes() const { return mNumNotes; }

   void addNote(int noteNumber) {
      if (isFull()) {
         removeOldestNote();
      }
      insertNote(noteNumber);
      insertNoteSorted(noteNumber);
   }

   const int * getNotesArrivalOrder() const { 
      return mNotesArrivalOrder;
   }

   const int * getNotesLowToHighOrder() const { 
      return mNotesLowToHigh;
   }

protected:
   void removeOldestNote() {
      mNumNotes--; 
   }  

   void insertNote(int noteNumber) {
      shiftNotes(mNotesArrivalOrder, 0);
      mNotesArrivalOrder[0] = noteNumber; 
      mNumNotes++;
   }

   void insertNoteSorted(int noteNumber) {
      int idx = findPositionSorted(noteNumber);
      shiftNotes(mNotesLowToHigh, idx);
      mNotesLowToHigh[idx] = noteNumber;
   }

   int findPositionSorted(int noteNumber) {     
      for (int i = 0; i < mNumNotes; i++) {
         if (noteNumber <= mNotesArrivalOrder[i]) {
            return i;
         }
      }
      return mNumNotes - 1;
   }

   // shift by 1 position away from "[0]" end of array  
   void shiftNotes(int * pArray, int idx) {
      for (int i = mNumNotes - 1; i > idx; i--) { 
         pArray[i] = pArray[i-1];
      }
   }

};


// Modified from tcArpGen

// // enum PitchMode 
// // {
// //    MODE_PRESSED,
// //    MODE_HOLD,
// // //   MODE_ADD_SUBTRACT,
// //    //
// //    kNum_KEY_PRESS_MODES
// // }; 


// class MidiPitch : public ListNode
// {
// public:
//    MidiPitch()
//       : mNoteNumber(0)
//       , mChannelNumber(0)
//       , mNoteVelocity(0)      // TODO: can probably get rid of this 
//       // , mIsHolding(false)
//    {
//    }

//    void set(int channelNumber, int noteNumber, int velocity)
//    {
//       mChannelNumber = channelNumber;
//       mNoteNumber = noteNumber;
//       mNoteVelocity = velocity;
//    }

//    int getChannelNumber() const { return mChannelNumber; } 
//    int getNoteNumber() const { return mNoteNumber; } 
//    int getNoteVelocity() const { return mNoteVelocity; } 
//    // bool isHolding() const { return mIsHolding; } 
//    // void setHolding(bool beHolding) { mIsHolding = beHolding; } 

// private:
//    int mChannelNumber;
//    int mNoteNumber;
//    int mNoteVelocity;
//    // bool mIsHolding;
// };


// // TODO: formerly KeyState
// class MidiPitchPool
// {
// public:
//    MidiPitchPool()
//       : // mMode(KeyPressMode::MODE_PRESSED) 
//         mArrivalOrder(256)   // initial capacity
//       , mLowToHighOrder(256) // initial capacity
//       , mMaxActivePitches(4)
//    {
//    }

//    ~MidiPitchPool()
//    {
//       mArrivalOrder.deleteContents();  // delete OWNED pointers
//       mLowToHighOrder.clear();         // null out SHARED pointers
//    }

//    int getNumNotes() const 
//    { 
//       return mArrivalOrder.getNumMembers(); 
//    } 

//    bool isEmpty() const 
//    { 
//       return mArrivalOrder.isEmpty(); 
//    } 

//    bool isFull() const 
//    { 
//       return mArrivalOrder.getNumMembers() == mMaxActivePitches;
//    } 

//    void clear() 
//    { 
//       retireAllNotes();
//    } 

//    void noteOn(int noteNumber, int channelNumber) {
//       if (isFull()) {
//          retireOldest();
//       }

//       insertNote(noteNumber, channelNumber);
//    }

//    void noteOff(int noteNumber, int channelNumber) {
//       removeNote(...)
//       // TODO: 
//    }




//    // void addMidiEvent(LiveMidiEvent * pEvent)
//    // {
//    //    if (pEvent->mMidiMessage.isNoteOn())
//    //    {
//    //       if (pEvent->mMidiMessage.getNoteVelocity() > 0)
//    //       {
//    //          addNoteOn(pEvent);
//    //       }
//    //       else
//    //       {
//    //          addNoteOff(pEvent);
//    //       }
//    //    }
//    //    else if (pEvent->mMidiMessage.isNoteOff())
//    //    {
//    //       addNoteOff(pEvent);
//    //    }
//    //    else
//    //    {
//    //       MYTRACE(( "Unexpected event type\n" ));
//    //    }
//    // }

//    // KeyPressMode getMode()
//    // {
//    //    return mMode;
//    // }

//    // void setMode(KeyPressMode mode)
//    // {
//    //    if (mode != mMode)
//    //    {
//    //       mMode = mode; 
//    //       if (mMode == MODE_PRESSED) // not a mode that enforces hold 
//    //       {
//    //          removeAllNotesInHolding();
//    //       }
//    //    }
//    // }

//    PtrArray<MidiPitch> & getArrivalOrder()
//    {
//       return mArrivalOrder;
//    }
   
//    PtrArray<MidiPitch> & getLowToHighOrder()
//    {
//       return mLowToHighOrder;
//    }

//    // void addNoteOn(LiveMidiEvent * pEvent)
//    // {
//    //    MidiPitch * pExistingKeyPress = getExistingKeyPress(pEvent);
//    //    if (pExistingKeyPress != 0)
//    //    {
//    //       if (mMode == MODE_HOLD)
//    //       {
//    //          pExistingKeyPress->setHolding(false); // note is actually pressed
//    //          return;
//    //       }

//    //       if (mMode == MODE_ADD_SUBTRACT)
//    //       {
//    //          removeNote(pExistingKeyPress); // in Add/SUB mode, pressing key for active note turns note off 
//    //          return;
//    //       }
//    //    }

//    //    insertNote(pEvent);
//    // }

//    // void addNoteOff(LiveMidiEvent * pEvent)
//    // {
//    //    Pitch * pExistingKeyPress = getExistingKeyPress(pEvent);
//    //    if (pExistingKeyPress != 0)
//    //    {
//    //       if ((mMode == MODE_HOLD) || (mMode == MODE_ADD_SUBTRACT))
//    //       {
//    //          pExistingKeyPress->setHolding(true);
//    //       }
//    //       else // mode = presses
//    //       {
//    //          removeNote(pExistingKeyPress);
//    //       }
//    //    }
//    // }

// private:
//    PtrArray<MidiPitch> mArrivalOrder;   // OWNS pointers
//    PtrArray<MidiPitch> mLowToHighOrder; // SHARES pointers 
//    FreePool<MidiPitch> mFreePool;
//    int mMaxActivePitches;               // maximum number of pitches in the pool 

//    // KeyPressMode mMode; 

//    MidiPitch * getExistingMidiPitch(int noteNumber, int channelNumber)
//    {
//       PtrArrayIterator<MidiPitch> iter(mArrivalOrder);
//       while (iter.hasMore())
//       {
//          MidiPitch * pExistingMidiPitch = iter.getNext();
//          if (pExistingMidiPitch->getNoteNumber() == noteNumber && 
//              pExistingMidiPitch->getChannelNumber() == channelNumber)
//          {
//             return pExistingMidiPitch;
//          }
//       }
//       return 0; // no match 
//    }

//    void insertNote(int noteNumber, int channel)
//    {
//       MidiPitch * pMidiPitch = mFreePool.allocate();
//       pMidiPitch->set(channel, noteNumber, 255);

//       mArrivalOrder.append(pMidiPitch);

//       int idx = findInsertPosition(pMidiPitch);
//       if (idx < 0)
//       {
//          mLowToHighOrder.append(pMidiPitch);
//       }
//       else
//       {
//          mLowToHighOrder.insertAt(idx, pMidiPitch);
//       }
//    }

//    void removeNote(MidiPitch * pMidiPitch)
//    {
//       mArrivalOrder.remove(pMidiPitch);
//       mLowToHighOrder.remove(pMidiPitch);
//       mFreePool.retire(pMidiPitch);
//    }

//    int findInsertPosition(MidiPitch * pMidiPitch)
//    {
//       int numMembers = mLowToHighOrder.getNumMembers();
//       for (int i = 0; i < numMembers; i++)
//       {
//          MidiPitch * pExistingKeyPress = mLowToHighOrder.getAt(i);
//          if (pExistingKeyPress->getNoteNumber() > pMidiPitch->getNoteNumber())
//          {
//             return i;
//          }
         
//          if (pExistingKeyPress->getNoteNumber() == pMidiPitch->getNoteNumber())
//          {
//             if (pExistingKeyPress->getChannelNumber() == pMidiPitch->getChannelNumber())
//             {
//                return i;
//             }
//          }
//       }
//       return -1; // not found
//    }

//    void retireAllNotes()
//    {
//       // Retire OWNED pointers
//       PtrArrayIterator<MidiPitch> iter(mArrivalOrder);
//       while (iter.hasMore())
//       {
//          MidiPitch * pMidiPitch = iter.getNext();
//          mFreePool.retire(pMidiPitch);
//       }
//       mArrivalOrder.clear();

//       // Null out SHARED pointers
//       mLowToHighOrder.clear();
//    }

//    // void removeAllNotesInHolding()
//    // {
//    //    // search backwards so that index points to array area 
//    //    // that would not be altered when an entry is removed
//    //    for (int i = mArrivalOrder.getNumMembers() - 1; i >= 0; i--)
//    //    {
//    //       MidiPitch * pMidiPitch = mArrivalOrder.getAt(i); 
//    //       if (pMidiPitch->isHolding())
//    //       {
//    //          removeNote(pMidiPitch);
//    //       }
//    //    }
//    // }

// };

#endif // _tc_midi_pitch_pool_h_ 
