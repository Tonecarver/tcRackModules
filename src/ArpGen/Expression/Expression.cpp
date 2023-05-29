// Expression.cpp

#include "../../plugin.hpp"
#include "Expression.hpp"
#include "ExpressionContext.hpp"

//#include "ArpPlayer.h"

#include <math.h>
#include <cstdio>
#include <cstring>

//#include "debugtrace.h"
#include <cassert>


#define DOUBLE_EQUALITY_TOLERANCE float(0.000001)
inline bool approxDoubleEquals(float d1, float d2)
{
   return ((d2 - DOUBLE_EQUALITY_TOLERANCE) <= d1) && (d1 <= (d2 + DOUBLE_EQUALITY_TOLERANCE));
}




// -----------------------------------------------
// Expression

bool Expression::isTrue(ExpressionContext * pContext)
{
   return (getValue(pContext) != 0.0);
}

bool Expression::isFalse(ExpressionContext * pContext) 
{ 
   return ( ! isTrue(pContext) ); 
}

Expression * Expression::unpack(ByteStreamReader * pReader)
{
   assert(pReader != 0);

   char ch;
   pReader->readChar(ch);
   // todo; check return code 

   ExpressionNodeType type = (ExpressionNodeType) ch; 

   int intValue;
   float floatValue;
   std::string stringValue;

   Expression * pLhs;
   Expression * pRhs;
   Expression * pExpression;

   switch (type)
   {
   case EXPRESSION_NODE_EMPTY:
      return new EmptyExpression();

   case EXPRESSION_NODE_PARENTHESIZED_EXPR:
      pExpression = unpack(pReader);
      return new ParenthesizedExpression(pExpression);

   case EXPRESSION_NODE_LOGICAL_NOT:
      pExpression = unpack(pReader);
      return new LogicalNotExpression(pExpression);

   case EXPRESSION_NODE_LOGICAL_AND:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new LogicalAndExpression(pLhs, pRhs);

   case EXPRESSION_NODE_LOGICAL_OR:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new LogicalOrExpression(pLhs, pRhs);

   case EXPRESSION_NODE_CONSTANT:
      pReader->readFloat(floatValue);
      return new ConstantValue(floatValue);

   case EXPRESSION_NODE_LOCAL_VARIABLE:
      pReader->readString(stringValue);
      pReader->readInt(intValue);  // local variable index, index of the argument to the PRODUCTION that owns the term 
      return new LocalVariableValue(stringValue, intValue);

   case EXPRESSION_NODE_GLOBAL_VARIABLE:
      pReader->readString(stringValue);
      return new GlobalVariableValue(stringValue);

   case EXPRESSION_NODE_ADDITION:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new AdditionExpression(pLhs, pRhs);

   case EXPRESSION_NODE_SUBTRACTION:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new SubtractionExpression(pLhs, pRhs);

   case EXPRESSION_NODE_MULTIPLY:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new MultiplyExpression(pLhs, pRhs);

   case EXPRESSION_NODE_DIVIDE:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new DivideExpression(pLhs, pRhs);

   case EXPRESSION_NODE_MODULO:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new ModuloExpression(pLhs, pRhs);

   case EXPRESSION_NODE_GREATER_THAN:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new GreaterThanExpression(pLhs, pRhs);

   case EXPRESSION_NODE_GREATER_THAN_OR_EQUAL:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new GreaterThanOrEqualExpression(pLhs, pRhs);

   case EXPRESSION_NODE_EQUALS:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new EqualsExpression(pLhs, pRhs);

   case EXPRESSION_NODE_NOT_EQUALS:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new NotEqualsExpression(pLhs, pRhs);

   case EXPRESSION_NODE_LESS_THAN:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new LessThanExpression(pLhs, pRhs);

   case EXPRESSION_NODE_LESS_THAN_OR_EQUAL:
      pLhs = unpack(pReader);
      pRhs = unpack(pReader);
      return new LessThanOrEqualExpression(pLhs, pRhs);

   case EXPRESSION_NODE_TERNARY:
      pExpression = unpack(pReader); // condition 
      pLhs = unpack(pReader);        // true part
      pRhs = unpack(pReader);        // false part
      return new TernaryExpression(pExpression, pLhs, pRhs);

   case EXPRESSION_NODE_FUNCTION:
      {
         pReader->readString(stringValue);
  
         FunctionDescription * pFunctionDescription = getFunctionDescriptionByName(stringValue);
         if (pFunctionDescription == 0)
         {
            // todo log
            return 0;
         }

         ArgumentExpressionList * pArgumentExpressionList = new ArgumentExpressionList();
         int numArgumentExpressions; 
         pReader->readInt(numArgumentExpressions);
         for (int i = 0; i < numArgumentExpressions; i++)
         {
            pExpression = Expression::unpack(pReader);
            if (pArgumentExpressionList->isFull())
            {
               // todo log
               delete pExpression;
            }
            else
            {
               pArgumentExpressionList->addArgument(pExpression);
            }
         }
         return new FunctionExpression(pFunctionDescription, pArgumentExpressionList);
      }

   default:
      DEBUG( "!! Unexpected Epxression type: %d   char=%c", type, (char)type );
      break;
   };

   DEBUG( "!! Error unpacking expression .. returning 0" );
   return 0; 
}

// -----------------------------------------------
// Empty 

EmptyExpression::EmptyExpression()
{
   DEBUG(( "Creating EMPTY expression" ));
}

float EmptyExpression::getValue(ExpressionContext * pContext)
{
   return 0.0;
}

void EmptyExpression::print(PrintWriter & writer)
{
   //writer.print(" <empty> ");
   writer.print(" ");
}

ExpressionNodeType EmptyExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_EMPTY;
}

void EmptyExpression::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
}

// -----------------------------------------------
// Unary 

UnaryExpression::UnaryExpression(Expression * pExpression)
: mpExpression(pExpression)
{
   if (pExpression == 0)
   {
      mpExpression = new EmptyExpression();
   }
}

UnaryExpression::~UnaryExpression()
{
   delete mpExpression;
}

void UnaryExpression::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
   mpExpression->serialize(pWriter);
}


// -----------------------------------------------
// Binary 

BinaryExpression::BinaryExpression(Expression * pLhs, Expression * pRhs)
   : mpLhs(pLhs)
   , mpRhs(pRhs)
{
   if (pLhs == 0)
   {
      mpLhs = new EmptyExpression();
   }
   if (pRhs == 0)
   {
      mpLhs = new EmptyExpression();
   }
}

BinaryExpression::~BinaryExpression()
{
   delete mpLhs;
   delete mpRhs;
}

void BinaryExpression::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
   mpLhs->serialize(pWriter);
   mpRhs->serialize(pWriter);
}


// -----------------------------------------------
// Unary Logical

UnaryLogicalExpression::UnaryLogicalExpression(Expression * pExpression)
   : UnaryExpression(pExpression)
{
}

float UnaryLogicalExpression::getValue(ExpressionContext * pContext)
{
   return isTrue(pContext) ? 1.0 : 0.0;
}


// -----------------------------------------------
// Binary Logical

BinaryLogicalExpression::BinaryLogicalExpression(Expression * pLhs, Expression * pRhs)
   : BinaryExpression(pLhs, pRhs)
{
}

float BinaryLogicalExpression::getValue(ExpressionContext * pContext)
{
   return isTrue(pContext) ? 1.0 : 0.0;
}

// ============================================================
// Concrete Expression classes 
// ============================================================

// -----------------------------------------------
// Parenthesized 

ParenthesizedExpression::ParenthesizedExpression(Expression * pExpression)
   : UnaryExpression(pExpression)
{
}

float ParenthesizedExpression::getValue(ExpressionContext * pContext)
{
   return getExpression()->getValue(pContext);
}

void ParenthesizedExpression::print(PrintWriter & writer)
{
   writer.print(" ( ");
   getExpression()->print(writer);
   writer.print(" ) ");
}

ExpressionNodeType ParenthesizedExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_PARENTHESIZED_EXPR;
}

// -----------------------------------------------
// Logical Not

LogicalNotExpression::LogicalNotExpression(Expression * pExpression)
   : UnaryLogicalExpression(pExpression)
{
}

bool LogicalNotExpression::isTrue(ExpressionContext * pContext) 
{
   return (getExpression()->isFalse(pContext));
}

void LogicalNotExpression::print(PrintWriter & writer)
{
   writer.print(" ! ");
   getExpression()->print(writer);
}

ExpressionNodeType LogicalNotExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_LOGICAL_NOT;
}



// -----------------------------------------------
// Logical And

LogicalAndExpression::LogicalAndExpression(Expression * pLhs, Expression * pRhs)
   : BinaryLogicalExpression(pLhs, pRhs)
{
}

bool LogicalAndExpression::isTrue(ExpressionContext * pContext) 
{
   return getLhs()->isTrue(pContext) && getRhs()->isTrue(pContext);
}

void LogicalAndExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" && ");
   getRhs()->print(writer);
}

ExpressionNodeType LogicalAndExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_LOGICAL_AND;
}


// -----------------------------------------------
// Logical Or

LogicalOrExpression::LogicalOrExpression(Expression * pLhs, Expression * pRhs)
   : BinaryLogicalExpression(pLhs, pRhs)
{
}

bool LogicalOrExpression::isTrue(ExpressionContext * pContext) 
{
   return getLhs()->isTrue(pContext) || getRhs()->isTrue(pContext);
}

void LogicalOrExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" || ");
   getRhs()->print(writer);
}

ExpressionNodeType LogicalOrExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_LOGICAL_OR;
}

// -----------------------------------------------
// Probability Gate

ProbabilityGate::ProbabilityGate(float threshold)
   : mThreshold(threshold)
{
}

float ProbabilityGate::getValue(ExpressionContext * pContext)
{
   return isTrue(pContext) ? 1.0 : 0.0;
}

bool ProbabilityGate::isTrue(ExpressionContext * pContext) 
{
   float rand = pContext->getRandomZeroOne();
   return rand >= mThreshold;
}

void ProbabilityGate::print(PrintWriter & writer)
{
   writer.print(" %d%%", int(mThreshold * 100.0));
}

ExpressionNodeType ProbabilityGate::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_PROBABILITY_GATE;
}

void ProbabilityGate::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
   pWriter->writeFloat(mThreshold);
}


// -----------------------------------------------
// Constant

ConstantValue::ConstantValue(float value)
   : mValue(value)
{
}

float ConstantValue::getValue(ExpressionContext * pContext)
{
   return mValue;
}
void ConstantValue::print(PrintWriter & writer)
{
   char buf[24];
   sprintf(buf, "%lf", mValue);
   //  strTrimTrailingZeroes(buf);  // <<-- TODO .. add to base writer class (maybe)
   writer.print(buf);
}

ExpressionNodeType ConstantValue::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_CONSTANT;
}

void ConstantValue::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
   pWriter->writeFloat(mValue);
}

// -----------------------------------------------
// Variable

VariableValue::VariableValue(std::string name)
   : mName(name)
{
}

std::string VariableValue::getName() const
{
   return mName;
}

// -----------------------------------------------
// Local Variable

LocalVariableValue::LocalVariableValue(std::string name, int localVariableIndex)
   : VariableValue(name)
   , mIndex(localVariableIndex)
{
}

float LocalVariableValue::getValue(ExpressionContext * pContext)
{
   assert(pContext != 0);
   return pContext->getLocalValue(mIndex);
}

void LocalVariableValue::print(PrintWriter & writer)
{
   writer.print(" %s", getName());
}

ExpressionNodeType LocalVariableValue::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_LOCAL_VARIABLE;
}

void LocalVariableValue::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
   pWriter->writeString(getName());
   pWriter->writeInt(mIndex);
}


// -----------------------------------------------
// Global Variable

GlobalVariableValue::GlobalVariableValue(std::string name)
   : VariableValue(name)
{
}

float GlobalVariableValue::getValue(ExpressionContext * pContext)
{
   assert(pContext != 0);
   // todo: get global variable ....
   return pContext->getValue(getName());
}

void GlobalVariableValue::print(PrintWriter & writer)
{
   writer.print(" $%s", getName());
}

ExpressionNodeType GlobalVariableValue::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_GLOBAL_VARIABLE;
}

void GlobalVariableValue::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
   pWriter->writeString(getName());
}

// -----------------------------------------------
// Addition

AdditionExpression::AdditionExpression(Expression * pLhs, Expression * pRhs)
   : BinaryExpression(pLhs, pRhs)
{
}

float AdditionExpression::getValue(ExpressionContext * pContext)
{
   assert(pContext != 0);
   return getLhs()->getValue(pContext) + getRhs()->getValue(pContext);
}

void AdditionExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" + ");
   getRhs()->print(writer);
}
 
ExpressionNodeType AdditionExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_ADDITION;
}

// -----------------------------------------------
// Subtraction

SubtractionExpression::SubtractionExpression(Expression * pLhs, Expression * pRhs)
   : BinaryExpression(pLhs, pRhs)
{
}

float SubtractionExpression::getValue(ExpressionContext * pContext)
{
   assert(pContext != 0);
   return getLhs()->getValue(pContext) - getRhs()->getValue(pContext);
}
void SubtractionExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" - ");
   getRhs()->print(writer);
}

ExpressionNodeType SubtractionExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_SUBTRACTION;
}

// -----------------------------------------------
// Multiply

MultiplyExpression::MultiplyExpression(Expression * pLhs, Expression * pRhs)
   : BinaryExpression(pLhs, pRhs)
{
}

float MultiplyExpression::getValue(ExpressionContext * pContext)
{
   assert(pContext != 0);
   return getLhs()->getValue(pContext) * getRhs()->getValue(pContext);
}
void MultiplyExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" * ");
   getRhs()->print(writer);
}

ExpressionNodeType MultiplyExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_MULTIPLY;
}

// -----------------------------------------------
// Divide

DivideExpression::DivideExpression(Expression * pLhs, Expression * pRhs)
   : BinaryExpression(pLhs, pRhs)
{
}

float DivideExpression::getValue(ExpressionContext * pContext)
{
   assert(pContext != 0);
   float rhsValue = getRhs()->getValue(pContext);
   if (rhsValue == 0.0)
   {
      return 0.0;
   }
   return getLhs()->getValue(pContext) / rhsValue;
}
void DivideExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" / ");
   getRhs()->print(writer);
}

ExpressionNodeType DivideExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_DIVIDE;
}

// -----------------------------------------------
// Modulo

ModuloExpression::ModuloExpression(Expression * pLhs, Expression * pRhs)
   : BinaryExpression(pLhs, pRhs)
{
}

float ModuloExpression::getValue(ExpressionContext * pContext)
{
   assert(pContext != 0);
   float rhsValue = getRhs()->getValue(pContext);
   if (rhsValue == 0.0)
   {
      return 0.0;
   }
   return fmod( getLhs()->getValue(pContext) , rhsValue );
}
void ModuloExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" %c ", '%');
   getRhs()->print(writer);
}

ExpressionNodeType ModuloExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_MODULO;
}

// -----------------------------------------------
// Greater Than

GreaterThanExpression::GreaterThanExpression(Expression * pLhs, Expression * pRhs)
   : BinaryLogicalExpression(pLhs, pRhs)
{
}

bool GreaterThanExpression::isTrue(ExpressionContext * pContext)
{
   return getLhs()->getValue(pContext) > getRhs()->getValue(pContext);
}

void GreaterThanExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" > ");
   getRhs()->print(writer);
}

ExpressionNodeType GreaterThanExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_GREATER_THAN;
}

// -----------------------------------------------
// Greater Than or Equal

GreaterThanOrEqualExpression::GreaterThanOrEqualExpression(Expression * pLhs, Expression * pRhs)
   : BinaryLogicalExpression(pLhs, pRhs)
{
}

bool GreaterThanOrEqualExpression::isTrue(ExpressionContext * pContext)
{
   float lhValue = getLhs()->getValue(pContext);
   float rhValue = getRhs()->getValue(pContext);
   return (lhValue > rhValue) || approxDoubleEquals(lhValue, rhValue);
}

void GreaterThanOrEqualExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" >= ");
   getRhs()->print(writer);
}

ExpressionNodeType GreaterThanOrEqualExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_GREATER_THAN_OR_EQUAL;
}

// -----------------------------------------------
// Equals

EqualsExpression::EqualsExpression(Expression * pLhs, Expression * pRhs)
   : BinaryLogicalExpression(pLhs, pRhs)
{
}

bool EqualsExpression::isTrue(ExpressionContext * pContext)
{
   float lhValue = getLhs()->getValue(pContext);
   float rhValue = getRhs()->getValue(pContext);
   return approxDoubleEquals(lhValue, rhValue);
}

void EqualsExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" == ");
   getRhs()->print(writer);
}

ExpressionNodeType EqualsExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_EQUALS;
}

// -----------------------------------------------
// Not Equals

NotEqualsExpression::NotEqualsExpression(Expression * pLhs, Expression * pRhs)
   : BinaryLogicalExpression(pLhs, pRhs)
{
}

bool NotEqualsExpression::isTrue(ExpressionContext * pContext)
{
   float lhValue = getLhs()->getValue(pContext);
   float rhValue = getRhs()->getValue(pContext);
   return approxDoubleEquals(lhValue, rhValue) == false;
}

void NotEqualsExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" != ");
   getRhs()->print(writer);
}

ExpressionNodeType NotEqualsExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_NOT_EQUALS;
}

// -----------------------------------------------
// Less Than

LessThanExpression::LessThanExpression(Expression * pLhs, Expression * pRhs)
   : BinaryLogicalExpression(pLhs, pRhs)
{
}

bool LessThanExpression::isTrue(ExpressionContext * pContext)
{
   return getLhs()->getValue(pContext) < getRhs()->getValue(pContext);
}

void LessThanExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" < ");
   getRhs()->print(writer);
}

ExpressionNodeType LessThanExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_LESS_THAN;
}


// -----------------------------------------------
// Less Than or Equals

LessThanOrEqualExpression::LessThanOrEqualExpression(Expression * pLhs, Expression * pRhs)
   : BinaryLogicalExpression(pLhs, pRhs)
{
}

bool LessThanOrEqualExpression::isTrue(ExpressionContext * pContext)
{
   float lhValue = getLhs()->getValue(pContext);
   float rhValue = getRhs()->getValue(pContext);
   return (lhValue < rhValue) || approxDoubleEquals(lhValue, rhValue);
}

void LessThanOrEqualExpression::print(PrintWriter & writer)
{
   getLhs()->print(writer);
   writer.print(" <= ");
   getRhs()->print(writer);
}

ExpressionNodeType LessThanOrEqualExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_LESS_THAN_OR_EQUAL;
}

// -----------------------------------------------
// Ternary

TernaryExpression::TernaryExpression(Expression * pCondition, Expression * pTruePart, Expression * pFalsePart)
   : mpCondition(pCondition)
   , mpTruePart(pTruePart)
   , mpFalsePart(pFalsePart)
{
   if (pCondition == 0)
   {
      mpCondition = new EmptyExpression();
   }
   if (pTruePart == 0)
   {
      mpTruePart = new EmptyExpression();
   }
   if (pFalsePart == 0)
   {
      mpFalsePart = new EmptyExpression();
   }
}

TernaryExpression::~TernaryExpression()
{
   delete mpCondition;
   delete mpTruePart;
   delete mpFalsePart;
}

float TernaryExpression::getValue(ExpressionContext * pContext)
{
   if (mpCondition->isTrue(pContext))
   {
      return mpTruePart->getValue(pContext);
   }
   else
   {
      return mpFalsePart->getValue(pContext);
   }
}

void TernaryExpression::print(PrintWriter & writer)
{
   mpCondition->print(writer);
   writer.print(" ? ");
   mpTruePart->print(writer);
   writer.print(" : ");
   mpFalsePart->print(writer);
}

ExpressionNodeType TernaryExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_TERNARY;
}

void TernaryExpression::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
   mpCondition->serialize(pWriter);
   mpTruePart->serialize(pWriter);
   mpFalsePart->serialize(pWriter);
}
// -----------------------------------------------
// Function


FunctionExpression::FunctionExpression(FunctionDescription * pFunctionDescription, ArgumentExpressionList * pArgumentExpressionList)
   : mpFunctionDescription(pFunctionDescription)
   , mpArgumentExpressionList(pArgumentExpressionList)
{
   assert(pFunctionDescription != 0);
   assert(pArgumentExpressionList != 0);
}

FunctionExpression::~FunctionExpression()
{
   delete mpArgumentExpressionList;
}

float FunctionExpression::getValue(ExpressionContext * pContext)
{
   ArgumentValuesList  argValues;

   DoubleLinkListIterator<Expression> iter(mpArgumentExpressionList->getArgumentExpressions());
   while (iter.hasMore())
   {
      Expression * pExpression = iter.getNext();
      float value = pExpression->getValue(pContext);
      argValues.addParameterValue(value); 
   }

   return pContext->callFunction(mpFunctionDescription, argValues);
}

void FunctionExpression::print(PrintWriter & writer)
{
   writer.print("@%s", mpFunctionDescription->mName);
   writer.print("(");
   DoubleLinkListIterator<Expression> iter(mpArgumentExpressionList->getArgumentExpressions());
   while (iter.hasMore())
   {
      Expression * pExpression = iter.getNext();
      pExpression->print(writer);
      if (iter.hasMore())
      {
         writer.print(",");
      }
   }
   writer.print(")");
}

ExpressionNodeType FunctionExpression::getNodeType()
{
   return ExpressionNodeType::EXPRESSION_NODE_FUNCTION;
}

void FunctionExpression::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);
   pWriter->writeChar(getNodeType());
   pWriter->writeString( mpFunctionDescription->mName);  // todo make method for this getName()

   int numArgumentExpressions = mpArgumentExpressionList->size();
   pWriter->writeInt(numArgumentExpressions);
   DoubleLinkListIterator<Expression> iter(mpArgumentExpressionList->getArgumentExpressions());
   while (iter.hasMore())
   {
      Expression * pExpression = iter.getNext();
      pExpression->serialize(pWriter);
   }
}

