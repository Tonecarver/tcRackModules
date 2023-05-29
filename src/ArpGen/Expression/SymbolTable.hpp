#ifndef _tc_symbol_table_h_
#define _tc_symbol_table_h_   1

#include "../../../lib/datastruct/List.hpp"
#include "../../../lib/datastruct/FreePool.hpp"
#include "../../../lib/datastruct/PtrArray.hpp"

#include <string>
#include <cassert>

// TODO: fix comment to be more general 
// Held by the LSystem to describe the symbols available/defined in the LSystem
class SymbolDescription
{
public:
   SymbolDescription(int id, const char * pName, float nominalValue, float minimumValue, float maximumValue, bool isBuiltin)
      : mId(id)
      , mName(pName)
      , mNominalValue(nominalValue) 
      , mMinimumValue(minimumValue)
      , mMaximumValue(maximumValue)
      , mIsBuiltin(isBuiltin)
   {
   }

   int    getId() const { return mId; } 
   const char * getName() const { return mName.c_str(); } 
   float getNominalValue() const { return mNominalValue; } 
   float getMinimumValue() const { return mMinimumValue; }
   float getMaximumValue() const { return mMaximumValue; } 
   bool   isBuiltin() const { return mIsBuiltin; } 

private:
   int mId; 
   std::string mName;
   float mNominalValue; 
   float mMinimumValue; 
   float mMaximumValue; 
   bool mIsBuiltin;
}; 






class Symbol : public ListNode
{
public:
   Symbol()
      : mName()
      , mValue(0.0)
      , mIsReadonly(false)
   {
   }

   void initialize(const char * pName, float value = 0.0, bool isReadonly = false)
   {
      assert(pName != 0);
      assert(pName[0] != 0);
      // todo: check no blanks?
      mName = pName;
      mValue = value;
      mIsReadonly = isReadonly;
   }

   const char * getName() const { return mName.c_str(); } 
   float getValue() const { return mValue; } 
   void setValue(float value) { mValue = value; } 

   bool isReadonly() const { return mIsReadonly; }
   void setReadonly(bool beReadonly) { mIsReadonly = beReadonly; } 

private:
   std::string mName;
   float     mValue; 
   bool       mIsReadonly;
};



class SymbolTable
{
public:
   SymbolTable()
   {
   }

   virtual ~SymbolTable()
   {
      mSymbols.deleteContents();
   }

   int addSymbol(const char * pName, float value = 0.0, bool isReadonly = false)
   {
      int id = getSymbolId(pName);
      if (id < 0)
      {
         Symbol * pSymbol = mSymbolFreePool.allocate();  
         pSymbol->initialize(pName, value, isReadonly);
         mSymbols.append(pSymbol);
         id = mSymbols.size();
      }
      return id;
   }

   int getSymbolId(const char * pName)
   {
      int numMembers = mSymbols.size(); 
      for (int i = 0; i < numMembers; i++)
      {
         Symbol * pSymbol = mSymbols.getAt(i);
         if (pSymbol->getName() == pName)
         {
            return i;
         }
      }
      return -1; // not found 
   }

   Symbol * getSymbolById(int id)
   {
      if (mSymbols.isValidIndex(id))
      {
         return mSymbols.getAt(id);
      }
      return 0;
   }

#ifdef INCLUDE_DEBUG_FUNCTIONS
   void showSymbols(const char * pHint)
   {
      DEBUG( "SymbolTable: %s -- num symbols = %d", (pHint == 0 ? "" : pHint), mSymbols.size() );
      int numMembers = mSymbols.size(); 
      for (int i = 0; i < numMembers; i++)
      {
         Symbol * pSymbol = mSymbols.getAt(i);
         DEBUG( "  sym[ %3d ]   %-12s = %lf   %s", i, pSymbol->getName(), pSymbol->getValue(), (pSymbol->isReadonly() ? "(readonly)" : "") );
      }
   }
#endif // INCLUDE_DEBUG_FUNCTIONS 

private:
   PtrArray<Symbol> mSymbols;
   FreePool<Symbol> mSymbolFreePool;
};






class SymbolReference : public ListNode // << this should probably be under the ValueExpression tree
{
public:
   SymbolReference(int symbolId, bool isGlobal)
      : mSymbolId(symbolId)
      , mIsGlobal(isGlobal)
   {
   }

   int getSymbolId() const
   {
      return mSymbolId;
   }

   bool isGlobal() const 
   {
      return mIsGlobal;
   }

   bool isLocal() const 
   {
      return (mIsGlobal == false);
   }

private:
   int  mSymbolId; 
   bool mIsGlobal;
};
// when parsing 
// on symbol reference
//   get or create symbol
//   store id and global flag
//   to resolve, look up value from context using flags 

#endif // _tc_symbol_table_h_
