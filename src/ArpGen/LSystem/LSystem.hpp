#pragma once
#ifndef _tc_lsystem_h_
#define _tc_lsystem_h_ 1

// #include "../../plugin.hpp"

#include "LSystemProduction.hpp"
#include "LSystemProductionGroup.hpp"

#include "../Expression/Expression.hpp"
#include "../Expression/ExpressionContext.hpp"
#include "../Expression/ArgumentList.hpp"

#include "LSystemTermCounter.hpp"

#include "../../../lib/datastruct/List.hpp"
#include "../../../lib/datastruct/FreePool.hpp"
#include "../../../lib/random/FastRandom.hpp"
#include "../../../lib/io/PrintWriter.hpp"
#include "../../../lib/io/ByteStream.hpp"

//#include "ResizableBuffer.h"
//#include "FastMath.h"
// #include "debugtrace.h"

#include <string>
#include <cstdio>

enum LSystemExecuteDirection { 
   DIRECTION_REVERSE,        /// order matches left-to-right placement
   DIRECTION_FORWARD_REVERSE,
   DIRECTION_FORWARD,
   //
   kNum_EXECUTE_DIRECTIONS
}; 

extern const char * lsystemExecuteDirectionNames[LSystemExecuteDirection::kNum_EXECUTE_DIRECTIONS]; 

class LSystem
{
public:
   LSystem()
   {
   }

   virtual ~LSystem()
   {
      //mBuiltinSymbols.deleteContents();
      mUserDefinedSymbols.deleteContents();
   }

   // maybe move these to a reader/writer class ? 
   static const char INFO_KEYWORD [];  // TODO: not used, remove ? 
   static const char DEPTH_KEYWORD []; // TODO: not used, remove ? 

   // todo: make these
   //   OVERSTEP = CYCLE | REFLECT 
   //   OCTAVE_MAX = number of octaves to go upward before applying overstep rule 
   //   OCTAVE_MIN = number of octaves to go downward before applying overstep rule 
   static const char BOUNDS_KEYWORD []; // TODO: not used, remove ? 
   static const char FLOOR_KEYWORD []; // TODO: not used, remove ? 
   static const char CEILING_KEYWORD []; // TODO: not used, remove ? 

   // todo: add
   //   PLAY_DIRECTION = FWD | REV | FWD_REV = direction to execute the expanded pattern 



   static const char CYCLE_KEYWORD []; // TODO: not used, remove ? 
   static const char REFLECT_KEYWORD []; // TODO: not used, remove ? 


   void addProduction(LSystemProduction * pProduction)
   {
      assert(pProduction != 0);

      std::string name = pProduction->getName();
      LSystemProductionGroup * pGroup = getProductionGroup(name);
      if (pGroup == 0)
      {
         pGroup = new LSystemProductionGroup(name);
         mProductions.append(pGroup);
      }
      pGroup->addProduction(pProduction);
   }


   LSystemProductionGroup * getProductionGroup(std::string name) const
   {
      DoubleLinkListIterator<LSystemProductionGroup> iter(mProductions);
      while (iter.hasMore())
      {
         LSystemProductionGroup * pProductionGroup = iter.getNext();
         if (pProductionGroup->getName() == name)
         {
            return pProductionGroup;
         }
      }
      return 0; // not found 
   }

   LSystemTerm * getTermById(int id)
   {
      DoubleLinkListIterator<LSystemProductionGroup> iter(mProductions);
      while (iter.hasMore())
      {
         LSystemProductionGroup * pProductionGroup = iter.getNext();
         LSystemTerm * pTerm = pProductionGroup->getTermById(id);
         if (pTerm != 0)
         {
            return pTerm;
         }
      }
      return 0; // not found 
   }

   //void addBuiltinSymbol(int id, const char * pName, float nominalValue, float minimumValue, float maximumValue)
   //{
   //   assert(id >= 0);

   //   if (id < mBuiltinSymbols.size())
   //   {
   //      if (mBuiltinSymbols.getAt(id) != 0) // already defined 
   //      {
   //         // todo log
   //         return;
   //      }
   //   }

   //   SymbolDescription * pDescription = new SymbolDescription(id, pName, nominalValue, minimumValue, maximumValue, true);
   //   mBuiltinSymbols.ensureMinimumSize(id);
   //   mBuiltinSymbols.putAt(id, pDescription);
   //}

   void addUserDefinedSymbol(int id, const char * pName, float nominalValue, float minimumValue, float maximumValue)
   {
      assert(id >= 0);

      if (id < mUserDefinedSymbols.size())
      {
         if (mUserDefinedSymbols.getAt(id) != 0) // already defined 
         {
            // todo log
            return;
         }
      }

      SymbolDescription * pDescription = new SymbolDescription(id, pName, nominalValue, minimumValue, maximumValue, false);
      mUserDefinedSymbols.ensureMinimumSize(id);
      mUserDefinedSymbols.putAt(id, pDescription);
   }

   //const PtrArray<SymbolDescription> & getBuiltinSymbolDescriptions() const
   //{
   //   return mBuiltinSymbols;
   //}

   const PtrArray<SymbolDescription> & getUserDefinedSymbolDescriptions() const
   {
      return mUserDefinedSymbols;
   }

   //SymbolDescription * getBuiltinSymbolByName(const char * pName) const
   //{
   //   return getSymbolByName(mBuiltinSymbols, pName);
   //}

   SymbolDescription * getUserDefinedSymbolByName(const char * pName) const
   {
      return getSymbolByName(mUserDefinedSymbols, pName);
   }

   SymbolDescription * getUserDefinedSymbolById(int id) const
   {
      if (mUserDefinedSymbols.isValidIndex(id))
      {
         return mUserDefinedSymbols.getAt(id);
      }
      return 0; // not found 
   }

   void clear() 
   {
      mUserDefinedSymbols.deleteContents();
      mProductions.deleteMembers(); 
   }

   void write(PrintWriter & writer)
   {
      writer.println("Attributes here ... (todo)");
      // todo: print number of productions  etc 

      DoubleLinkListIterator<LSystemProductionGroup> iter(mProductions);
      while (iter.hasMore())
      {
         LSystemProductionGroup * pProductionGroup = iter.getNext();
         pProductionGroup->write(writer);
      }
   }

   bool serialize(ByteStreamWriter * pWriter);
   bool deserialize(ByteStreamReader * pReader);

private:
   // attributes 
   // description 
   // .... 

   // PtrArray<SymbolDescription> mBuiltinSymbols;
   PtrArray<SymbolDescription> mUserDefinedSymbols;

   DoubleLinkList<LSystemProductionGroup> mProductions;

   SymbolDescription * getSymbolByName(const PtrArray<SymbolDescription> & symbols, std::string name) const
   {
      PtrArrayIterator<SymbolDescription> iter(symbols);
      while (iter.hasMore())
      {
         SymbolDescription * pDescription = iter.getNext();
         if (pDescription->getName() == name)
         {
            return pDescription;
         }
      }
      return 0; // not found 
   }

};

#endif // __tc_lsystem_h_