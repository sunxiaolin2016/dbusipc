

#include "RequestContext.hpp"
#include "Exceptions.hpp"
#include "Connection.hpp"
#include "dbus/dbus.h"
#include "trace.h"

RequestContext::RequestContext
   (
   Connection*    conn,
   DBusMessage*   request
   )
   : mConn(conn)
   , mReqMsg(dbus_message_ref(request))
{
   if ( 0 == mReqMsg )
   {
      throw DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                        DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                        "Failed to construct request context");
   }
}

   
RequestContext::~RequestContext()
{
   dbus_message_unref(mReqMsg);
}


DBUSIPC_tError RequestContext::sendReply
   (
   DBUSIPC_tConstStr  result
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   dbus_uint32_t serial;
   
   DBusConnection* dbusConn = Connection::getDBusConnection(mConn);
   if ( (0 == dbusConn) || !dbus_connection_get_is_connected(dbusConn) )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_NOT_CONNECTED);
   }
   else
   {
      DBusMessage* reply = dbus_message_new_method_return(mReqMsg);
      if ( 0 == reply )
      {
         TRACE_WARN("sendReply: failed to create reply message");
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_NO_MEMORY);
      }
      else
      {
         DBUSIPC_tConstStr empty = "";
         if ( !dbus_message_append_args(reply, DBUS_TYPE_STRING,
            result ? &result : &empty, DBUS_TYPE_INVALID) )
         {
            TRACE_WARN("sendReply: failed to append arguments");
            status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_NO_MEMORY);
         }
         else
         {
            // Enqueue the message for delivery
            if ( !dbus_connection_send(dbusConn, reply, &serial) )
            {
               TRACE_WARN("sendReply: failed to send reply message");
               status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NO_MEMORY);
            }
            
            // Free up the reply message
            dbus_message_unref(reply);
         }
      }
   }
   
   return status;
}

DBUSIPC_tError RequestContext::sendError
   (
   DBUSIPC_tConstStr  errName,
   DBUSIPC_tConstStr  errMsg
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   dbus_uint32_t serial;
   
   DBusConnection* dbusConn = Connection::getDBusConnection(mConn);
   if ( (0 == dbusConn) || !dbus_connection_get_is_connected(dbusConn) )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_NOT_CONNECTED);
   }
   else
   {
      DBusMessage* error = dbus_message_new_error(mReqMsg,
            errName ? errName : DBUS_ERROR_FAILED,
            errMsg ? errMsg : "D-Bus request failed");
      if ( 0 == error )
      {
         TRACE_WARN("sendError: failed to create error message");
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_NO_MEMORY);
      }
      else
      {
         // Enqueue the message for delivery
         if ( !dbus_connection_send(dbusConn, error, &serial) )
         {
            TRACE_WARN("sendError: failed to send error message");
            status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_NO_MEMORY);
         }

         // Free up the error message
         dbus_message_unref(error);
      }
   }
   
   return status;
}


