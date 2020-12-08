#ifndef DBUSIPC_PRIV_H_
#define DBUSIPC_PRIV_H_

#include <dbus/dbus.h>
#include "dbusipc/dbusipc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DBUSIPC_sReqContext
{
   DBusMessage* msg;
};

#ifdef __cplusplus
}
#endif
   
#endif /* Guard for DBUSIPC_PRIV_H_ */
