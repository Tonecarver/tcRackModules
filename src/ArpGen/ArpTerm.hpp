#ifndef _tc_arp_term_h_
#define _tc_arp_term_h_  1

#include "ArpPlayerConstants.hpp"

enum ArpTermType
{
   TERM_NOTE, // note play 

   TERM_REST,

   // TERM_CHORD,
   //TERM_GLISS,  // strum 
   //TERM_GLISS_SPEED_UP, 
   //TERM_GLISS_SPEED_DOWN, 
   //TERM_GLISS_SPEED_RANDOM, 
   //TERM_GLISS_SPEED_NOMINAL,
   //TERM_GLISS_SPEED_ASSIGN, 

   //TERM_GLISS_DIR_UP, 
   //TERM_GLISS_DIR_DOWN, 
   //TERM_GLISS_DIR_RANDOM, 
   //TERM_GLISS_DIR_NOMINAL,
   //TERM_GLISS_DIR_ASSIGN, 

   TERM_NOTE_UP, 
   TERM_NOTE_DOWN, 
   TERM_NOTE_RANDOM, 
   TERM_NOTE_NOMINAL, // 0  
   TERM_NOTE_ASSIGN, 

   //TERM_VELOCITY_UP,
   //TERM_VELOCITY_DOWN,
   //TERM_VELOCITY_RANDOM,
   //TERM_VELOCITY_NOMINAL,
   //TERM_VELOCITY_ASSIGN,

   TERM_SEMITONE_UP,
   TERM_SEMITONE_DOWN,
   TERM_SEMITONE_RANDOM,
   TERM_SEMITONE_NOMINAL,
   TERM_SEMITONE_ASSIGN,

   TERM_OCTAVE_UP,
   TERM_OCTAVE_DOWN,
   TERM_OCTAVE_RANDOM,
   TERM_OCTAVE_NOMINAL,
   TERM_OCTAVE_ASSIGN,

#if ARP_PLAYER_NUM_HARMONIES >=1
   TERM_HARMONY_1_UP,
   TERM_HARMONY_1_DOWN,
   TERM_HARMONY_1_RANDOM,
   TERM_HARMONY_1_NOMINAL, // off 
   TERM_HARMONY_1_ASSIGN, 
#endif 
#if ARP_PLAYER_NUM_HARMONIES >=2
   TERM_HARMONY_2_UP,
   TERM_HARMONY_2_DOWN,
   TERM_HARMONY_2_RANDOM,
   TERM_HARMONY_2_NOMINAL, // off 
   TERM_HARMONY_2_ASSIGN, 
#endif 
#if ARP_PLAYER_NUM_HARMONIES >=3
   TERM_HARMONY_3_UP,
   TERM_HARMONY_3_DOWN,
   TERM_HARMONY_3_RANDOM,
   TERM_HARMONY_3_NOMINAL, // off 
   TERM_HARMONY_3_ASSIGN, 
#endif 
#if ARP_PLAYER_NUM_HARMONIES >=4
   TERM_HARMONY_4_UP,
   TERM_HARMONY_4_DOWN,
   TERM_HARMONY_4_RANDOM,
   TERM_HARMONY_4_NOMINAL, // off 
   TERM_HARMONY_4_ASSIGN, 
#endif 

   TERM_UP_DOWN_INVERT,   // invert sense of +/-
   TERM_UP_DOWN_NOMINAL,  // restore +/- to natural meaning

   TERM_PUSH,
   TERM_POP,

   //TERM_GROUP_BEGIN,
   //TERM_GROUP_END,
   TERM_IF,
   TERM_ELSE,
   TERM_EXPRESSION_BEGIN,
   TERM_EXPRESSION_END,
   TERM_FUNCTION,
   TERM_PROBABILITY,  // todo: remove
   TERM_CONDITION,    // todo: remove

   // .. 
   TERM_REWRITABLE, // NON-Note rewritable term (to be expanded during expansion)
   // .. 
   kNUM_TERM_TYPES
};
extern const char * arpTermTypeNames[ ArpTermType::kNUM_TERM_TYPES ]; 

struct PlayableTerm
{
public:
   const char * mpName;
   ArpTermType  mTermType;

   bool isEndOfTable() const { return mpName == 0; } 
   bool isValidEntry() const { return mpName != 0; } 
};

extern const PlayableTerm * getPlayableTermDescription(const char * pName);

#endif // _tc_arp_term_h_