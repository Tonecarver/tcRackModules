#include "StringUtil.hpp"

#include <ctype.h> // tolower

int startsWithIgnoreCase(const char * p1, const char * p2) {
   int matchCount = 0;
   while (p1[0] && p2[0] && tolower(p1[0]) == tolower(p2[0])) {
      matchCount++; 
      p1++;
      p2++;
   }
   return matchCount;
}
