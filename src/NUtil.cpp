
#include <string.h>
#include "NUtil.hpp"

namespace NUtil
{

std::string busNameToObjPath
   (
   const DBUSIPC_tConstStr  busName
   )
{
   std::string result("/");
   
   if ( 0 != busName )
   {
      size_t len = strlen(busName);
      for ( size_t idx = 0; idx < len; ++idx )
      {
         DBUSIPC_tChar val = busName[idx];
         if ( '.' == val )
         {
            result.push_back('/');
         }
         else if ( (('A' <= val) && ('Z' >= val)) ||
                  (('a' <= val) && ('z' >= val)) ||
                  (('0' <= val) && ('9' >= val)) ||
                  ('_' == val) )
         {
            result.push_back(val);
         }
         else
         {
            result.push_back('_');
         }
      }
   }
   
   return result;
}

}  // End of namespace NUtil


