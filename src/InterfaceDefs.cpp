
#include "InterfaceDefs.hpp"
#include "dbus/dbus.h"

DBUSIPC_tConstStr DBUSIPC_INTERFACE_NAME = "com.hsae.dbusipc";
DBUSIPC_tConstStr DBUSIPC_INTERFACE_SIGNAL_NAME = "Emit";
DBUSIPC_tConstStr DBUSIPC_INTERFACE_METHOD_NAME = "Invoke";
DBUSIPC_tConstStr DBUSIPC_INTERFACE_ERROR_NAME = "com.hsae.service.Error";
const DBUSIPC_tChar DBUSIPC_INTERFACE_SIGNAL_SIGNATURE[] = 
                     {DBUS_TYPE_STRING, DBUS_TYPE_STRING, DBUS_TYPE_INVALID};
DBUSIPC_tConstStr INTROSPECTION_INTERFACE_METHOD_NAME = "Introspect";
