#ifndef _tc_ascii_file_reader_h_
#define _tc_ascii_file_reader_h_  1

#include "Reader.hpp"
#include <string>
#include <cstdio>

class AsciiFileReader : public Reader
{
public:
   AsciiFileReader()
      : mpFile(0)
      , mLineNumber(0)
      , mPeekChar(0)
      , mPrevChar(0)
   {
   }

   virtual ~AsciiFileReader()
   {
      close();
   }

   void open(std::string path)
   {
      close();
      resetFlags();
      mFilename = path;
      mpFile = fopen(path.c_str(), "rb"); // read, binary 
      if (mpFile == 0)
      {
         setErrored();
         return;
      }

      mLineNumber = 1;
      mColumnNumber = 0;
      mPeekChar = fgetc(mpFile);
      if (ferror(mpFile))
      {
         setErrored();
         close();
      }
   }

   void close()
   {
      if (mpFile != 0)
      {
         fclose(mpFile);
         mpFile = 0;
         setEnd();
      }
   }

   bool isOpen()
   {
      return (mpFile != 0);
   }

   virtual char readChar() override
   {
      if (mpFile == 0)
      {
         setErrored();
         return EOF;
      }

      if (mPrevChar == '\n')
      {
         mLineNumber++;
         mColumnNumber = 0;
      }

      mColumnNumber++;

      char nextChar = mPeekChar;

      if (ferror(mpFile))
      {
         nextChar = EOF;
         setErrored();
         close();
      }
      if (feof(mpFile))
      {
         nextChar = EOF;
         setEnd();
         close();
      }

      mPeekChar = EOF;
      if (mpFile != 0)
      {
         mPeekChar = fgetc(mpFile);
      }

      mPrevChar = nextChar;

      return nextChar;
   }

   virtual char peekChar() override
   {
      return mPeekChar;
   }

   virtual int getLineNumber() override
   {
      return mLineNumber;
   }

   virtual int getColumnNumber() override
   {
      return mColumnNumber;
   }

private:
   FILE * mpFile; 
   int    mLineNumber; 
   int    mColumnNumber; 
   char   mPeekChar;
   char   mPrevChar;
   std::string mFilename;
};

#endif // _tc_ascii_file_reader_h_