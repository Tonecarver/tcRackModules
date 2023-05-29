#ifndef _spec_parser_h_
#define _spec_parser_h_ 1

#include "SpecLexer.hpp"
#include "../../../lib/io/Reader.hpp"

#include "../Expression/Expression.hpp"
#include "../Expression/ArgumentList.hpp"
#include "../parser/ArpToken.hpp"
#include "../ArpTerm.hpp"  // Playable Term 

#include "../LSystem/LSystem.hpp"
//#include "ArpPlayer.h"

#include <string>
#include <cmath>

//#include "debugtrace.h"
#include <cassert>


class BuiltinSymbol;


class SpecParser
{
public:
   SpecParser(Reader & reader, LSystem & lSystem);

   virtual ~SpecParser()
   {
      // do not delete the lsystem, that was passed in by owner 
      // the 'xxxUnderConstruction' pointers point to objects already added to the lsystem
      //  so they should not be deleted here either
   }

   bool isErrored()
   {
      return mErrorCount > 0;
   }

   int getErrorLineNumber()
   {
      return mErrorLineNumber;
   }
   
   int getErrorColumnNumber()
   {
      return mErrorColumnNumber;
   }

   std::string getErrorText()
   {
      return mErrorText;
   }

   // Parse the specification
   // call isErrored() and getErrorXXX() methods to detect success
   void parse();

private:
   SpecLexer mLexer;
   int   mErrorCount;
   int   mErrorLineNumber; 
   int   mErrorColumnNumber; 
   std::string mErrorText;
   LSystem           * mpLSystemUnderConstruction; 
   LSystemProductionGroup * mpProductionGroupUnderConstruction;
   LSystemProduction * mpProductionUnderConstruction; 
   LSystemTerm       * mpTermUnderConstruction; 
   int                 mTermCounter;       // for building term ids. incremented once for each term on current line
   int                 mProductionCounter; // for building production ids. incremented once for each production
   //int                 mContextCounter; // increment for push, decrement for pop. Must be 0 at end of production 
   //int                 mGroupCounter;   // increment for begin, decrement for end. Must be 0 at end of production 

   int makeTermId()
   {
      mTermCounter++;
      return (getLineNumber() * 1000) + mTermCounter;
   }

   int makeProductionId()
   {
      mProductionCounter++;
      return (getLineNumber() * 1000) + mProductionCounter;
   }

   int getLineNumber()
   {
      return mLexer.getLineNumber();
   }
   
   int getColumnNumber()
   {
      return mLexer.getCurrentToken().mColumnNumber; // .getColumnNumber();
   }
   
   void clearError()
   {
      mErrorCount = 0;
      mErrorLineNumber = 0;
      mErrorColumnNumber = 0;
      mErrorText.erase();
   }

   // takes printf like args 
   void reportError(const char * pFmt, ...);

   bool hasMoreLines()
   {
      skipBlankLines();
      return (mErrorCount == 0) && mLexer.hasMore();
   }

   bool hasMoreTokensOnCurrentLine()
   {
      return (mErrorCount == 0) && mLexer.hasMore() && (mLexer.getCurrentToken().mTokenType != TOKEN_END_OF_LINE);
   }

   void advance()
   {
      mLexer.advance();

      // DEBUG( "parser::advance() line %d, column %d, token=%d (%s) '%s'", 
      //    getLineNumber(), 
      //    getColumnNumber(),
      //    mLexer.getCurrentToken().mTokenType,
      //    getTokenTypeName(mLexer.getCurrentToken().mTokenType),
      //    mLexer.getCurrentToken().text );
   }

   void expectToken(ArpTokenType tokenType)
   {
      if (mLexer.getCurrentToken().mTokenType != tokenType)
      {
         reportError("expected %s, found %s", 
            getTokenTypeName(tokenType), 
            getTokenTypeName(mLexer.getCurrentToken().mTokenType));
      }
   }

   void expectAndConsumeToken(ArpTokenType tokenType)
   {
      expectToken(tokenType);
      advance();
   }

   bool matchToken(ArpTokenType tokenType)
   {
      return (mErrorCount == 0) && (mLexer.getCurrentToken().mTokenType == tokenType);
   }

   bool peekToken(ArpTokenType tokenType)
   {
      return (mErrorCount == 0) && (mLexer.getPeekToken().mTokenType == tokenType);
   }

   void skipBlankLines()
   {
      while (matchToken(ArpTokenType::TOKEN_END_OF_LINE))
      {
         advance();
      }
   }

   // Points to backing buffer behind the current token
   // value in buffer changes after advance()
   char * getTokenText()
   {
      return mLexer.getCurrentToken().text;
   }

   float getTokenValue()
   {
      return mLexer.getCurrentToken().mValue;
   }

   void addProductionToLSystem();
   //{
   //   MYTRACE(( "parser::addProductionToSystem() -- STUBBED -- todo" ));
   //   // todo
   //}

   void addProduction( const char * pName );
   //{
   //   MYTRACE(( "parser  building Production  name='%s'", pName ));

   //   int productionId = makeProductionId(); 

   //   mpProductionUnderConstruction = new LSystemProduction(pName, productionId);
   //   mpLSystemUnderConstruction->addProduction(mpProductionUnderConstruction);
   //   mpProductionGroupUnderConstruction = mpLSystemUnderConstruction->getProductionGroup(pName);
   //   mTermCounter = 0;
   //   //mContextCounter = 0;
   //   //mGroupCounter = 0;
   //}

   void endProduction();
   //{
   //   //if (mContextCounter > 0)
   //   //{
   //   //   reportError("Missing %d context pop(s)", mContextCounter);
   //   //}
   //   //if (mGroupCounter > 0)
   //   //{
   //   //   reportError("Missing %d group end(s)", mGroupCounter);
   //   //}
   //}


   void setProductionProbability(float prob);
   void addProductionFormalArguments(FormalArgumentList * pFormalArgumentList);
   int getFormalArgumentIndex(const char * pName);
   void addProductionCondition(Expression * pExpression);
   void addRewritableTerm( const char * pName );
   void addPlayableTerm( const char * pName, ArpTermType termType );
   void addRestTerm( const char * pName );
   void addActionTerm( const char * pName, int termType );
   void addProbabilityTerm( const char * pName, int termType, float probabilityValue );
   void addConditionTerm( const char * pName, int termType, Expression * pExpression );
   void addTermArgumentExpressionList(ArgumentExpressionList * pArgumentExpressionList);
   void addFunctionTerm( const char * pName );

   void addSymbolIncrementTerm(BuiltinSymbol * pSymbolDescription);
   void addSymbolDecrementTerm(BuiltinSymbol * pSymbolDescription);
   void addSymbolNominalTerm(BuiltinSymbol * pSymbolDescription);
   void addSymbolRandomTerm(BuiltinSymbol * pSymbolDescription);
   void addSymbolAssignTerm(BuiltinSymbol * pSymbolDescription, Expression * pExpression);

   void getSpec();
   void getVariables();
   void getProductions();
   void getProduction();
   void getProductionName();
   void getOptionalProductionParameters();
   FormalArgumentList * getProductionFormalArguments();
   void getOptionalProductionCondition();
   void getProductionTerms(); // remove this ? 
   void getExpressionTermSequence();
   void getFunctionTerm();
   //void getGroupTermSequence();
   void getContextTermSequence();
   void getProductionTermSequence();
   //void getProductionTerm(); // remove this ?
   //void getOptionalProductionTermParameters();
   void getOptionalTermArgumentList();
   ArgumentExpressionList * getArgumentExpressionList();

   void expectExpression(Expression * pExpression); // report error if expression is null 

   Expression * getExpression();
   Expression * getAssignmentExpression();
   Expression * getConditionalExpression();
   Expression * getLogicalOrExpression();
   Expression * getLogicalAndExpression();
   Expression * getEqualityExpression();
   Expression * getRelationalExpression();
   Expression * getAdditiveExpression();
   Expression * getMultiplicativeExpression();
   Expression * getUnaryExpression();
   Expression * getPostfixExpression();
   Expression * getPrimaryExpression();
   Expression * getFunctionExpression();

//   SymbolDescription * getBuiltinSymbolDescription(const char * pName);
//    SymbolDescription * findOrCreateSymbolDescription(const char * pName);

   ///---

};


#endif // _spec_parser_h_
