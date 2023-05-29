#ifndef _tc_reader_h_
#define _tc_reader_h_ 1

class Reader
{
public:
   Reader()
      : mIsErrored(false)
      , mIsEnd(false)
   {
   }

   virtual ~Reader()
   {
   }

   virtual char readChar() = 0;
   virtual char peekChar() = 0;
   virtual int getLineNumber() = 0;
   virtual int getColumnNumber() = 0;

   bool isEnd() { return mIsEnd; }
   bool isErrored() { return mIsErrored; } 

   bool hasMore()
   {
      return mIsEnd == false && mIsErrored == false;
   }

protected:
   void setErrored()
   {
      mIsErrored = true;
   }
   void setEnd()
   {
      mIsEnd = true;
   }
   void resetFlags()
   {
      mIsEnd = false; 
      mIsErrored = false;
   }
private:
   bool mIsErrored; 
   bool mIsEnd;
};

#endif // _tc_reader_h_ 1
