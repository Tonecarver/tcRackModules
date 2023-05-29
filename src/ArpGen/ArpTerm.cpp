#include "ArpTerm.hpp"

//#include "Constants.h" // strmatch 

#include <cstring>


// order must match enum ArpTermType 
const char * arpTermTypeNames[ ArpTermType::kNUM_TERM_TYPES ] = 
{
   "Note",        // TERM_NOTE, 

   "Rest",        // TERM_REST,

   // "Chord",       // TERM_CHORD,
   
   // "Gliss",       // TERM_GLISS,

   // "GlissSpeedUp",     // TERM_GLISS_SPEED_UP, 
   // "GlissSpeedDown",   // TERM_GLISS_SPEED_DOWN, 
   // "GlissSpeedRand",   // TERM_GLISS_SPEED_RANDOM, 
   // "GlissSpeedNom",    // TERM_GLISS_SPEED_NOMINAL,
   // "GlissSpeedAssign", // TERM_GLISS_SPEED_ASSIGN, 

   // "GlissDirUp",     // TERM_GLISS_DIR_UP, 
   // "GlissDirDown",   // TERM_GLISS_DIR_DOWN, 
   // "GlissDirRand",   // TERM_GLISS_DIR_RANDOM, 
   // "GlissDirNom",    // TERM_GLISS_DIR_NOMINAL,
   // "GlissDirAssign", // TERM_GLISS_DIR_ASSIGN, 

   "NoteUp",      // TERM_NOTE_UP, 
   "NoteDown",    // TERM_NOTE_DOWN, 
   "NoteRand",    // TERM_NOTE_RANDOM, 
   "NoteNom",     // TERM_NOTE_NOMINAL, // 0  
   "NoteAssign",  // TERM_NOTE_ASSIGN, 

   // "VelUp",       // TERM_VELOCITY_UP,
   // "VelDown",     // TERM_VELOCITY_DOWN,
   // "VelRand",     // TERM_VELOCITY_RANDOM,
   // "VelNom",      // TERM_VELOCITY_NOMINAL,
   // "VelAssign",   // TERM_VELOCITY_ASSIGN,

   "SemiUp",      // TERM_SEMITONE_UP,
   "SemiDown",    // TERM_SEMITONE_DOWN,
   "SemiRand",    // TERM_SEMITONE_RANDOM,
   "SemiNom",     // TERM_SEMITONE_NOMINAL,
   "SemiAssign",  // TERM_SEMITONE_ASSIGN,

   "OctUp",       // TERM_OCTAVE_UP,
   "OctDown",     // TERM_OCTAVE_DOWN,
   "OctRand",     // TERM_OCTAVE_RANDOM,
   "OctNom",      // TERM_OCTAVE_NOMINAL,
   "OctAssign",   // TERM_OCTAVE_ASSIGN,

#if ARP_PLAYER_NUM_HARMONIES >=1
   "Harm1Up",     // TERM_HARMONY_1_UP,
   "Harm1Down",   // TERM_HARMONY_1_DOWN,
   "Harm1Rand",   // TERM_HARMONY_1_RANDOM,
   "Harm1Nom",    // TERM_HARMONY_1_NOMINAL, // off 
   "Harm1Assign", // TERM_HARMONY_1_ASSIGN,
#endif
#if ARP_PLAYER_NUM_HARMONIES >=2
   "Harm2Up",     // TERM_HARMONY_2_UP,
   "Harm2Down",   // TERM_HARMONY_2_DOWN,
   "Harm2Rand",   // TERM_HARMONY_2_RANDOM,
   "Harm2Nom",    // TERM_HARMONY_2_NOMINAL, // off 
   "Harm2Assign", // TERM_HARMONY_2_ASSIGN,
#endif
#if ARP_PLAYER_NUM_HARMONIES >=3
   "Harm3Up",     // TERM_HARMONY_3_UP,
   "Harm3Down",   // TERM_HARMONY_3_DOWN,
   "Harm3Rand",   // TERM_HARMONY_3_RANDOM,
   "Harm3Nom",    // TERM_HARMONY_3_NOMINAL, // off 
   "Harm3Assign", // TERM_HARMONY_3_ASSIGN,
#endif
#if ARP_PLAYER_NUM_HARMONIES >=4
   "Harm4Up",     // TERM_HARMONY_4_UP,
   "Harm4Down",   // TERM_HARMONY_4_DOWN,
   "Harm4Rand",   // TERM_HARMONY_4_RANDOM,
   "Harm4Nom",    // TERM_HARMONY_4_NOMINAL, // off 
   "Harm4Assign", // TERM_HARMONY_4_ASSIGN,
#endif

   "UpDownInvert", // TERM_UP_DOWN_INVERT,   // invert sense of +/-
   "UpDownNom",    // restore +/- to natural meaning

   "Push",        // TERM_PUSH,
   "Pop",         // TERM_POP,
   //"GroupBegin",  // TERM_GROUP_BEGIN,
   //"GroupEnd",    // TERM_GROUP_END,
   "If",          // TERM_IF,
   "Else",        // TERM_ELSE,
   "ExprBegin",   // TERM_EXPRESSION_BEGIN,
   "ExprEnd",     // TERM_EXPRESSION_END,
   "Function",    // TERM_FUNCTION,
   "Probability", // TERM_PROBABILITY, 
   "Condition",   // TERM_CONDITION,
   "Rewritable",  // TERM_REWRITABLE, // NON-Note rewritable term (to be expanded during expansion)
};


const PlayableTerm PlayableTermTable[] = 
{
   { "N", ArpTermType::TERM_NOTE }, 
   // { "C", ArpTermType::TERM_CHORD }, 
//   { "G", ArpTermType::TERM_GLISS }, 
   { 0,   ArpTermType::kNUM_TERM_TYPES } // end of table marker 
};

const PlayableTerm * getPlayableTermDescription(const char * pName)
{
   const PlayableTerm * pDescription = PlayableTermTable;
   while (pDescription->isValidEntry())
   {
      if (strcmp(pDescription->mpName, pName) == 0)
      {
         return pDescription;
      }
      pDescription++; 
   }
   return 0; // not found;
}

