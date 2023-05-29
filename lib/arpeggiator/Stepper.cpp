
#include "Stepper.hpp"

// must match order of enums in  StepperAlgo
const char * stepperAlgoNames[StepperAlgo::kNum_STEPPER_ALGOS] = 
{
   "Up",           // STEPPER_ALGO_CYCLE_UP         // reset to first note, set direction upward
   "Down",         // STEPPER_ALGO_CYCLE_DOWN       // reset to last note, set direction downward
   "Reflect",      // STEPPER_ALGO_REFLECT // reverse direction
   "Linger",       // STEPPER_ALGO_REFLECT_LINGER // reverse direction
   "StairsUp",     // STEPPER_ALGO_STAIRS_UP
   "StairsDown",   // STEPPER_ALGO_STAIRS_DOWN
   "StairsUpDown", // STEPPER_ALGO_STAIRS_UP_DOWN
   "Flip",         // STEPPER_ALGO_FLIP             // reset note to nominal index (0), and reverse direction
   "Converge",     // STEPPER_ALGO_CONVERGE
   "Diverge",      // STEPPER_ALGO_DIVERGE
   "Spring",       // STEPPER_ALGO_SPRING
   "PinkyUp",      // STEPPER_ALGO_PINKY_UP
   "PinkyDown",    // STEPPER_ALGO_PINKY_DOWN
   "PinkyUpDown",  // STEPPER_ALGO_PINKY_UP_DOWN
   "ThumbUp",      // STEPPER_ALGO_THUMB_UP
   "ThumbDown",    // STEPPER_ALGO_THUMB_DOWN
   "ThumbUpDown",  // STEPPER_ALGO_THUMB_UP_DOWN
   "Walk",         // STEPPER_ALGO_RANDOM_WALK
   "Random",       // STEPPER_ALGO_RANDOM

   // "Stay",      // STEPPER_ALGO_STAY_AT_EDGE     // hug the edge until a command arrives to go the other direction
   //"Nominal",    // STEPPER_ALGO_RESET_TO_NOMINAL // reset to nominal value
   //"Discard",    // STEPPER_ALGO_DISCARD          // suppress the out of bounds values 
};
