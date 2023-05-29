#include "ExpressionContext.hpp"

//#include "debugtrace.h"


float ExpressionContext::getLocalValue(int idx)
{
   if (mpLocalParameterValues == 0)
   {
      return 0.0;
   }
   // todo: valid index check? done by Local Param List
   return mpLocalParameterValues->getValueAt(idx); 
}


////////////////////////////////////////////////////////////////////////////////
//// prepare to call context that takes formal parameters
//// push parameter values onto the local stack 
//void ExpressionContext::pushParameterValues( List<Expression> & parameterValues )
//{
//   MYTRACE(( " ExpressionContext.pushParameterValues" ));
//
//   InvocationFrame * pFrame = mFrameFreePool.allocate(); 
//   pFrame->initialize();
//
//   ListIterator<Expression> iter(parameterValues);
//   while (iter.HasMore())
//   {
//      pFrame->push( iter.GetNext()->getValue(this) );
//   }
//
//   mFrameStack.push(pFrame);
//}
//
//// pop the local context from the local stack
//void ExpressionContext::popParameterValues()
//{
//   MYTRACE(( " ExpressionContext.popParameterValues" ));
//
//   InvocationFrame * pFrame = mFrameStack.pop();
//   mFrameFreePool.retire(pFrame);
//}

float ExpressionContext::getValue(int symbolId, bool isGlobal)
{
   if (isGlobal)
   {
      Symbol * pSymbol = mSymbolTable.getSymbolById(symbolId);
      if (pSymbol != 0)
      {
         return pSymbol->getValue();
      }
   }
   else
   {
      //InvocationFrame * pFrame = mFrameStack.top();
      //if (pFrame != 0)
      //{
      //   if (pFrame->isValidIndex(symbolId))
      //   {
      //      return pFrame->getAt(symbolId);
      //   }
      //}
   }
   return 0.0;
}

float ExpressionContext::getRandomZeroOne()
{
   return mRandomNumberGenerator.generateZeroToOne();
}

float ExpressionContext::getRandomRangeFloat(float minValue, float maxValue)
{
   return minValue + (mRandomNumberGenerator.generateZeroToOne() * (maxValue - minValue));
}

int ExpressionContext::getRandomRangeInteger(int minValue, int maxValue)
{
   // TODO: use fast _floatToInt()
   return minValue + int((mRandomNumberGenerator.generateZeroToOne() * float(maxValue - minValue)) + 0.5);
}

float ExpressionContext::callFunction(FunctionDescription * pFunctionDescription, ArgumentValuesList & args)
{
   // DEBUG( "CALL Function" ); 

   // assert ptrs not null 
   // check desc=>numArgs == args.numArgs 

   float value = 0.f;

   switch (pFunctionDescription->mFunctionType)
   {
   case FunctionType::FUNC_DECR:
      // todo decrement the symbol referred to in the arg list 
         //value = args.getValueAt(0) - 1.0;
      break;
   case FunctionType::FUNC_INCR:
      // todo value = args.getValueAt(0) + 1.0;
      break;
   case FunctionType::FUNC_PICK:
      // todo 
      break;
   case FunctionType::FUNC_ZERO:
      // todo 
      break;
   case FunctionType::FUNC_PROB:
      if (getRandomZeroOne() < args.getValueAt(0))
      {
         value = 1.0;
      }
      else
      {
         value = 0.0;
      }
      break;
   case FunctionType::FUNC_RAND:
      value = getRandomZeroOne();
      break;
   default:
      // value = 0.0;
      break;
   }

   return value;
}

