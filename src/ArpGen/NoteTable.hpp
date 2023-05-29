#pragma once
#ifndef _tc_note_table_h_
#define _tc_note_table_h_  1

// #include "logger.hpp" // for DEBUG 
#include "../../lib/datastruct/List.hpp"
#include "../../lib/datastruct/PtrArray.hpp"
#include "../../lib/datastruct/FreePool.hpp"

class ActiveNote : public ListNode
{
public:
   ActiveNote()
      : mChannelNumber(0)    // TODO: remove channel number 
      , mNoteNumber(0)
   {
   }

   void set(int channelNumber, int noteNumber)
   {
      mChannelNumber = channelNumber;
      mNoteNumber = noteNumber;
   }

   int getChannelNumber() const { return mChannelNumber; } 
   int getNoteNumber() const { return mNoteNumber; } 

   void setNoteNumber(int noteNumber) { 
      mNoteNumber = noteNumber;
   }

private:
   int mChannelNumber;
   int mNoteNumber;
};


class NoteTable
{
public:
   NoteTable()
      : mArrivalOrder(16)   // initial capacity  // TODO: const, should be max_polyphony ? 
      , mLowToHighOrder(16) // initial capacity
      , mHighToLowOrder(16) // initial capacity
   {
      clear();
   }

   ~NoteTable()
   {
      mArrivalOrder.clear(); 
      mLowToHighOrder.clear();         // null out SHARED pointers
      mHighToLowOrder.clear();         // null out SHARED pointers
   }

   // void updateKeyState(List<LiveMidiEvent> & queue)
   // {
   //    ListIterator<LiveMidiEvent> iter(queue);
   //    while (iter.HasMore())
   //    {
   //       LiveMidiEvent * pLiveMidiEvent = iter.GetNext();
   //       addMidiEvent(pLiveMidiEvent);
   //    }
   // }

	void setNotes(int numNotes, int * pNotes) {
      bool updateRequired = false;
      if (numNotes != mArrivalOrder.getNumMembers()) { 
         updateRequired = true;
      } else {
         for (int i = 0; i < numNotes; i++) {
            if (mArrivalOrder.getAt(i)->getNoteNumber() != pNotes[i]) {
               updateRequired = true;
               break;
            }
         }
      }

      if (updateRequired) {
         clear();
         for (int i = 0; i < numNotes; i++) {
            insertNote(pNotes[i]);
         }
         for (int i = 0; i < numNotes; i++) {
            mHighToLowOrder.append( mLowToHighOrder.getAt(numNotes - i - 1) );
         }
      }

      // if (updateRequired) {
      //    DEBUG("Set Notes: ");
      //    for (int i = 0; i < mArrivalOrder.getNumMembers(); i++) { 
      //       DEBUG(" Arrival[%d] = %d", i, mArrivalOrder.getAt(i)->getNoteNumber());
      //    }
      //    for (int i = 0; i < mLowToHighOrder.getNumMembers(); i++) { 
      //       DEBUG(" LowToHigh[%d] = %d", i, mLowToHighOrder.getAt(i)->getNoteNumber());
      //    }
      //    for (int i = 0; i < mHighToLowOrder.getNumMembers(); i++) { 
      //       DEBUG(" HighToLow[%d] = %d", i, mHighToLowOrder.getAt(i)->getNoteNumber());
      //    }
      // }
   }

   int getNumNotes() const 
   { 
      return mArrivalOrder.getNumMembers(); 
   } 

   bool isEmpty() const 
   { 
      return mArrivalOrder.isEmpty(); 
   } 

   void clear() {

      // Retire the OWNED pointers 
      for (int i = 0; i < mArrivalOrder.getNumMembers(); i++) {
         ActiveNote * pActiveNote = mArrivalOrder.getAt(i);
         mNotePool.retire(pActiveNote);
      }

      mArrivalOrder.clear();
      mLowToHighOrder.clear();
      mHighToLowOrder.clear();
   }
   

   PtrArray<ActiveNote> & getNotesArrivalOrder()
   {
      return mArrivalOrder;
   }
   
   PtrArray<ActiveNote> & getNotesLowToHighOrder()
   {
      return mLowToHighOrder;
   }

   PtrArray<ActiveNote> & getNotesHighToLowOrder()
   {
      return mHighToLowOrder;
   }

private:
   PtrArray<ActiveNote> mArrivalOrder;   // OWNS pointers
   PtrArray<ActiveNote> mLowToHighOrder; // SHARES pointers 
   PtrArray<ActiveNote> mHighToLowOrder; // SHARES pointers 
   FreePool<ActiveNote> mNotePool;

   void insertNote(int noteNumber) {
    // DEBUG("NoteTable: insert note, noteNUmber %d", noteNumber);

      ActiveNote * pActiveNote = mNotePool.allocate();
      pActiveNote->set(0, noteNumber); // if using channel as index into local array, then active note does not need channel

      // Record new note 
      mArrivalOrder.append(pActiveNote);

      int idx = findInsertPosition(pActiveNote);
      if (idx < 0)
      {
         mLowToHighOrder.append(pActiveNote);
      }
      else
      {
         mLowToHighOrder.insertAt(idx, pActiveNote);
      }
   }

   int findInsertPosition(ActiveNote * pActiveNote)
   {
      int numMembers = mLowToHighOrder.getNumMembers();
      for (int i = 0; i < numMembers; i++)
      {
         ActiveNote * pExistingNote = mLowToHighOrder.getAt(i);
         if (pExistingNote->getNoteNumber() > pActiveNote->getNoteNumber())
         {
            return i;
         }
         
         if (pExistingNote->getNoteNumber() == pActiveNote->getNoteNumber())
         {
            if (pExistingNote->getChannelNumber() == pActiveNote->getChannelNumber())
            {
               return i;
            }
         }
      }
      return -1; // not found
   }

};

#endif // _tc_note_table_h_