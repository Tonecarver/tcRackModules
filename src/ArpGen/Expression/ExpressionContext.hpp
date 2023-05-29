#ifndef _tc_expression_context_h_
#define _tc_expression_context_h_ 1

//#include "Constants.h" // strmatch 
//#include "PseudoRandom.h"
//#include "SafeString.h"
//#include "GenericList.h"
#include "../../../lib/datastruct/FreePool.hpp"
#include "../../../lib/datastruct/PtrArray.hpp"
#include "../../../lib/io/PrintWriter.hpp"
#include "../../../lib/random/FastRandom.hpp"

#include "ArgumentList.hpp"
#include "FunctionType.hpp"
#include "SymbolTable.hpp"

#include <math.h>
#include <string>
#include <cstdio>

#include <cassert>
//#include "debugtrace.h"


// class Expression;


class ExpressionContext
{
public:
   ExpressionContext()
      : mpLocalParameterValues(0)
   {
   }

   float getValue(std::string name)
   {
      // todo: 
      return 0;
   }

   float getLocalValue(int idx);

   void addSymbol(const char * pName, float value = 0.0)
   {
      mSymbolTable.addSymbol(pName, value);
   }

   float getValue(int symbolId, bool isGlobal);

   float getRandomZeroOne();
   float getRandomRangeFloat(float minValue, float maxValue);
   int   getRandomRangeInteger(int minValue, int maxValue);

   void setLocalParameterValues(ArgumentValuesList * pParameterValues)
   {
      mpLocalParameterValues = pParameterValues;
   }

   float callFunction(FunctionDescription * pFunctionDescription, ArgumentValuesList & args);

private:
   SymbolTable  mSymbolTable;
   FastRandom   mRandomNumberGenerator; 

   ArgumentValuesList * mpLocalParameterValues; // just BORROWING this pointer 
};

#endif // _tc_expression_context_h_