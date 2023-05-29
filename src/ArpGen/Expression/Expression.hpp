#ifndef _expression_h_
#define _expression_h_ 1

#include "FunctionType.hpp"
#include "../../../lib/io/ByteStream.hpp"
#include "../../../lib/io/PrintWriter.hpp"
#include "../../../lib/datastruct/List.hpp"

class ExpressionContext; 
class ArgumentExpressionList;

// For Serialization
enum ExpressionNodeType
{
   EXPRESSION_NODE_EMPTY              = 'z',
   EXPRESSION_NODE_PARENTHESIZED_EXPR = '(',
   EXPRESSION_NODE_LOGICAL_NOT        = '!',
   EXPRESSION_NODE_LOGICAL_AND        = '&',
   EXPRESSION_NODE_LOGICAL_OR         = '|',
   EXPRESSION_NODE_CONSTANT           = 'C',
   EXPRESSION_NODE_LOCAL_VARIABLE     = 'V',
   EXPRESSION_NODE_GLOBAL_VARIABLE    = 'G',
   EXPRESSION_NODE_ADDITION           = '+',
   EXPRESSION_NODE_SUBTRACTION        = '-',
   EXPRESSION_NODE_MULTIPLY           = '*',
   EXPRESSION_NODE_DIVIDE             = '/',
   EXPRESSION_NODE_MODULO             = '%',
   EXPRESSION_NODE_GREATER_THAN       = '>',
   EXPRESSION_NODE_GREATER_THAN_OR_EQUAL = 'g',
   EXPRESSION_NODE_EQUALS             = '=',
   EXPRESSION_NODE_NOT_EQUALS         = 'N',
   EXPRESSION_NODE_LESS_THAN          = '<',
   EXPRESSION_NODE_LESS_THAN_OR_EQUAL = 'l',  // lower case L
   EXPRESSION_NODE_TERNARY            = '?',
   EXPRESSION_NODE_FUNCTION           = 'F',
   //
   EXPRESSION_NODE_PROBABILITY_GATE  = 'P', // class ProbabilityGate : public Expression // is this used? 
   //
   kNum_EXPRESSION_NODE_TYPES
};



class Expression : public ListNode // can be held in a List<>
{
public:
   virtual ~Expression() { } 
   
   virtual float getValue(ExpressionContext * pContext) = 0;

   virtual bool isTrue(ExpressionContext * pContext);
   virtual bool isFalse(ExpressionContext * pContext);

   virtual void print(PrintWriter & writer) = 0; 
   
   virtual ExpressionNodeType getNodeType() = 0;

   virtual void serialize(ByteStreamWriter * pWriter) = 0;

   static Expression * unpack(ByteStreamReader * pReader);

};

class EmptyExpression : public Expression // placeholder for missing parts 
{
public:
   EmptyExpression();
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual void serialize(ByteStreamWriter * pWriter) override;
   virtual ExpressionNodeType getNodeType() override;
};


class UnaryExpression : public Expression 
{
public:
   UnaryExpression(Expression * pExpression);
   virtual ~UnaryExpression();

   virtual void serialize(ByteStreamWriter * pWriter) override;

protected: 
   Expression * getExpression() { return mpExpression; }

private:
   Expression * mpExpression;
};

class BinaryExpression : public Expression 
{
public:
   BinaryExpression(Expression * pLhs, Expression * pRhs);

   virtual ~BinaryExpression();

   virtual void serialize(ByteStreamWriter * pWriter) override;

protected: 
   Expression * getLhs() { return mpLhs; } 
   Expression * getRhs() { return mpRhs; } 
private:
   Expression * mpLhs;
   Expression * mpRhs;
};

class UnaryLogicalExpression : public UnaryExpression 
{
public:
   UnaryLogicalExpression(Expression * pExpression);

   virtual float getValue(ExpressionContext * pContext) override;
};

class BinaryLogicalExpression : public BinaryExpression 
{
public:
   BinaryLogicalExpression(Expression * pLhs, Expression * pRhs);

   virtual float getValue(ExpressionContext * pContext) override;
};


// ============================================================
// Concrete Expression classes 
// ============================================================


class ParenthesizedExpression : public UnaryExpression
{
public:
   ParenthesizedExpression(Expression * pExpression);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class LogicalNotExpression : public UnaryLogicalExpression 
{
public:
   LogicalNotExpression(Expression * pExpression);
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class LogicalAndExpression : public BinaryLogicalExpression 
{
public:
   LogicalAndExpression(Expression * pLhs, Expression * pRhs);
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class LogicalOrExpression : public BinaryLogicalExpression 
{
public:
   LogicalOrExpression(Expression * pLhs, Expression * pRhs);
   virtual bool isTrue(ExpressionContext * pContext) override; 
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class ProbabilityGate : public Expression 
{
public:
   ProbabilityGate(float threshold);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
   virtual void serialize(ByteStreamWriter * pWriter) override;
private:
   float mThreshold;
};

class ConstantValue : public Expression
{
public:
   ConstantValue(float value);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
   void serialize(ByteStreamWriter * pWriter) override;
private:
   float mValue; 
}; 

class VariableValue : public Expression // base class for variable expresssions
{
public:
   VariableValue(std::string name);
   std::string getName() const;
private:
   std::string mName; 
   
     // TODO: use a SymbolReference for this 
}; 

class LocalVariableValue : public VariableValue
{
public:
   LocalVariableValue(std::string name, int localVariableIndex);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
   virtual void serialize(ByteStreamWriter * pWriter) override;
private:
   int mIndex; // index into formal/actual paramter list for production 
}; 

class GlobalVariableValue : public VariableValue
{
public:
   GlobalVariableValue(std::string name);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
   virtual void serialize(ByteStreamWriter * pWriter) override;
}; 

class AdditionExpression : public BinaryExpression
{
public:
   AdditionExpression(Expression * pLhs, Expression * pRhs);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
}; 

class SubtractionExpression : public BinaryExpression
{
public:
   SubtractionExpression(Expression * pLhs, Expression * pRhs);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
}; 

class MultiplyExpression : public BinaryExpression
{
public:
   MultiplyExpression(Expression * pLhs, Expression * pRhs);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
}; 

class DivideExpression : public BinaryExpression
{
public:
   DivideExpression(Expression * pLhs, Expression * pRhs);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
}; 

class ModuloExpression : public BinaryExpression
{
public:
   ModuloExpression(Expression * pLhs, Expression * pRhs);
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
}; 

class GreaterThanExpression : public BinaryLogicalExpression
{
public:
   GreaterThanExpression(Expression * pLhs, Expression * pRhs);
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class GreaterThanOrEqualExpression : public BinaryLogicalExpression
{
public:
   GreaterThanOrEqualExpression(Expression * pLhs, Expression * pRhs);
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class EqualsExpression : public BinaryLogicalExpression
{
public:
   EqualsExpression(Expression * pLhs, Expression * pRhs);
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class NotEqualsExpression : public BinaryLogicalExpression
{
public:
   NotEqualsExpression(Expression * pLhs, Expression * pRhs);
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class LessThanExpression : public BinaryLogicalExpression
{
public:
   LessThanExpression(Expression * pLhs, Expression * pRhs);
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class LessThanOrEqualExpression : public BinaryLogicalExpression
{
public:
   LessThanOrEqualExpression(Expression * pLhs, Expression * pRhs);
   virtual bool isTrue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
};

class TernaryExpression : public Expression
{
public:
   TernaryExpression(Expression * pCondition, Expression * pTruePart, Expression * pFalsePart);
   virtual ~TernaryExpression();
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
   void serialize(ByteStreamWriter * pWriter) override;
private:
   Expression * mpCondition;
   Expression * mpTruePart;
   Expression * mpFalsePart;
};

class FunctionExpression : public Expression
{
public:
   FunctionExpression(FunctionDescription * pFunctionDescription, ArgumentExpressionList * pArgumentExpressionList);
   virtual ~FunctionExpression();
   virtual float getValue(ExpressionContext * pContext) override;
   virtual void print(PrintWriter & writer) override;
   virtual ExpressionNodeType getNodeType() override;
   virtual void serialize(ByteStreamWriter * pWriter) override;
private:
   FunctionDescription    * mpFunctionDescription;  // Just BORROWING this  
   ArgumentExpressionList * mpArgumentExpressionList;  // OWN this
};

#endif // _expression_h_
