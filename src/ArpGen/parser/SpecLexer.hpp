#ifndef _tc_spec_lexer_h_
#define _tc_spec_lexer_h_  1

#include "./ArpToken.hpp"
#include "../../../lib/io/Reader.hpp"
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstring>


const int MAX_TOKEN_LENGTH = 32;

class Token
{
public:
   ArpTokenType mTokenType;
   char text[MAX_TOKEN_LENGTH + 1];  // cannot use SafeString if tokens are copied via t1 = t2 
   float mValue;  // 0 if not numeric 
   int mColumnNumber;
   int mLineNumber;
private:
};


template <int SIZE>
class CollectionBuffer 
{
public:
   CollectionBuffer()
   {
      clear();
   }

   void clear()
   {
      mBuf[0] = 0;
      mBuflen = 0;
   }

   void append(char ch)
   {
      if (mBuflen < int(sizeof(mBuf) - 1))
      {
         mBuf[mBuflen] = ch;
         mBuflen++;
         mBuf[mBuflen] = 0;
      }
   }

   char * get()
   {
      return mBuf;
   }

private:
   char mBuf[SIZE + 1];
   int  mBuflen;
};

class SpecLexer
{
public:
   SpecLexer(Reader & reader)
      :mReader(reader)
   {
      // initialize
      advance();
      advance();
   }

   virtual ~SpecLexer()
   {
   }

   void initialize(Reader & reader)
   {
   }

   Token & getCurrentToken() { return mCurrToken; } 
   Token & getPeekToken()    { return mPeekToken; }
   
   bool hasMore() 
   {
      return mReader.isErrored() == false && mCurrToken.mTokenType != ArpTokenType::TOKEN_END_OF_FILE;
   }

   bool isEnd()
   {
      return mReader.isErrored() || mCurrToken.mTokenType == ArpTokenType::TOKEN_END_OF_FILE;
   }

   int getLineNumber()
   {
      return mReader.getLineNumber(); 
   }

   int getColumnNumber()
   {
      return mReader.getColumnNumber(); 
   }

   void advance()
   {
      mCurrToken = mPeekToken;
      
      skipLeadingWhiteSpaceAndComments();

      char ch = mReader.readChar();
      if (ch == EOF)
      {
         setPeekToken(TOKEN_END_OF_FILE, "<eof>", 0.0);
      }
      else if (isalpha(ch))
      {
         collectIdentifier(ch);
      }
      else if (isdigit(ch))
      {
         collectNumber(ch);
      }
      else if (ch == '.')
      {
         char peekChar = mReader.peekChar();
         if (peekChar == '.')
         {
            mReader.readChar(); // consume the second '.'
            setPeekToken(TOKEN_RANGE, "..", 0.0);
         }
         else if (isdigit(peekChar))
         {
            collectNumber(ch);
         }
         else
         {
            setPeekToken(TOKEN_DOT, ".", 0.0);
         }
      }
      else if (ch == '\n')
      {
         setPeekToken(TOKEN_END_OF_LINE, "<eol>", 0.0);
      }
      else
      {
         ArpTokenDescription * pDesc = getPunctuation(ch);
         if (pDesc != 0)
         {
            setPeekToken(pDesc->mTokenType, pDesc->mpText, 0.0);
         }
         else
         {
            char buf[2]; 
            buf[0] = ch;
            buf[1] = 0;
            setPeekToken(TOKEN_UNKNOWN, buf, 0.0);
         }
      }
   }

private:
   Token mCurrToken; 
   Token mPeekToken;
   Reader & mReader;

   CollectionBuffer<MAX_TOKEN_LENGTH> mBuffer;


   void setPeekToken(ArpTokenType tokenType, const char * pText, float value)
   {
      mPeekToken.mTokenType = tokenType;
      strncpy(mPeekToken.text, pText, MAX_TOKEN_LENGTH);
      mPeekToken.text[MAX_TOKEN_LENGTH] = 0;
      mPeekToken.mValue = value;
      mPeekToken.mLineNumber = mReader.getLineNumber();
      mPeekToken.mColumnNumber = mReader.getColumnNumber();
   }

   void skipLeadingWhiteSpaceAndComments()
   {
      // skip white space 
      for(;;)
      {
         char ch = mReader.peekChar();
         if (ch == '\n')
         {
            break; // preserve newlines 
         }
         else if (isspace(ch))
         {
            mReader.readChar(); // consume the space 
         }
         else if (ch == ';')
         {
            // skip line comments
            while (mReader.hasMore() && mReader.peekChar() != '\n')
            {
               mReader.readChar();  // consume remainder of line up to but not including newline
            }
         }
         else
         {
            break;
         }
      }
   }

   void collectIdentifier(char firstChar)
   {
      mBuffer.clear();
      mBuffer.append(firstChar);

      for (;;)
      {
         char peekChar = mReader.peekChar();
         if (isalpha(peekChar) || isdigit(peekChar))
         {
            mReader.readChar(); // consume the char
            mBuffer.append(peekChar);
         }
         else
         {
            break;
         }
      }

      setPeekToken(TOKEN_IDENT, mBuffer.get(), 0.0);
   }

   void collectNumber(char firstChar)
   {
      mBuffer.clear();
      mBuffer.append(firstChar);

      char peekChar; 

      for (;;)
      {
         peekChar = mReader.peekChar();
         if (isdigit(peekChar))
         {
            mReader.readChar(); // consume the char
            mBuffer.append(peekChar);
         }
         else
         {
            break;
         }
      }

      peekChar = mReader.peekChar();
      if (peekChar == '.')
      {
         mReader.readChar(); // consume the char
         mBuffer.append(peekChar);
      }

      for (;;)
      {
         peekChar = mReader.peekChar();
         if (isdigit(peekChar))
         {
            mReader.readChar(); // consume the char
            mBuffer.append(peekChar);
         }
         else
         {
            break;
         }
      }

      float value = std::atof( mBuffer.get());

      setPeekToken(TOKEN_NUMBER, mBuffer.get(), value);
   }

};

#endif // _tc_spec_lexer_h_
