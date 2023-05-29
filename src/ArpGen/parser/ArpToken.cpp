#include "ArpToken.hpp"

ArpTokenDescription arpTokenDescriptionTable[] = 
{
   { "<identifier>", TOKEN_IDENT,  false },
   { "<number>",     TOKEN_NUMBER, false },
   { "..",           TOKEN_RANGE,  false }, // punct, but more than one char 

   { "(", TOKEN_OPEN_PAREN, true },
   { ")", TOKEN_CLOSE_PAREN, true },

   { "[", TOKEN_OPEN_SQUARE_BRACKET, true },
   { "]", TOKEN_CLOSE_SQUARE_BRACKET, true },
      
   { "{", TOKEN_OPEN_CURLY_BRACE, true },
   { "}", TOKEN_CLOSE_CURLY_BRACE, true },

   { "<", TOKEN_OPEN_ANGLE_BRACKET, true },
   { ">", TOKEN_CLOSE_ANGLE_BRACKET, true },
   { "!", TOKEN_BANG, true },
   { "@", TOKEN_AT, true },
   { "#", TOKEN_HASH, true },
   { "$", TOKEN_DOLLAR, true },
   { "%", TOKEN_PERCENT, true },
   { "^", TOKEN_CARET, true },
   { "&", TOKEN_AMPERSAND, true },
   { "*", TOKEN_ASTERISK, true },
   { "_", TOKEN_UNDERSCORE, true },
   { "-", TOKEN_DASH, true },  // same as Minus
   { "~", TOKEN_TILDE, true },
   { "+", TOKEN_PLUS, true },
   { "=", TOKEN_EQUAL, true },  // single equal sign 
   { ":", TOKEN_COLON, true },
   { ";", TOKEN_SEMICOLON, true },
   { "'", TOKEN_SINGLE_QUOTE, true },
   { "|", TOKEN_VERTICAL_BAR, true },
   { "\"", TOKEN_DOUBLE_QUOTE, true },
   { ",", TOKEN_COMMA, true },
   { ".", TOKEN_DOT, true },
   { "?", TOKEN_QUESTION_MARK, true },
   { "/", TOKEN_FORWARD_SLASH, true },
   { "\\", TOKEN_BACKSLASH, true },
   //
   { "<eol>", TOKEN_END_OF_LINE, false },
   { "<eof>", TOKEN_END_OF_FILE, false },
   { "<unknown>", TOKEN_UNKNOWN, false },
   { "<error>", TOKEN_ERRORED, false },  
   //
   { "", (ArpTokenType)-999, false } // end of table marker
};


ArpTokenDescription * getTokenDescription(ArpTokenType tokenType)
{
   ArpTokenDescription * pDesc = arpTokenDescriptionTable;
   while (pDesc->mpText[0] != 0)
   {
      if (pDesc->mTokenType == tokenType)
      {
         return pDesc;
      }
      pDesc++;
   }
   return 0; // no match 
}

const char * getTokenTypeName(ArpTokenType tokenType)
{
   ArpTokenDescription * pDesc = getTokenDescription(tokenType);
   if (pDesc != 0)
   {
      return pDesc->mpText;
   }
   return "<unknown-token-type>";
}

ArpTokenDescription * getPunctuation(char ch)
{
   ArpTokenDescription * pDesc = arpTokenDescriptionTable;
   while (pDesc->mpText[0] != 0)
   {
      if (pDesc->mIsPunctuation && pDesc->mpText[0] == ch)
      {
         return pDesc;
      }
      pDesc++;
   }
   return 0; // no match 
}
