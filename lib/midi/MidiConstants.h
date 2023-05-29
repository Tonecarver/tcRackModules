#pragma once
#ifndef _midi_constants_h
#define _midi_constants_h  1

#define NUM_MIDI_NOTES      (128)
#define NUM_MIDI_CHANNELS    (16)
#define NUM_MIDI_CC_VALUES  (128)
#define NUM_MIDI_VELOCITIES (128)
#define IS_VALID_MIDI_BYTE(val)                  ((0 <= val) && (val <= 127))                 // 7-bits
#define IS_VALID_MIDI_CHANNEL_NUMBER(chan)       ((0 <= chan) && (chan < NUM_MIDI_CHANNELS))  // zero based
#define IS_VALID_MIDI_NOTE_NUMBER(note)          ((0 <= note) && (note < NUM_MIDI_NOTES))     // zero based 
#define IS_VALID_MIDI_VELOCITY(val)              IS_VALID_MIDI_BYTE(val)
#define IS_VALID_MIDI_CHANNEL_PRESSURE(press)    IS_VALID_MIDI_BYTE(press)
#define IS_VALID_MIDI_AFTERTOUCH_PRESSURE(press) IS_VALID_MIDI_BYTE(press)
#define IS_VALID_MIDI_CC_NUMBER(val)             IS_VALID_MIDI_BYTE(val)
#define IS_VALID_MIDI_CC_VALUE(val)              IS_VALID_MIDI_BYTE(val)
#define IS_VALID_MIDI_PROGRAM_NUMBER(val)        IS_VALID_MIDI_BYTE(val)
#define IS_VALID_MIDI_PITCH_BEND(val)            ((0 <= val) && (val < 0x4000)) // 14 bits 

#define MIDI_COMMAND_NOTE_OFF              (0x80)
#define MIDI_COMMAND_NOTE_ON               (0x90)
#define MIDI_COMMAND_AFTERTOUCH            (0xA0)
#define MIDI_COMMAND_CONTINUOUS_CONTROLLER (0xB0)
#define MIDI_COMMAND_PROGRAM_CHANGE        (0xC0)
#define MIDI_COMMAND_CHANNEL_PRESSURE      (0xD0)
#define MIDI_COMMAND_PITCH_BEND            (0xE0)
#define MIDI_COMMAND_SYSEX                 (0xF0)

typedef signed long tickOffset_t;  // 31-bits 
//typedef signed long long tickOffset_t;  // 63-bits 

enum EMidiChunkId
{
   MIDI_CHUNK_MThd = 0x4d546864, // 'MThd' 
   MIDI_CHUNK_MTrk = 0x4d54726B, // 'MTrk' 
};



#endif // _midi_constants_h