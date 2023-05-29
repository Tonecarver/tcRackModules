#ifndef _tc_byte_stream_h_ 
#define _tc_byte_stream_h_   1


#include <string> 

class ByteStreamWriter
{
public:
   ByteStreamWriter()   { } 
   virtual ~ByteStreamWriter()   { } 

   virtual void writeBoolean(bool val) = 0;
   virtual void writeChar(char ch) = 0;
   virtual void writeDouble(double val) = 0;
   virtual void writeFloat(float val) = 0;
   virtual void writeInt(int val) = 0;
   virtual void writeString(const char * pStr) = 0;
   virtual void writeString(std::string str) = 0;
   virtual int size() = 0;
   virtual unsigned char * get() = 0;
private:
};

class ByteStreamReader
{
public:
   ByteStreamReader() { }
   virtual ~ByteStreamReader() {  }

   virtual bool readBoolean(bool & val) = 0;
   virtual bool readChar(char & ch) = 0;
   virtual bool readDouble(double & val) = 0;
   virtual bool readFloat(float & val) = 0;
   virtual bool readInt(int & val)  = 0;              // int32, int64 versions
   virtual bool readString(std::string & str) = 0;
   virtual int size() = 0;
   virtual unsigned char * get() = 0;
private:
};

#endif // _tc_byte_stream_h_ 

