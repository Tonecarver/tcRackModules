
#include "ArgumentList.hpp"
#include "../../plugin.hpp"

/////////////////////////////////////////////////////////////////////////
//   ArgumentExpressionList

bool ArgumentExpressionList::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);

   pWriter->writeInt(mArgumentExpressions.size());

   DoubleLinkListIterator<Expression> iter(mArgumentExpressions);
   while (iter.hasMore())
   {
      Expression * pExpression = iter.getNext();
      pExpression->serialize(pWriter);
   }
   return true;
}


bool ArgumentExpressionList::unserialize(ByteStreamReader * pReader)
{
   assert(pReader != 0);

   int numExpressions;
   pReader->readInt(numExpressions);
   for (int i = 0; i < numExpressions; i++)
   {
      Expression * pExpression = Expression::unpack(pReader);
      if (pExpression == 0)
      {
         DEBUG( "ArgumentExpressionList: error unpacking expression[%d] .. ", i );
         return false;
      }
      mArgumentExpressions.append(pExpression);
   }
   return true;
}

/////////////////////////////////////////////////////////////////////////
//   FormalArgumentList

bool FormalArgumentList::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);

   pWriter->writeInt(mNumMembers);
   for (int i = 0; i < mNumMembers; i++)
   {
      std::string name = mArgumentNames[i];
      pWriter->writeString(name);
   }
   return true;
}

bool FormalArgumentList::deserialize(ByteStreamReader * pReader)
{
   assert(pReader != 0);

   pReader->readInt(mNumMembers);
   // TODO: truncate num membres if > MAX 
   for (int i = 0; i < mNumMembers; i++)
   {
      std::string name;
      bool isOkay = pReader->readString( name );
      if (isOkay == false)
      {
         DEBUG( "FormalArgumentList: error unpacking argname[%d] .. ", i );
         return false;
      }
      mArgumentNames[i] = name;
   }
   return true;
}

/////////////////////////////////////////////////////////////////////////
//   ArgumentValuesList

bool ArgumentValuesList::serialize(ByteStreamWriter * pWriter)
{
   assert(pWriter != 0);

   pWriter->writeInt(mNumMembers);
   for (int i = 0; i < mNumMembers; i++)
   {
      pWriter->writeFloat(mArgumentValues[i]);
   }

   return true;
}

bool ArgumentValuesList::deserialize(ByteStreamReader * pReader)
{
   assert(pReader != 0);

   pReader->readInt(mNumMembers);
   for (int i = 0; i < mNumMembers; i++)
   {
      float val;
      pReader->readFloat( val );
      mArgumentValues[i] = val;
   }
   return true;
}
