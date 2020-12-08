

#include <assert.h>
#include "ServiceRegistration.hpp"
#include "Connection.hpp"
#include "NUtil.hpp"
#include "RequestContext.hpp"
#include "NSysDep.hpp"
#include "dbus/dbus.h"

ServiceRegistration::ServiceRegistration
   (
   Connection*             conn,
   const std::string&      busName,
   const std::string&      objPath,
   uint32_t                flag,
   DBUSIPC_tRequestCallback onRequest,
   DBUSIPC_tUserToken       token
   )
   : mConn(conn)
   , mBusName(busName)
   , mObjectPath(objPath)
   , mFlags(flag)
   , mOnRequest(onRequest)
   , mUserToken(token)
{
   if ( mObjectPath.empty() )
   {
      mObjectPath = NUtil::busNameToObjPath(busName.c_str());
   }

}

ServiceRegistration::~ServiceRegistration()
{
}


void ServiceRegistration::dispatch
   (
   DBusMessage*      reqMsg,
   DBUSIPC_tConstStr  method,
   DBUSIPC_tConstStr  parms,
   uint64_t          timeout
   )
{
   // If there is a callback to invoke then ...
   if ( 0 != mOnRequest )
   {
      // The owner of the callback will own the request context and it's
      // their responsibility to free it.
      RequestContext* reqCtx = new RequestContext(mConn, reqMsg);
      assert( 0 != reqCtx );
      assert( 0 != method );
      assert( 0 != parms );
      uint64_t now = NSysDep::DBUSIPC_getSystemTime();
      mOnRequest(reqCtx, method, parms, dbus_message_get_no_reply(reqMsg),
                 mUserToken);
      uint64_t elapsed = NSysDep::DBUSIPC_getSystemTime() - now;
      if (  elapsed > timeout )
      {
         NSysDep::DBUSIPC_slog(NSysDep::SLOG_SEV_WARNING,
               "Failed to process D-Bus method (%s) within %" PRIu64 " msec [PID=%u]",
               method, timeout, NSysDep::DBUSIPC_getProcId());
      }
   }
}


void ServiceRegistration::introspect
   (
   std::string&   xml
   )
{
   xml.append(
      "   <interface name=\"com.hsae.ServiceIpc\">\n"
      "      <method name=\"Invoke\">\n"
      "         <arg name=\"method\" type=\"s\" direction=\"in\"/>\n"
      "         <arg name=\"parameters\" type=\"s\" direction=\"in\"/>\n"
      "         <arg name=\"result\" type=\"s\" direction=\"out\"/>\n"
      "      </method>\n"
      "      <signal name=\"Emit\">\n"
      "         <arg name=\"name\" type=\"s\"/>\n"
      "         <arg name=\"data\" type=\"s\"/>\n"
      "      </signal>\n"
      "   </interface>\n"
      "   <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
      "      <method name=\"Introspect\">\n"
      "         <arg direction=\"out\" type=\"s\" name=\"data\"/>\n"
      "      </method>\n"
      "   </interface>\n");
}


