#include "FunctionType.hpp"

FunctionDescription functionDescriptionTable[kNUM_FUNC_TYPES] =
{
   { FUNC_INCR, "incr", 1 },
   { FUNC_DECR, "decr", 1 },
   { FUNC_ZERO, "zero", 1 },
   { FUNC_PICK, "pick", 3 },  // pick(expr, true_part, false_part )
   { FUNC_PROB, "prob", 1 },  // prob(0..1)   true if random value < argvalue 
   { FUNC_RAND, "rand", 0 },  // rand()       random number 0..1
};

FunctionDescription * getFunctionDescriptionByName(std::string name)
{
   for (int i = 0; i < kNUM_FUNC_TYPES; i++) {
      FunctionDescription * pFunctionDescription = &functionDescriptionTable[i];
      if (pFunctionDescription->mName == name)
      {
         return pFunctionDescription;
      }
   }
   return 0; // not found 
}

FunctionDescription * getFunctionDescriptionByType(FunctionType functionType)
{
   return &functionDescriptionTable[ int(functionType) ];
}

