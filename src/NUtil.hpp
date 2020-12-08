#ifndef NUTIL_HPP_
#define NUTIL_HPP_

#include <string>
#include "dbusipc/dbusipc.h"

namespace NUtil
{
   std::string busNameToObjPath(const DBUSIPC_tConstStr busName);
}

#endif /* Guard for NUTIL_HPP_ */
