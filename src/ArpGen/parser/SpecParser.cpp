#include "SpecParser.hpp"

bool verbose = false; // TODO: ove this to plugin.hpp header file
#define MYTRACE(x) if (verbose) { DEBUG x; }

// TODO: movue to utility header 
inline bool strmatch(const char * s1, const char * s2) {
   if (s1 == s2) {
      return true;
   }
   return (s1 && s2 && strcmp(s1,s2) == 0);
}

inline bool strmatch(const char * s1, std::string std2) {
   return strmatch(s1, std2.c_str());
}


class BuiltinSymbol
{
public:
   const char * mpName;
   ArpTermType mIncrementType;
   ArpTermType mDecrementType;
   ArpTermType mRandomType;
   ArpTermType mNominalType;
   ArpTermType mAssignType;

};

BuiltinSymbol builtinSymbolDescriptions[] = 
{
   // name  increment          decrement            random                 nominal                 assign 
   { "n",  TERM_NOTE_UP,      TERM_NOTE_DOWN,      TERM_NOTE_RANDOM,      TERM_NOTE_NOMINAL,      TERM_NOTE_ASSIGN },
   { "s",  TERM_SEMITONE_UP,  TERM_SEMITONE_DOWN,  TERM_SEMITONE_RANDOM,  TERM_SEMITONE_NOMINAL,  TERM_SEMITONE_ASSIGN },
//   { "v",  TERM_VELOCITY_UP,  TERM_VELOCITY_DOWN,  TERM_VELOCITY_RANDOM,  TERM_VELOCITY_NOMINAL,  TERM_VELOCITY_ASSIGN },
   { "o",  TERM_OCTAVE_UP,    TERM_OCTAVE_DOWN,    TERM_OCTAVE_RANDOM,    TERM_OCTAVE_NOMINAL,    TERM_OCTAVE_ASSIGN },

#if ARP_PLAYER_NUM_HARMONIES >=1
   { "h1", TERM_HARMONY_1_UP, TERM_HARMONY_1_DOWN, TERM_HARMONY_1_RANDOM, TERM_HARMONY_1_NOMINAL, TERM_HARMONY_1_ASSIGN },
#endif
#if ARP_PLAYER_NUM_HARMONIES >= 2
   { "h2", TERM_HARMONY_2_UP, TERM_HARMONY_2_DOWN, TERM_HARMONY_2_RANDOM, TERM_HARMONY_2_NOMINAL, TERM_HARMONY_2_ASSIGN },
#endif
#if ARP_PLAYER_NUM_HARMONIES >= 3
   { "h3", TERM_HARMONY_3_UP, TERM_HARMONY_3_DOWN, TERM_HARMONY_3_RANDOM, TERM_HARMONY_3_NOMINAL, TERM_HARMONY_3_ASSIGN },
#endif
#if ARP_PLAYER_NUM_HARMONIES == 4 
   { "h4", TERM_HARMONY_4_UP, TERM_HARMONY_4_DOWN, TERM_HARMONY_4_RANDOM, TERM_HARMONY_4_NOMINAL, TERM_HARMONY_4_ASSIGN },  
#endif
#if ARP_PLAYER_NUM_HARMONIES > 4 
   Unsupported number of Harmonies 
#endif

//   { "gspeed", TERM_GLISS_SPEED_UP, TERM_GLISS_SPEED_DOWN, TERM_GLISS_SPEED_RANDOM, TERM_GLISS_SPEED_NOMINAL, TERM_GLISS_SPEED_ASSIGN },
//   { "gdir",   TERM_GLISS_DIR_UP,   TERM_GLISS_DIR_DOWN,   TERM_GLISS_DIR_RANDOM,   TERM_GLISS_DIR_NOMINAL,   TERM_GLISS_DIR_ASSIGN },
   //
   { 0, (ArpTermType)-1, (ArpTermType)-1, (ArpTermType)-1, (ArpTermType)-1, (ArpTermType)-1 }, // end of table marker 
};

BuiltinSymbol * getBuiltinSymbolDescription(const char * pName)
{
   BuiltinSymbol * pDescription = builtinSymbolDescriptions;
   while (pDescription->mpName != 0)
   {
      if (strmatch(pDescription->mpName, pName))
      {
         return pDescription;
      }
      pDescription++;
   }
   return 0; // not found 
}




SpecParser::SpecParser(Reader & reader, LSystem & lSystem)
   : mLexer(reader)
   , mpLSystemUnderConstruction(&lSystem)
   , mpProductionUnderConstruction(0)
   , mpTermUnderConstruction(0)
   , mTermCounter(0)
   , mProductionCounter(0)
{
   clearError();
}

void SpecParser::parse()
{
   // initializeState()
   // mLexer.initialize();
   getVariables();
   getProductions();
}


// takes printf like args 
void SpecParser::reportError(const char * pFmt, ...)
{
   if (mErrorCount == 0)
   {
      char buf[256];
      va_list args;
      va_start(args, pFmt);
      vsprintf(buf, pFmt, args);
      mErrorLineNumber = mLexer.getCurrentToken().mLineNumber; // getLineNumber();
      mErrorColumnNumber = mLexer.getCurrentToken().mColumnNumber; // .getColumnNumber();
      mErrorText = buf;
      va_end(args);

      MYTRACE(( ">> Parse error: line %d, column %d: %s", mErrorLineNumber, mErrorColumnNumber, buf ));
   }
   mErrorCount++;
}


//bool SpecParser::hasMoreLines()
//{
//   skipBlankLines();
//   return (mErrorCount == 0) && mLexer.hasMore();
//}
//
//bool hasMoreTokensOnCurrentLine()
//{
//   return (mErrorCount == 0) && mLexer.hasMore() && (mLexer.getCurrentToken().mTokenType != TOKEN_END_OF_LINE);
//}

//void SpecParser::advance()
//{
//   mLexer.advance();
//
//   MYTRACE(( "parser::advance() line %d, column %d, token=%d (%s) '%s'", 
//      getLineNumber(), 
//      getColumnNumber(),
//      mLexer.getCurrentToken().mTokenType,
//      getTokenTypeName(mLexer.getCurrentToken().mTokenType),
//      mLexer.getCurrentToken().text ));
//
//}

//void SpecParser::expectToken(ArpTokenType tokenType)
//{
//   if (mLexer.getCurrentToken().mTokenType != tokenType)
//   {
//      reportError("expected %s, found %s", 
//         getTokenTypeName(tokenType), 
//         getTokenTypeName(mLexer.getCurrentToken().mTokenType));
//   }
//}
//
//void SpecParser::expectAndConsumeToken(ArpTokenType tokenType)
//{
//   expectToken(tokenType);
//   advance();
//}
//
//bool matchToken(ArpTokenType tokenType)
//{
//   return (mErrorCount == 0) && (mLexer.getCurrentToken().mTokenType == tokenType);
//}
//
//bool peekToken(ArpTokenType tokenType)
//{
//   return (mErrorCount == 0) && (mLexer.getPeekToken().mTokenType == tokenType);
//}
//
//void SpecParser::skipBlankLines()
//{
//   while (matchToken(ArpTokenType::TOKEN_END_OF_LINE))
//   {
//      advance();
//   }
//}
//
//// Points to backing buffer behind the current token
//// value in buffer changes after advance()
//const char * getTokenText()
//{
//   return mLexer.getCurrentToken().text;
//}
//
//float getTokenValue()
//{
//   return mLexer.getCurrentToken().mValue;
//}

void SpecParser::addProductionToLSystem()
{
   MYTRACE(( "parser::addProductionToSystem() -- STUBBED -- todo" ));
   // todo
}

void SpecParser::addProduction( const char * pName )
{
   MYTRACE(( "parser  building Production  name='%s'", pName ));

   int productionId = makeProductionId(); 

   mpProductionUnderConstruction = new LSystemProduction(pName, productionId);
   mpLSystemUnderConstruction->addProduction(mpProductionUnderConstruction);
   mpProductionGroupUnderConstruction = mpLSystemUnderConstruction->getProductionGroup(pName);
   mTermCounter = 0;
   //mContextCounter = 0;
   //mGroupCounter = 0;
}

void SpecParser::endProduction()
{
   //if (mContextCounter > 0)
   //{
   //   reportError("Missing %d context pop(s)", mContextCounter);
   //}
   //if (mGroupCounter > 0)
   //{
   //   reportError("Missing %d group end(s)", mGroupCounter);
   //}
}

void SpecParser::setProductionProbability(float prob)
{
   MYTRACE(( "parser  setting Production probabilty to %lf", prob ));

   bool isCompatible = true; 

   // Current production under consruction has already been added to the production group
   // so the count will always be at least 1. A production count of 1 means that the 
   // current production under construction is the only production in the group (so far).

   if (mpProductionGroupUnderConstruction->getNumProductions() > 1) // other productions already defined 
   {
      if (mpProductionGroupUnderConstruction->isSelectRandom() == false) // other productions are all random select       
      {
         reportError("Probability-selected production is not consistent with other productions in this group");
         isCompatible = false;
      }
   }

   if (isCompatible)
   {
      // todo: emit error if out of range ?
      prob = clamp(prob, 0.f, 1.f);
      mpProductionUnderConstruction->setProbaility(prob);
   }
}

void SpecParser::addProductionFormalArguments(FormalArgumentList * pFormalArgumentList)
{ 
   MYTRACE(( "parser  adding Production Formal Parameters: size = %d", pFormalArgumentList->size() ));

   bool isCompatible = true; 

   // Current production under consruction has already been added to the production group
   // so the count will always be at least 1. A production count of 1 means that the 
   // current production under construction is the only production in the group (so far).

   if (mpProductionGroupUnderConstruction->getNumProductions() > 1) // other productions present 
   {
      if (mpProductionGroupUnderConstruction->isSelectParameterized() == false)
      {
         reportError("Parameterized production is not consistent with other productions in this group");
         isCompatible = false;
      }
      else if (mpProductionGroupUnderConstruction->getNumFormalParameters() != pFormalArgumentList->size())
      {
         reportError("Incorrect number of formal parameters: found %d, expected %d", 
            pFormalArgumentList->size(),
            mpProductionGroupUnderConstruction->getNumFormalParameters());
         isCompatible = false;
      }
   }

   if (isCompatible)
   {
      mpProductionUnderConstruction->setFormalArgumentList(pFormalArgumentList);
   }
   else
   {
      delete pFormalArgumentList;
   }
}

int SpecParser::getFormalArgumentIndex(const char * pName)
{
   FormalArgumentList * pFormalArgumentList = mpProductionUnderConstruction->getFormalArgumentList();
   if (pFormalArgumentList != 0)
   {
      int numFormalArguments = pFormalArgumentList->size();
      for (int i = 0; i < numFormalArguments; i++)
      {
         if (strmatch(pName, pFormalArgumentList->getNameAt(i)))
         {
            return i; // match 
         }
      }
   }

   return -1; // no match 
}

void SpecParser::addProductionCondition(Expression * pExpression)
{
   MYTRACE(( "parser  adding Production condition" ));

   bool isCompatible = true; 

   // Current production under consruction has already been added to the production group
   // so the count will always be at least 1. A production count of 1 means that the 
   // current production under construction is the only production in the group (so far).

   if (mpProductionGroupUnderConstruction->getNumProductions() > 1) // other productions present 
   {
      if (mpProductionGroupUnderConstruction->isSelectParameterized() == false) 
      {
         reportError("Probability-selected production is not consistent with other productions in this group");
         isCompatible = false;
      }
   }

   if (isCompatible)
   {
      mpProductionUnderConstruction->setSelectionExpression(pExpression);
   }
   else
   {
      delete pExpression;
   }
}



void SpecParser::addRewritableTerm( const char * pName )
{
   int termType = ArpTermType::TERM_REWRITABLE;
   int termId = makeTermId();

   MYTRACE(( "parser  adding Rewritable Term  name='%s'  type=%d  id=%d", pName, termType, termId ));

   mpTermUnderConstruction = new LSystemTerm( pName, termType, termId );
   mpTermUnderConstruction->setRewritable(true);
   mpTermUnderConstruction->setExecutable(false);
   mpTermUnderConstruction->setStepTerminator(false);
   mpProductionUnderConstruction->addTerm(mpTermUnderConstruction);
}

void SpecParser::addPlayableTerm( const char * pName, ArpTermType termType )
{
   int termId = makeTermId();

   MYTRACE(( "parser  adding %s Term  name='%s'  type=%d  id=%d", arpTermTypeNames[termType], pName, termType, termId ));

   mpTermUnderConstruction = new LSystemTerm( pName, termType, termId );
   mpTermUnderConstruction->setRewritable(true);
   mpTermUnderConstruction->setExecutable(true);
   mpTermUnderConstruction->setStepTerminator(true);
   mpProductionUnderConstruction->addTerm(mpTermUnderConstruction);
}

void SpecParser::addRestTerm( const char * pName )
{
   int termType = ArpTermType::TERM_REST;
   int termId = makeTermId();

   MYTRACE(( "parser  adding Rest Term  name='%s'  type=%d  id=%d", pName, termType, termId ));

   mpTermUnderConstruction = new LSystemTerm( pName, termType, termId );
   mpTermUnderConstruction->setRewritable(false);
   mpTermUnderConstruction->setExecutable(true);
   mpTermUnderConstruction->setStepTerminator(true);
   mpProductionUnderConstruction->addTerm(mpTermUnderConstruction);
}

void SpecParser::addActionTerm( const char * pName, int termType )
{
   int termId = makeTermId();

   MYTRACE(( "parser  adding Action Term  name='%s'  type=%d  id=%d", pName, termType, termId ));

   mpTermUnderConstruction = new LSystemTerm( pName, termType, termId );
   mpTermUnderConstruction->setRewritable(false);
   mpTermUnderConstruction->setExecutable(true);
   mpTermUnderConstruction->setStepTerminator(false);
   mpProductionUnderConstruction->addTerm(mpTermUnderConstruction);
}

void SpecParser::addProbabilityTerm( const char * pName, int termType, float probabilityValue )
{
   int termId = makeTermId();

   MYTRACE(( "parser  adding Probability Term  name='%s'  type=%d  id=%d", pName, termType, termId ));

   if ((probabilityValue < 0.0) || (probabilityValue > 100.0))
   {
      reportError("probability value must be between 0.0 and 100.0" );
      probabilityValue = 50.0;
   }
   probabilityValue *= 0.01; // convert to 0..1
   mpTermUnderConstruction = new LSystemTerm( pName, termType, termId );
   mpTermUnderConstruction->setRewritable(false);
   mpTermUnderConstruction->setExecutable(true);
   mpTermUnderConstruction->setStepTerminator(false);

   // class ProbabilityThreshold : public LogicalExpression
   mpTermUnderConstruction->setCondition(new ProbabilityGate(probabilityValue) );

   mpProductionUnderConstruction->addTerm(mpTermUnderConstruction);
}

void SpecParser::addConditionTerm( const char * pName, int termType, Expression * pExpression )
{
   int termId = makeTermId();

   MYTRACE(( "parser  adding Condition Term  name='%s'  type=%d  id=%d", pName, termType, termId ));

   // todo: verify pExression is a logical expressiion ? 

   mpTermUnderConstruction = new LSystemTerm( pName, termType, termId );
   mpTermUnderConstruction->setRewritable(false);
   mpTermUnderConstruction->setExecutable(true);
   mpTermUnderConstruction->setStepTerminator(false);
   mpTermUnderConstruction->setCondition(pExpression);

   mpProductionUnderConstruction->addTerm(mpTermUnderConstruction);
}

void SpecParser::addTermArgumentExpressionList(ArgumentExpressionList * pArgumentExpressionList)
{
   MYTRACE(( "parser  adding ArgumentExpressionList to term, size = %d", pArgumentExpressionList->size() ));
   mpTermUnderConstruction->setArgumentExpressionList(pArgumentExpressionList);
}

void SpecParser::addFunctionTerm( const char * pName )
{
   int termId = makeTermId();

   MYTRACE(( "parser  adding Function Term  name='%s'  id=%d", pName, termId ));

   FunctionDescription * pFunctionDescription = getFunctionDescriptionByName( pName );
   if (pFunctionDescription == 0)
   {
      reportError("no such function: %s", getTokenText() );
      // fall through to build the term anyway so the parts can be de-allocated
   }

   mpTermUnderConstruction = new LSystemTerm( pName, ArpTermType::TERM_FUNCTION, termId );
   mpTermUnderConstruction->setRewritable(false);
   mpTermUnderConstruction->setExecutable(true);
   mpTermUnderConstruction->setStepTerminator(false);

   mpProductionUnderConstruction->addTerm(mpTermUnderConstruction);
}



void SpecParser::addSymbolIncrementTerm(BuiltinSymbol * pSymbolDescription)
{
   assert(pSymbolDescription != 0);

   char nameBuf[24];
   sprintf(nameBuf, "$%s+", pSymbolDescription->mpName);

   addActionTerm(nameBuf, pSymbolDescription->mIncrementType);
}

void SpecParser::addSymbolDecrementTerm(BuiltinSymbol * pSymbolDescription)
{
   assert(pSymbolDescription != 0);

   char nameBuf[24];
   sprintf(nameBuf, "$%s-", pSymbolDescription->mpName);

   addActionTerm(nameBuf, pSymbolDescription->mDecrementType);
}

void SpecParser::addSymbolNominalTerm(BuiltinSymbol * pSymbolDescription)
{
   assert(pSymbolDescription != 0);

   char nameBuf[24];
   sprintf(nameBuf, "$%s!", pSymbolDescription->mpName);

   addActionTerm(nameBuf, pSymbolDescription->mNominalType);
}

void SpecParser::addSymbolRandomTerm(BuiltinSymbol * pSymbolDescription)
{
   assert(pSymbolDescription != 0);

   char nameBuf[24];
   sprintf(nameBuf, "$%s*", pSymbolDescription->mpName);

   addActionTerm(nameBuf, pSymbolDescription->mRandomType);
}

void SpecParser::addSymbolAssignTerm(BuiltinSymbol * pSymbolDescription, Expression * pExpression)
{
   assert(pSymbolDescription != 0);

   char nameBuf[24];
   sprintf(nameBuf, "$%s=", pSymbolDescription->mpName);

   addActionTerm(nameBuf, pSymbolDescription->mAssignType);
   mpTermUnderConstruction->setCondition(pExpression);  // << todo is this right ?
}


//void SpecParser::setTermProbability(float value)
//{
//   mpTermUnderConstruction->setProbaility(value);
//}

//void SpecParser::setTermRepetition(int valueMin, int valueMax)
//{
//   if (valueMin == valueMax)
//   {
//      mpTermUnderConstruction->getRepetitionConstraint().addValue(valueMin);
//   }
//   else
//   {
//      mpTermUnderConstruction->getRepetitionConstraint().addRange(valueMin, valueMax);
//   }
//}

//void SpecParser::setProductionTermName( const char * pName )
//{
//   MYTRACE(( "parser::setProductionTermName(  '%s' ) -- STUBBED -- todo", pName ));
//   // todo

//   // will need to set the term attributes based on the name 
//   //   executable, rewritable, 

//   bool isExecutable = true; 
//   bool isRewritable = false; 

//   if (strmatch(pName, "N"))
//   {
//      MYTRACE(( "  term is NOTE:" ));
//      isRewritable = true; 
//   }
//   
//   // todo: need a lookup table or something for this .. 

//   MYTRACE(( "   executable = %d", isExecutable ));
//   MYTRACE(( "   rewritable = %d", isRewritable ));

//}

void SpecParser::getSpec()
{
   getVariables();
   getProductions();
}

void SpecParser::getVariables()
{
   // todo:
   MYTRACE(( "parser::getVariables() -- STUBBED -- todo" ));
}

void SpecParser::getProductions()
{
   MYTRACE(( "parser --- get productions(), line %d, column %d", getLineNumber(), getColumnNumber() ));
   while (hasMoreLines())
   {
      getProduction();
   }
   MYTRACE(( "parser --- get productions(), done"));
}

void SpecParser::getProduction()
{
   MYTRACE(( "parser --- start new production, line %d, column %d", getLineNumber(), getColumnNumber() ));

   getProductionName();
   getOptionalProductionParameters();
   expectAndConsumeToken(TOKEN_EQUAL);
   getProductionTerms();
   endProduction();
}

void SpecParser::getProductionName()
{
   expectToken(TOKEN_IDENT);
   addProduction( getTokenText() );
   advance();
}

void SpecParser::getOptionalProductionParameters()
{
   // NAME ( probability )
   // NAME ( formal_args ) : conditional expression
   if (matchToken(TOKEN_OPEN_PAREN))
   {
      advance(); // consumer the '('
      if (matchToken(TOKEN_NUMBER))
      {
         float prob = getTokenValue(); 
         setProductionProbability(prob);
         advance(); // consume the value 
         expectAndConsumeToken(TOKEN_CLOSE_PAREN);
      }
      else if (matchToken(TOKEN_IDENT))
      {
         FormalArgumentList * pFormalArgumentList = getProductionFormalArguments();
         addProductionFormalArguments(pFormalArgumentList);
         expectAndConsumeToken(TOKEN_CLOSE_PAREN);
         getOptionalProductionCondition();
      }
   }
}

FormalArgumentList * SpecParser::getProductionFormalArguments()
{
   FormalArgumentList * pFormalArgumentList = new FormalArgumentList();

   if (matchToken(TOKEN_CLOSE_PAREN))
   {
      return pFormalArgumentList; // empty list
   }

   while (hasMoreTokensOnCurrentLine())
   {
      if (matchToken(TOKEN_CLOSE_PAREN))
      {
         break;
      }

      if (matchToken(TOKEN_COMMA))
      {
         if (pFormalArgumentList->size() == 0)
         {
            reportError("expected formal parameter name");
         }
         advance(); // consume the comma
      }

      if (matchToken(TOKEN_IDENT) == false)
      {
         reportError("formal parameter name must be a valid identifier");
         break; // stop on non-identifier
      }

      char* pName = getTokenText();
      if (pFormalArgumentList->contains(pName))
      {
         reportError("Formal parameter name is not unique: %s", pName);
      }

      if (pFormalArgumentList->isFull())
      {
         reportError("Number of arguments exceeds builtin maximum");
      }
      else
      {
         pFormalArgumentList->addParameterName(pName);
      }

      advance(); // consume the param name
   }

   return pFormalArgumentList;
}

void SpecParser::getOptionalProductionCondition()
{
   if (matchToken(TOKEN_COLON))
   {
      advance(); // consume the : 

      Expression * pExpression = getExpression();
      if (pExpression == 0)
      {
         reportError("Expected conditional expression");
      }

      addProductionCondition(pExpression);
   }
}

void SpecParser::getProductionTerms() // remove this ? 
{
   getProductionTermSequence();
   if (matchToken(TOKEN_END_OF_LINE) || matchToken(TOKEN_END_OF_FILE))
   {
      advance(); // consume the token
   }
   else
   {
      reportError("Unexpected token at end of production terms: %s", getTokenText());
      advance(); 
   }

   // getTermSequence()
   // expect EOL

   //while (hasMoreTokensOnCurrentLine())
   //{
   //   getProductionTerm();
   //}
   //advance(); // consume the end of line token
}

//void SpecParser::getProbableTermSequence()  /// TODO: re-activate this .. 
//{
//   // matchToken is number
//   // peekToken is % 
//   float prob = getTokenValue(); 
//   MYTRACE(( "parser getProbableTermSequence: prob = %lf", prob ));

//   addProbabilityTerm( getTokenText(), ArpTermType::TERM_PROBABILITY, prob );

//   expectAndConsumeToken(TOKEN_NUMBER);
//   expectAndConsumeToken(TOKEN_PERCENT);

//   getProductionTerm();
//   getOptionalElseTerm(); 
//}

void SpecParser::getExpressionTermSequence()
{
   // matchToken is { 
   expectToken(TOKEN_OPEN_CURLY_BRACE);
   advance(); // consume the token

   Expression * pExpression = getExpression();

   addConditionTerm( "{", ArpTermType::TERM_EXPRESSION_BEGIN, pExpression);

   if (matchToken(TOKEN_QUESTION_MARK))
   {
      addActionTerm( "?", ArpTermType::TERM_IF);
      advance(); // consume the ? 

      getProductionTermSequence();
      if (matchToken(TOKEN_COLON))
      {
         advance(); // consume token
         addActionTerm( ":", ArpTermType::TERM_ELSE);
         getProductionTermSequence();
      }
   }
   expectToken(TOKEN_CLOSE_CURLY_BRACE); 
   advance(); // consume it 

   // add token '}'
   addActionTerm( "}", ArpTermType::TERM_EXPRESSION_END);
}

void SpecParser::getFunctionTerm()
{
   MYTRACE(( "parser getFunctionTerm" ));
   // matchToken is (
   expectToken(TOKEN_AT);
   advance(); // consume the @

   expectToken(TOKEN_IDENT);
   addFunctionTerm( getTokenText() ); 
   advance();

   if (matchToken(TOKEN_OPEN_PAREN))
   {
      advance(); // consume the (

      ArgumentExpressionList * pArgumentExpressionList = getArgumentExpressionList();

      expectAndConsumeToken(TOKEN_CLOSE_PAREN);

   // todo: verify that arg list size == description->numArgs
      addTermArgumentExpressionList(pArgumentExpressionList);
   }
}


//void SpecParser::getGroupTermSequence()
//{
//   MYTRACE(( "parser getGroupTermSequence" ));
//   // matchToken is (
//   expectToken(TOKEN_OPEN_PAREN);
//   //mGroupCounter++;
//   addActionTerm( getTokenText(), ArpTermType::TERM_GROUP_BEGIN);
//   advance(); // consume the token
//      
//   // collect terms up to closing/stopping point
//   getProductionTermSequence();
//
//   expectToken(TOKEN_CLOSE_PAREN);
//   addActionTerm( getTokenText(), ArpTermType::TERM_GROUP_END);
//   advance(); // consume the ) 
//
//   //while (hasMoreTokensOnCurrentLine())
//   //{
//   //   if (matchToken(TOKEN_CLOSE_PAREN))
//   //   {
//   //      mGroupCounter--;
//   //      addActionTerm( getTokenText(), ArpTermType::TERM_GROUP_END);
//   //      advance(); // consume the ')'
//   //      return;
//   //   }
//   //   getProductionTerm();
//   //}
//}

void SpecParser::getContextTermSequence()
{
   MYTRACE(( "parser getContextTermSequence" ));
   // matchToken is [
   expectToken(TOKEN_OPEN_SQUARE_BRACKET);
   addActionTerm( getTokenText(), ArpTermType::TERM_PUSH);
   advance(); // consume the token

   // collect terms up to closing/stopping point
   getProductionTermSequence();

   expectToken(TOKEN_CLOSE_SQUARE_BRACKET);
   addActionTerm( getTokenText(), ArpTermType::TERM_POP);
   advance(); // consume the ']'

//      mContextCounter++;
   //while (hasMoreTokensOnCurrentLine())
   //{
   //   if (matchToken(TOKEN_CLOSE_SQUARE_BRACKET))
   //   {
   //      mContextCounter--;
   //      addActionTerm( getTokenText(), ArpTermType::TERM_POP);
   //      advance(); // consume the ']'
   //      return;
   //   }
   //   getProductionTerm();
   //}
}

//void SpecParser::getOptionalElseTerm() // todo: remove 
//{
//   MYTRACE(( "parser getOptionalElse" ));
//   if (matchToken(TOKEN_COLON))
//   {
//      MYTRACE(( "parser getOptionalElse -- found else branch" ));
//      addActionTerm( getTokenText(), ArpTermType::TERM_ELSE);
//      advance(); // consume the : 
//      getProductionTerm();
//   }
//}

void SpecParser::getProductionTermSequence()
{
   while (hasMoreTokensOnCurrentLine())
   {
      //if (matchToken(TOKEN_NUMBER) && peekToken(TOKEN_PERCENT))
      //{
      //   getProbableTermSequence(); // TODO: reactivea this ? 
      //}
      if (matchToken(TOKEN_OPEN_CURLY_BRACE))
      {
         getExpressionTermSequence(); 
      }
      //else if (matchToken(TOKEN_OPEN_PAREN))
      //{
      //   getGroupTermSequence();
      //}
      else if (matchToken(TOKEN_OPEN_SQUARE_BRACKET))
      {
         getContextTermSequence();
      }
      else if (matchToken(TOKEN_IDENT))
      {
         const char * pName = getTokenText();
         const PlayableTerm * pPlayableTerm = getPlayableTermDescription(pName);
         if (pPlayableTerm != 0)
         {
            addPlayableTerm( pName, pPlayableTerm->mTermType );
         }
         else
         {
            addRewritableTerm( getTokenText() );
         }
         advance(); // consume the identifier 

         getOptionalTermArgumentList();
      }
      else if (matchToken(TOKEN_UNDERSCORE))
      {
         addRestTerm( getTokenText() );
         advance(); // consume the token
      }
      else if (matchToken(TOKEN_NUMBER))
      {
         // add explicit note number 
         reportError("Bare numbers not supported yet .. ");
         advance(); // consume the number 
      }
      else if (matchToken(TOKEN_AT))
      {
         getFunctionTerm();
      }
      else if (matchToken(TOKEN_PLUS))
      {
         addActionTerm( getTokenText(), ArpTermType::TERM_NOTE_UP);
         advance(); // consume the token
      }
      else if (matchToken(TOKEN_MINUS))
      {
         addActionTerm( getTokenText(), ArpTermType::TERM_NOTE_DOWN);
         advance(); // consume the token
      }
      else if (matchToken(TOKEN_FORWARD_SLASH))
      {
         addActionTerm( getTokenText(), ArpTermType::TERM_SEMITONE_UP);
         advance(); // consume the token
      }
      else if (matchToken(TOKEN_BACKSLASH))
      {
         addActionTerm( getTokenText(), ArpTermType::TERM_SEMITONE_DOWN);
         advance(); // consume the token
      }
      // else if (matchToken(TOKEN_OPEN_ANGLE_BRACKET))
      // {
      //    addActionTerm( getTokenText(), ArpTermType::TERM_VELOCITY_UP);
      //    advance(); // consume the token
      // }
      // else if (matchToken(TOKEN_CLOSE_ANGLE_BRACKET))
      // {
      //    addActionTerm( getTokenText(), ArpTermType::TERM_VELOCITY_DOWN);
      //    advance(); // consume the token
      // }
      else if (matchToken(TOKEN_CARET))
      {
         addActionTerm( getTokenText(), ArpTermType::TERM_OCTAVE_UP);
         advance(); // consume the token
      }
      // else if (matchToken(TOKEN_AMPERSAND))
      // {
      //    addActionTerm( getTokenText(), ArpTermType::TERM_VELOCITY_DOWN);
      //    advance(); // consume the token
      // }
      else if (matchToken(TOKEN_VERTICAL_BAR))
      {
         addActionTerm( getTokenText(), ArpTermType::TERM_UP_DOWN_INVERT);
         advance(); // consume the token
      }
      else if (matchToken(TOKEN_TILDE))
      {
         addActionTerm( getTokenText(), ArpTermType::TERM_UP_DOWN_NOMINAL);
         advance(); // consume the token
      }
      else if (matchToken(TOKEN_DOLLAR))
      {
         advance(); // consume the $
         expectToken(TOKEN_IDENT);

         // Maybe this should be restricted to BUILTIN symbols only 
         // Use expressions { $x = 12 } for USER-DEFINED symbols 
         // but expressions can reference BOTH 
         //   { $h1 = $h2 - 7 }

         const char * pSymbolName = getTokenText();

         BuiltinSymbol * pSymbolDescription = getBuiltinSymbolDescription(pSymbolName);
         if (pSymbolDescription == 0)
         {
            reportError("inappropriate symbol reference: expected a builtin symbol");
            advance(); // consume the ident
         }
         
         else if (peekToken(TOKEN_PLUS))
         {
            addSymbolIncrementTerm(pSymbolDescription);
            advance(); // consume the token
            advance(); // consume the +
         }
         else if (peekToken(TOKEN_MINUS))
         {
            addSymbolDecrementTerm(pSymbolDescription);
            advance(); // consume the token
            advance(); // consume the -
         }
         else if (peekToken(TOKEN_BANG))
         {
            addSymbolNominalTerm(pSymbolDescription);
            advance(); // consume the token
            advance(); // consume the !
         }
         else if (peekToken(TOKEN_ASTERISK))
         {
            addSymbolRandomTerm(pSymbolDescription);
            advance(); // consume the token
            advance(); // consume the *
         }
         else if (peekToken(TOKEN_EQUAL))
         {
            // todo get Expression 
            // assignment 
            advance(); // consume the token -- << OVERWTITES the SYMBOL NAME bufffer .. neeed to save it first =
            advance(); // consume the =
            Expression * pExpression = getExpression();
            addSymbolAssignTerm(pSymbolDescription, pExpression);
         }
         else
         {
            reportError("incomplete symbol reference");
            advance(); // comsume the ident
         }
      }
      else
      {
         MYTRACE(( "getTermSequence: ending on token '%s'",  getTokenText() ));
         break; // stop collecting on first token that cannot begin a term sequence 
      }
   }
}

void SpecParser::getOptionalTermArgumentList()
{
   if (matchToken(TOKEN_OPEN_PAREN))
   {
      advance(); // consume the (

      ArgumentExpressionList * pArgumentExpressionList = getArgumentExpressionList();

      expectAndConsumeToken(TOKEN_CLOSE_PAREN);

      addTermArgumentExpressionList(pArgumentExpressionList);
   }

}

ArgumentExpressionList * SpecParser::getArgumentExpressionList()
{
   ArgumentExpressionList * pArgumentExpressionList = new ArgumentExpressionList();
         
   if (matchToken(TOKEN_CLOSE_PAREN))
   {
      return pArgumentExpressionList; // empty list 
   }

   while(hasMoreTokensOnCurrentLine())
   {
      Expression * pExpression = getExpression();
      if (pExpression == 0)
      {
         reportError("Argument expression expected");
         break; // bail on first error 
      }

      if (pArgumentExpressionList->isFull())
      {
         reportError("Number of arguments exceeds builtin maximum");
         delete pExpression;
      }
      else
      {
         pArgumentExpressionList->addArgument(pExpression);
      }

      if (matchToken(TOKEN_COMMA) == false)
      {
         break; 
      }

      advance(); // consume the ,
   }
   return pArgumentExpressionList;
}

//void SpecParser::getOptionalProductionTermProbability()
//{
//   if (matchToken(TOKEN_OPEN_PAREN))
//   {
//      advance(); // consume the '('
//      expectToken(TOKEN_NUMBER);
//      float value = getTokenValue();
//      if ((value >= 0.0) && (value <= 1.0))
//      {
//         setTermProbability(value);
//      }
//      else
//      {
//         reportError("Probability value of of range: %;f",  value);
//      }
//      advance(); // consume the number
//      expectAndConsumeToken(TOKEN_CLOSE_PAREN);
//   }
//}

//void SpecParser::getOptionalProductionTermRepeats()
//{
//   if (matchToken(TOKEN_OPEN_CURLY_BRACE))
//   {
//      advance(); // consume the '('
//      while (hasMoreTokensOnCurrentLine() && matchToken(TOKEN_CLOSE_CURLY_BRACE) == false)
//      {
//         getTermRepeatRange();
//         if (matchToken(TOKEN_COMMA) == false)
//         {
//            break;
//         }
//         advance(); // consume the ','
//      }
//      expectAndConsumeToken(TOKEN_CLOSE_CURLY_BRACE);
//   }
//}


//void SpecParser::getTermRepeatRange()
//{
//   expectToken(TOKEN_NUMBER);
//   float valueMin = getTokenValue();
//   float valueMax = valueMin;
//   advance();
//   if (matchToken(TOKEN_RANGE))
//   {
//      advance(); // consume the .. 
//      expectToken(TOKEN_NUMBER);
//      valueMax = getTokenValue();
//      advance();
//   }

//   if ((valueMin < 0) || (valueMin > 99))
//   {
//      reportError("Repetition value is out of range: %d", int(valueMin) );
//   }
//   else if ((valueMax < 0) || (valueMax > 99))
//   {
//      reportError("Repetition value is out of range: %d", int(valueMax) );
//   }
//   else
//   {
//      setTermRepetition(valueMin, valueMax);
//   }
//}


void SpecParser::expectExpression(Expression * pExpression)
{
   if (pExpression == 0)
   {
      reportError("expression is incomplete");
   }
}




/*
exp			: assignment_exp
		| exp ',' assignment_exp
		;
*/

Expression * SpecParser::getExpression()
{
   Expression * pExpression = getAssignmentExpression();
   //while (matchToken(TOKEN_COMMA)
   return pExpression;
}


/*
assignment_exp		: conditional_exp
		| unary_exp assignment_operator assignment_exp
		;
assignment_operator	: '=' | '*=' | '/=' | '%=' | '+=' | '-=' | '<<='
		| '>>=' | '&=' | '^=' | '|='
		;
*/
Expression * SpecParser::getAssignmentExpression()
{
   Expression * pExpression = getConditionalExpression();
   // todo
   //if (pExpression == 0)
   //{
   //   pExpression = getUnaryExpression();
   //   // todo: get assignment op
   //   pAssignmentExpression = getAssignmentExpression;
   //   return new AssignmentExpression(...);
   //}
   return pExpression;
}

/*
conditional_exp		: logical_or_exp
		| logical_or_exp '?' exp ':' conditional_exp
		;
*/

Expression * SpecParser::getConditionalExpression()
{
   Expression * pExpression = getLogicalOrExpression();

   // TODO resolve ambiguity 
   //   ternary at the expression context is ambiguous with ternary at the production term context 
   //        { x < 10 ? A : B }
   //        { x = p < q ? 1 : 0 }
   //    maybe only allow ternary in ASSIGNMENT sub-context ? 
   //    or use @pick(expr,true_part,false_part) as a substitute 
   // --- 
   // if (matchToken(TOKEN_QUESTION_MARK))
   //{
   //   advance();// consume the ?
   //   Expression * pTruePart  = getExpression();
   //   expectAndConsumeToken(TOKEN_COLON);
   //   Expression * pFalsePart = getConditionalExpression();
   //   pExpression = new TernaryExpression(pExpression, pTruePart, pFalsePart);
   //}
   return pExpression;
}

/*
const_exp		: conditional_exp
		;
*/

/*
logical_or_exp		: logical_and_exp
		| logical_or_exp '||' logical_and_exp
		;
*/

Expression * SpecParser::getLogicalOrExpression()
{
   Expression * pExpression = getLogicalAndExpression();
   while (matchToken(TOKEN_VERTICAL_BAR) && peekToken(TOKEN_VERTICAL_BAR))
   {
      advance();// consume the |
      advance();// consume the |
      Expression * pRhs  = getLogicalAndExpression();
      expectExpression(pRhs);
      pExpression = new LogicalOrExpression(pExpression, pRhs);
   }
   return pExpression;
}

/*
logical_and_exp		: inclusive_or_exp
		| logical_and_exp '&&' inclusive_or_exp
		;
*/
Expression * SpecParser::getLogicalAndExpression()
{
   Expression * pExpression = getEqualityExpression();
   while (matchToken(TOKEN_AMPERSAND) && peekToken(TOKEN_AMPERSAND))
   {
      advance();// consume the &
      advance();// consume the &
      Expression * pRhs  = getEqualityExpression();
      expectExpression(pRhs);
      pExpression = new LogicalAndExpression(pExpression, pRhs);
   }
   return pExpression;
}

/*
inclusive_or_exp	: exclusive_or_exp
		| inclusive_or_exp '|' exclusive_or_exp
		;
*/

/*
exclusive_or_exp	: and_exp
		| exclusive_or_exp '^' and_exp
		;
*/

/*
and_exp			: equality_exp
		| and_exp '&' equality_exp
		;
*/

/*
equality_exp		: relational_exp
		| equality_exp '==' relational_exp
		| equality_exp '!=' relational_exp
		;
*/
Expression * SpecParser::getEqualityExpression()
{
   Expression * pExpression = getRelationalExpression();
   for(;;)
   {
      if (matchToken(TOKEN_EQUAL) && peekToken(TOKEN_EQUAL))
      {
         advance();// consume the =
         advance();// consume the =
         Expression * pRhs  = getRelationalExpression();
         expectExpression(pRhs);
         pExpression = new EqualsExpression(pExpression, pRhs);
      }
      else if (matchToken(TOKEN_BANG) && peekToken(TOKEN_EQUAL))
      {
         advance();// consume the =
         advance();// consume the =
         Expression * pRhs  = getRelationalExpression();
         expectExpression(pRhs);
         pExpression = new NotEqualsExpression(pExpression, pRhs);
      }
      else
      {
         break;
      }
   }
   return pExpression;
}

/*
relational_exp		: shift_expression
		| relational_exp '<' shift_expression
		| relational_exp '>' shift_expression
		| relational_exp '<=' shift_expression
		| relational_exp '>=' shift_expression
		;
*/
Expression * SpecParser::getRelationalExpression()
{
   Expression * pExpression = getAdditiveExpression();
   for (;;)
   {
      if (matchToken(TOKEN_OPEN_ANGLE_BRACKET))
      {
         if (peekToken(TOKEN_EQUAL))
         {
            advance(); // consume the <
            advance(); // consume the =
            Expression * pRhs = getAdditiveExpression();
            expectExpression(pRhs);
            pExpression = new LessThanOrEqualExpression(pExpression, pRhs);
         }
         else
         {
            advance(); // consume the <
            Expression * pRhs  = getAdditiveExpression();
            expectExpression(pRhs);
            pExpression = new LessThanExpression(pExpression, pRhs);
         }
      }
      else if (matchToken(TOKEN_CLOSE_ANGLE_BRACKET))
      {
         if (peekToken(TOKEN_EQUAL))
         {
            advance(); // consume the >
            advance(); // consume the =
            Expression * pRhs = getAdditiveExpression();
            expectExpression(pRhs);
            pExpression = new GreaterThanOrEqualExpression(pExpression, pRhs);
         }
         else
         {
            advance(); // consume the >
            Expression * pRhs  = getAdditiveExpression();
            expectExpression(pRhs);
            pExpression = new GreaterThanExpression(pExpression, pRhs);
         }
      }
      else
      {
         break;
      }
   }
   return pExpression;
}

/*
shift_expression	: additive_exp
		| shift_expression '<<' additive_exp
		| shift_expression '>>' additive_exp
		;
*/

/*
additive_exp		: mult_exp
		| additive_exp '+' mult_exp
		| additive_exp '-' mult_exp
		;
*/
Expression * SpecParser::getAdditiveExpression()
{
   Expression * pExpression = getMultiplicativeExpression();
   for (;;)
   {
      if (matchToken(TOKEN_PLUS))
      {
         advance(); // consume the +
         Expression * pRhs = getMultiplicativeExpression();
         expectExpression(pRhs);
         pExpression = new AdditionExpression(pExpression, pRhs);
      }
      else if (matchToken(TOKEN_MINUS))
      {
         advance(); // consume the -
         Expression * pRhs = getMultiplicativeExpression();
         expectExpression(pRhs);
         pExpression = new SubtractionExpression(pExpression, pRhs);
      }
      else
      {
         break;
      }
   }
   return pExpression;
}

/*
mult_exp		: cast_exp
		| mult_exp '*' cast_exp
		| mult_exp '/' cast_exp
		| mult_exp '%' cast_exp
		;
*/

Expression * SpecParser::getMultiplicativeExpression()
{
   Expression * pExpression = getUnaryExpression();
   for (;;)
   {
      if (matchToken(TOKEN_ASTERISK))
      {
         advance(); // consume the *
         Expression * pRhs = getUnaryExpression();
         expectExpression(pRhs);
         pExpression = new MultiplyExpression(pExpression, pRhs);
      }
      else if (matchToken(TOKEN_FORWARD_SLASH))
      {
         advance(); // consume the /
         Expression * pRhs = getUnaryExpression();
         expectExpression(pRhs);
         pExpression = new DivideExpression(pExpression, pRhs);
      }
      else if (matchToken(TOKEN_PERCENT))
      {
         advance(); // consume the %
         Expression * pRhs = getUnaryExpression();
         expectExpression(pRhs);
         pExpression = new ModuloExpression(pExpression, pRhs);
      }
      else
      {
         break;
      }
   }
   return pExpression;
}


/*
cast_exp		: unary_exp
		| '(' type_name ')' cast_exp
		;
*/

/*
unary_exp		: postfix_exp
		| '++' unary_exp
		| '--' unary_exp
		| unary_operator cast_exp
		| 'sizeof' unary_exp
		| 'sizeof' '(' type_name ')'
		;
unary_operator		: '&' | '*' | '+' | '-' | '~' | '!'
		;
*/

Expression * SpecParser::getUnaryExpression()
{
   // todo: check for prefix operators here 
   //   ++ 
   //   -- 
   Expression * pExpression; 
   if (matchToken(TOKEN_BANG))
   {
      advance(); // consume the !
      Expression * pRhs = getPostfixExpression();
      expectExpression(pRhs);
      pExpression = new LogicalNotExpression(pRhs);
   }
   else if (matchToken(TOKEN_AT))
   {
      pExpression = getFunctionExpression();
      expectExpression(pExpression);
   }
   else
   {
      pExpression = getPostfixExpression();
   }
      
   return pExpression;
}

/*
postfix_exp		: primary_exp
		| postfix_exp '[' exp ']'
		| postfix_exp '(' argument_exp_list ')'
		| postfix_exp '('			')'
		| postfix_exp '.' id
		| postfix_exp '->' id
		| postfix_exp '++'
		| postfix_exp '--'
		;
*/

Expression * SpecParser::getPostfixExpression()
{
   Expression * pExpression = getPrimaryExpression();

   // todo: check for postfix operators here
   //  ++
   //  -- 

   // TDB 

   return pExpression;
}


/*
primary_exp		: id
		| const
		| string
		| '(' exp ')'
		;
*/
Expression * SpecParser::getPrimaryExpression()
{
   Expression * pExpression = 0;

   if (matchToken(TOKEN_OPEN_PAREN))
   {
      advance(); // consume the '('
      pExpression = getExpression();
      expectAndConsumeToken(TOKEN_CLOSE_PAREN);
      pExpression = new ParenthesizedExpression(pExpression);
   }
   else if (matchToken(TOKEN_IDENT))
   {
      const char * pName = getTokenText();
      int idx = getFormalArgumentIndex(pName);
      if (idx < 0)
      {
         reportError("variable '%s' is not defined for this production", pName);
      }
      pExpression = new LocalVariableValue(pName, idx);
      advance(); // consume the token 
   }
   else if (matchToken(TOKEN_DOLLAR) && peekToken(TOKEN_IDENT))
   {
      advance(); // consume the $ 
      pExpression = new GlobalVariableValue( getTokenText() );
      advance(); // consume the token 
   }
   else if (matchToken(TOKEN_NUMBER))
   {
      float value = getTokenValue();
      pExpression = new ConstantValue(value);
      advance(); // consume the token 
   }
   else if (matchToken(TOKEN_MINUS) && peekToken(TOKEN_NUMBER))
   {
      advance(); // consume the -
      float value = 0.0 - getTokenValue();
      pExpression = new ConstantValue(value);
      advance(); // consume the number
   }
   return pExpression;
}

/*
argument_exp_list	: assignment_exp
		| argument_exp_list ',' assignment_exp
		;
*/

/*
const			: int_const
		| char_const
		| float_const
		| enumeration_const
		;
*/


Expression * SpecParser::getFunctionExpression()
{
   expectToken(TOKEN_AT);
   advance(); // consume the @

   expectToken(TOKEN_IDENT);
   const char * pName = getTokenText(); 
   FunctionDescription * pFunctionDescription = getFunctionDescriptionByName(pName);
   if (pFunctionDescription == 0)
   {
      reportError("no function named '%s'", pName);
      return 0; // ? 
   }
   advance(); // consume the function name 

   expectAndConsumeToken(TOKEN_OPEN_PAREN);

   ArgumentExpressionList * pArgumentExpressionList = getArgumentExpressionList();

   expectAndConsumeToken(TOKEN_CLOSE_PAREN);

   // todo: verify that arg list size == description->numArgs

   return new FunctionExpression(pFunctionDescription, pArgumentExpressionList);
}

//SymbolDescription * SpecParser::getBuiltinSymbolDescription(const char * pName)
//{
//   return mpLSystemUnderConstruction->getBuiltinSymbolByName(pName); 
//}

// #include <float.h> // DBL_MIN, DBL_MAX 
// SymbolDescription * SpecParser::findOrCreateSymbolDescription(const char * pName)
// {
//    //SymbolDescription * pDescription = mpLSystemUnderConstruction->getBuiltinSymbolByName(pName); 
//    //if (pDescription != 0)
//    //{
//    //   pDescription = mpLSystemUnderConstruction->getUserDefinedSymbolByName(pName); 
//    //   if (pDescription != 0)
//    //   {
//    //      int id = mpLSystemUnderConstruction->getUserDefinedSymbolDescriptions().getNumMembers();
//    //      mpLSystemUnderConstruction->addUserDefinedSymbol(id, pName, 0.0, DBL_MIN, DBL_MAX); 
//    //      pDescription = mpLSystemUnderConstruction->getUserDefinedSymbolById(id);
//    //   }
//    //}
//    //return pDescription;

//    // todo 
//    return 0;
// }

