#ifndef _tc_arp_token_h_
#define _tc_arp_token_h_  1

enum ArpTokenType 
{
   TOKEN_IDENT,
   TOKEN_NUMBER,

   TOKEN_RANGE,  // '..'

   TOKEN_OPEN_PAREN,   // (
   TOKEN_CLOSE_PAREN,  // )
   
   TOKEN_OPEN_SQUARE_BRACKET,  // [
   TOKEN_CLOSE_SQUARE_BRACKET, // ]
   
   TOKEN_OPEN_CURLY_BRACE,    // { 
   TOKEN_CLOSE_CURLY_BRACE,   // } 
   
   TOKEN_OPEN_ANGLE_BRACKET,  // < 
   TOKEN_CLOSE_ANGLE_BRACKET, // > 

   TOKEN_BANG,
   TOKEN_AT,
   TOKEN_HASH,
   TOKEN_DOLLAR,
   TOKEN_PERCENT,
   TOKEN_CARET,
   TOKEN_AMPERSAND,
   TOKEN_ASTERISK,
   TOKEN_UNDERSCORE,
   TOKEN_DASH,  // same as MINUS
   TOKEN_MINUS = TOKEN_DASH,
   TOKEN_TILDE,
   TOKEN_PLUS,
   TOKEN_EQUAL,  // single equal sign 
   TOKEN_COLON,
   TOKEN_SEMICOLON,
   TOKEN_SINGLE_QUOTE,
   TOKEN_DOUBLE_QUOTE,
   TOKEN_COMMA,
   TOKEN_DOT,
   TOKEN_QUESTION_MARK,
   TOKEN_VERTICAL_BAR,
   TOKEN_FORWARD_SLASH,
   TOKEN_BACKSLASH,
   //
   TOKEN_END_OF_LINE,
   TOKEN_END_OF_FILE,
   //
   TOKEN_UNKNOWN,
   TOKEN_ERRORED
};

struct ArpTokenDescription
{
   const char  * mpText;
   ArpTokenType  mTokenType; 
   bool          mIsPunctuation;
}; 

extern ArpTokenDescription * getTokenDescription(ArpTokenType tokenType);
extern ArpTokenDescription * getPunctuation(char ch);
extern const char * getTokenTypeName(ArpTokenType tokenType);

#endif // _tc_arp_token_h_
