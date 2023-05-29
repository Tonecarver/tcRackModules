#ifndef _tc_function_type_h_
#define _tc_function_type_h_ 1

#include <string>

enum FunctionType 
{
   FUNC_INCR,
   FUNC_DECR,
   FUNC_ZERO,
   FUNC_PICK,
   FUNC_PROB,
   FUNC_RAND,
   //
   kNUM_FUNC_TYPES
}; 

class FunctionDescription
{
public:
   FunctionType mFunctionType;
   std::string  mName;
   int          mNumArguments;
}; 

extern FunctionDescription * getFunctionDescriptionByName(std::string name);
extern FunctionDescription * getFunctionDescriptionByType(FunctionType functionType);

//class FunctionArguments
//{
//public:
//   enum {
//      MAX_ARGUMENTS = 
//   };
//
//   FunctionArguments()
//   {
//      memset(mArgumentValues, 0, sizeof(mArgumentValues));
//      mNumArguments = 0;
//   }
//
//   bool isValidIndex(int idx) const
//   {
//      return ((0 <= idx) && (idx < MAX_ARGUMENTS));
//   }
//
//   void setAt(int idx, float value)
//   {
//      if (isValidIndex(idx))
//      {
//         mArgumentValues[ idx ] = value; 
//      }
//   }
//
//   float getAt(int idx)
//   {
//      if (isValidIndex(idx))
//      {
//         return mArgumentValues[ idx ]; 
//      }
//      return 0.0;
//   }
//
//   void setNumArguments(int numArgs)
//   {
//      mNumArguments = numArgs;
//      if (mNumArguments < 0)
//      {
//         mNumArguments = 0;
//      }
//      else if (mNumArguments >= MAX_ARGUMENTS)
//      {
//         mNumArguments = MAX_ARGUMENTS;
//      }
//   }
//
//   int getNumArguments() const
//   {
//      return mNumArguments;
//   }
//
//private:
//   float mArgumentValues[ MAX_ARGUMENTS ]; 
//   int mNumArguments; 
//};


#endif // _tc_function_type_h_
