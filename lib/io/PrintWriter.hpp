#ifndef _print_writer_h_
#define _print_writer_h_

#include <string>
#include <cstring>
#include <cstdarg>
#include <cstdio>

class PrintWriter
{
public:
   PrintWriter()
   {
   }

   virtual ~PrintWriter()
   {
   }

   virtual void printChars(const char * pStr) = 0; 
   virtual void printlnChars(const char * pStr = 0) = 0; 

   virtual void print(std::string str) {
      print(str.c_str());
   }

   virtual void println(std::string str) {
      print(str.c_str());
      println();
   } 

   virtual void print(const char * pFmt, ...)
   {
      char buf[256]; 
      va_list args;
      va_start(args, pFmt);
      vsnprintf(buf, sizeof(buf), pFmt, args);
      buf[ sizeof(buf) - 1 ] = 0;
      printChars(buf);
      va_end(args);
   }

   virtual void println(const char * pFmt = 0, ...)
   {
      if (pFmt) {
         char buf[312]; 
         va_list args;
         va_start(args, pFmt);
         vsnprintf(buf, sizeof(buf), pFmt, args);
         buf[ sizeof(buf) - 1 ] = 0;
         printlnChars(buf);
         va_end(args);
      }
      else {
         printlnChars();
      }
   }

};

class FilePrintWriter : public PrintWriter
{
public:
   FilePrintWriter(FILE * pFile)
      :mpFile(pFile)
   {
   }

   virtual ~FilePrintWriter()
   {
   }

   virtual void printChars(const char * pStr) override
   {
      if (pStr != 0)
      {
         fprintf(mpFile, "%s", pStr ? pStr : "<~NULL~>");
      }
   }

   virtual void printlnChars(const char * pStr) override
   {
      if (pStr != 0)
      {
         fprintf(mpFile, "%s", pStr ? pStr : "<~NULL~>");
      }
      print("\n");
   }

private:
   FILE * mpFile;
}; 




class HtmlFilePrintWriter : public FilePrintWriter
{
public:
   HtmlFilePrintWriter(FILE * pFile)
      : FilePrintWriter(pFile)
   {
   }

   virtual ~HtmlFilePrintWriter()
   {
   }

   virtual void printChars(const char * pStr) override
   {
      if (pStr != 0)
      {
         char buf[2];
         buf[1] = 0; // pre-position null terminator 
         while (*pStr != 0)
         {
            switch (*pStr)
            {
            case '<':  FilePrintWriter::print("&lt;"); break;
            case '>':  FilePrintWriter::print("&gt;"); break;
            case '&':  FilePrintWriter::print("&amp;"); break;
            case '"':  FilePrintWriter::print("&quot;"); break;
            // case ' ':  FilePrintWriter::print("&nbsp;"); break;
            default:   
               buf[0] = (*pStr);
               FilePrintWriter::print(buf);
               break;
            }

            pStr++;
         }
      }
   }

   virtual void printlnChars(const char * pStr = 0) override
   {
      if (pStr != 0)
      {
         print(pStr);
      }
      printNoEscape("<br>");
   }

   virtual void printNoEscape(const char * pStr)
   {
      FilePrintWriter::print(pStr);
   }

   virtual void printlnNoEscape(const char * pStr)
   {
      FilePrintWriter::println(pStr);
   }

   virtual void printHeader(int level, const char * pTitle)
   {
      char buf[ 16 ];
      sprintf(buf, "<h%d>", level);
      printNoEscape(buf);
      print(pTitle);
      sprintf(buf, "</h%d>", level);
      printNoEscape(buf);
   }

   void startItalics()
   {
      printNoEscape("<i>");
   }

   void endItalics()
   {
      printNoEscape("</i>");
   }

   void startBold()
   {
      printNoEscape("<b>");
   }

   void endBold()
   {
      printNoEscape("</b>");
   }

   void startStrike()
   {
      printNoEscape("<strike>");
   }

   void endStrike()
   {
      printNoEscape("</strike>");
   }

   void startColor(int color)
   {
      char buf[48];
      sprintf(buf, "<font color=#%06x>", color);
      printNoEscape(buf);
   }

   void endColor()
   {
      printNoEscape("</font>");
   }

   int makeColor(int red, int green, int blue)
   {
      return ((red   << 16) & 0x00ff0000) |
             ((green <<  8) & 0x0000ff00) | 
             ((blue       ) & 0x000000ff);
   }

private:
}; 

#endif // _print_writer_h_
