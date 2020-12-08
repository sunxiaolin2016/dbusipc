
#include "Connection.hpp"

#include <list>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include "Dispatcher.hpp"
#include "Exceptions.hpp"
#include "SignalSubscription.hpp"
#include "ServiceRegistration.hpp"
#include "DBusErrorHolder.hpp"
#include "InterfaceDefs.hpp"
#include "Command.hpp"
#include "NSysDep.hpp"
#include "trace.h"


//
// Static initialization
//
Connection::tConnCache Connection::msConnCache;


Connection* Connection::create
   (
   DBUSIPC_tConstStr  address,
   DBUSIPC_tBool      openPrivate,
   Dispatcher*       disp
   )
{
   DBusErrorHolder dbusError;
   DBusConnection* dbusConn(0);
   Connection* conn(0);
   tConnCache::iterator it;
   
   if ( openPrivate )
   {
      dbusConn = dbus_connection_open_private(address ? address : "",
                                             dbusError.getInst());
      
   }
   else
   {
      dbusConn = dbus_connection_open(address ? address : "",
                                    dbusError.getInst());
   }
   
   if ( 0 == dbusConn )
   {
      DBusException e(dbusError.getName(), dbusError.getMessage());
      throw e;
   }
   else
   {
      // If we fail to register the connection then ...
      if ( !dbus_bus_register(dbusConn, dbusError.getInst()) )
      {
         if ( openPrivate )
         {
            dbus_connection_close(dbusConn);
         }
         dbus_connection_unref(dbusConn);
         
         DBusException e(dbusError.getName(), dbusError.getMessage());
         throw e;   
      }
   }
   
   // See if an existing connection already exists
   for ( it = msConnCache.begin(); it != msConnCache.end(); ++it )
   {
      if ( dbusConn == (*it).second )
      {
         break;
      }
   }
   
   // If this connection doesn NOT already exist in the cache then ...
   if ( msConnCache.end() == it )
   {
      conn = new Connection(dbusConn, openPrivate, disp);
   }
   else  /* Else this is a shared connection */
   {
      conn = (*it).first;
      conn->incRef();
   }
   
   return conn;
}

Connection* Connection::create
   (
   DBUSIPC_tConnType  connType,
   DBUSIPC_tBool      openPrivate,
   Dispatcher*       disp
   )
{
   DBusErrorHolder dbusError;
   DBusBusType busType(DBUS_BUS_SESSION); 
   DBusConnection* dbusConn(0);
   Connection* conn(0);
   tConnCache::iterator it;
   
   switch ( connType )
   {
      case DBUSIPC_CONNECTION_SESSION:
         busType = DBUS_BUS_SESSION;
         break;
         
      case DBUSIPC_CONNECTION_SYSTEM:
         busType = DBUS_BUS_SYSTEM;
         break;
         
      case DBUSIPC_CONNECTION_STARTER:
         busType = DBUS_BUS_STARTER;
         break;
      
      default:
         throw DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
               DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_BAD_ARGS),
               "Unknown connection type");
         break;
   }
   
   if ( openPrivate )
   {
      dbusConn = dbus_bus_get_private(busType, dbusError.getInst());
      
   }
   else
   {
      dbusConn = dbus_bus_get(busType, dbusError.getInst());
   }
   
   if ( 0 == dbusConn )
   {
      DBusException e(dbusError.getName(), dbusError.getMessage());
      throw e;
   }
   
   // See if an existing connection already exists
   for ( it = msConnCache.begin(); it != msConnCache.end(); ++it )
   {
      if ( dbusConn == (*it).second )
      {
         break;
      }
   }
   
   // If this connection doesn NOT already exist in the cache then ...
   if ( msConnCache.end() == it )
   {
      conn = new Connection(dbusConn, openPrivate, disp);
   }
   else  /* Else this is a shared connection */
   {
      conn = (*it).first;
      conn->incRef();
   }
   
   return conn;   
}


void Connection::release
   (
   Connection* conn
   )
{
   // Let's double-verify that the connection still exists in the
   // connection cache in case someone tries to release a connection
   // more times than it was acquired.
   DBusConnection* dbusConn = getDBusConnection(conn);
   if ( 0 != dbusConn )
   {
      conn->decRef();
   }
}


void Connection::forceReleaseAll()
{
   tConnCache::const_iterator it;
   
   while ( !msConnCache.empty() )
   {
      it = msConnCache.begin();
      while ( (*it).first->mRefCount > 1 )
      {
         (*it).first->decRef();
      }
      // Do the final decrement which deletes the connection
      (*it).first->decRef();
   }
}


DBusConnection* Connection::getDBusConnection
   (
   Connection* conn
   )
{
   DBusConnection* dbusConn(0);
   tConnCache::const_iterator it = msConnCache.find(conn);

   if ( it != msConnCache.end() )
   {
      dbusConn = (*it).second;
   }
   
   return dbusConn;
}


DBUSIPC_tError Connection::cancelPendingByHandle
   (
   DBUSIPC_tHandle hnd
   )
{
   DBUSIPC_tError status(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NOT_FOUND));
   
   for ( tConnCache::iterator it = msConnCache.begin();
      (it != msConnCache.end()) && (status != DBUSIPC_ERROR_NONE); ++it )
   {
      Connection* conn = (*it).first;
      for ( tPendingContainer::iterator cmdIt = conn->mPendingCmds.begin();
      cmdIt != conn->mPendingCmds.end(); ++cmdIt )
      {
         if ( (*cmdIt)->getHandle() == hnd )
         {
            // Cancel the pending command
            (*cmdIt)->cancel(*(conn->mDispatcher));
            // Destroy it
            delete *cmdIt;
            // Remove it from the collection of pending commands
            conn->mPendingCmds.erase(cmdIt);
            status = DBUSIPC_ERROR_NONE;
            break;
         }
      }
   }
   return status;
}

bool Connection::signalSubExists
   (
   SignalSubscription*  sigSub
   )
{
   bool found(false);
   
   for ( tConnCache::iterator it = msConnCache.begin();
      (it != msConnCache.end()) && !found; ++it )
   {
      Connection* conn = (*it).first;
      if ( conn->mSigSubscriptions.end() !=
         conn->mSigSubscriptions.find(sigSub) )
      {
         found = true;
      }
   }
   
   return found;
}


bool Connection::serviceRegExists
   (
   ServiceRegistration* svcReg
   )
{
   bool found(false);
   
   for ( tConnCache::iterator it = msConnCache.begin();
      (it != msConnCache.end()) && !found; ++it )
   {
      Connection* conn = (*it).first;
      if ( conn->mSvcRegistrations.end() !=
         conn->mSvcRegistrations.find(svcReg) )
      {
         found = true;
      }
   }
   
   return found;   
}


Connection::Connection
   (
   DBusConnection*   conn,
   bool              priv,
   Dispatcher*       disp
   )
   : mDBusConn(conn)
   , mPrivate(priv)
   , mRefCount(1)
   , mDispatcher(disp)
   , mSigSubscriptions()
   , mSvcRegistrations()
   , mPendingCmds()
   , mMaxDispatchProcTime(DBUSIPC_MAX_UINT64)
{
   std::string value = NSysDep::DBUSIPC_getenv("DBUSIPC_MAX_DISPATCH_PROC_TIME_MSEC");
   if ( !value.empty() )
   {
      mMaxDispatchProcTime = static_cast<uint64_t>(std::atol(value.c_str()));
      TRACE_INFO("Connection: maxDispatchProcTime=%" PRIu64, mMaxDispatchProcTime);
   }

   // We don't want the the program to exit when the connection is
   // disconnected.
   dbus_connection_set_exit_on_disconnect(mDBusConn, false);
   dbus_connection_set_dispatch_status_function(mDBusConn,
         Connection::onDispatchStatusUpdate, this, 0);
   
   try
   {
      dbus_bool_t status(FALSE);      
      status = dbus_connection_set_watch_functions(
            mDBusConn,
            Dispatcher::onAddWatchCallback,
            Dispatcher::onRemoveWatchCallBack,
            Dispatcher::onToggleWatchCallback,
            mDispatcher,
            0);
      if ( !status )
      {
         throw DBusException(DBUSIPC_ERR_NAME_NO_MEMORY,
                             "Unable to 'watch' callback handlers");
      }

      status = dbus_connection_set_timeout_functions(
            mDBusConn,
            Dispatcher::onAddTimeoutCallback,
            Dispatcher::onRemoveTimeoutCallback,
            Dispatcher::onToggleTimeoutCallback,
            mDispatcher,
            0);
      if ( !status )
      {
         throw DBusException(DBUSIPC_ERR_NAME_NO_MEMORY,
                             "Unable to 'watch' callback handlers");
      }
      
      // We want to monitor all signals the library receives
      if ( !dbus_connection_add_filter(mDBusConn, Connection::messageFilter,
         this, 0) )
      {
         throw DBusException(DBUSIPC_ERR_NAME_NO_MEMORY,
                             "Unable to add message filter");
      }

      msConnCache[this] = mDBusConn;
      mDispatcher->addPending(this);
   }
   catch ( ... )
   {
      if ( mPrivate )
      {
         dbus_connection_close(mDBusConn);
      }
      dbus_connection_unref(mDBusConn);
      throw;
   }
}
   
Connection::~Connection()
{   
   // *Try* to remove this connection from the list of connections managed
   // by the dispatcher with messages to dispatch
   mDispatcher->removePending(this);

   // Delete any pending commands
   while ( !mPendingCmds.empty() )
   {
      tPendingContainer::iterator it = mPendingCmds.begin();
      (*it)->cancel(*mDispatcher);
      delete (*it);
      mPendingCmds.erase(it);
   }
   
   // Delete any subscriptions that might be left around
   while ( !mSigSubscriptions.empty() )
   {
      tSigSubContainer::iterator it = mSigSubscriptions.begin();
      delete (*it);
      mSigSubscriptions.erase(it);
   }

   // Delete any service registrations that might be left around
   while ( !mSvcRegistrations.empty() )
   {
      tSvcRegContainer::iterator it = mSvcRegistrations.begin();
      delete (*it);
      mSvcRegistrations.erase(it);
   }
}


void Connection::incRef()
{
   ++mRefCount;
   
   // We let the static "create" function indirectly increment the
   // reference count of the underlying D-Bus connection associated
   // with this connection wrapper. This is done by the
   // dbus_connection_open_xxx or dbus_bus_get_xxxx functions.
}

void Connection::decRef()
{
   --mRefCount;
   //assert( 0 <= mRefCount );
   
   if ( 0 == mRefCount )
   {
      // If this connection is still connected to the daemon
      if ( dbus_connection_get_is_connected(mDBusConn) )
      {
         DBusErrorHolder dbusError;
         
         // Remove any outstanding subscriptions
         for ( tSigSubContainer::iterator it = mSigSubscriptions.begin();
                     it != mSigSubscriptions.end(); ++it )
         {
            (void)dbus_bus_remove_match(mDBusConn, (*it)->getRule(),
                                       dbusError.getInst());
         }
         
         // Release any well-known bus names we might own
         for ( tSvcRegContainer::iterator it = mSvcRegistrations.begin();
            it != mSvcRegistrations.end(); ++it )
         {
            (void)dbus_bus_release_name(mDBusConn, (*it)->getBusName(),
                                       dbusError.getInst());
         }
         
         // Flush the out-going message queue
         dbus_connection_flush(mDBusConn);
      }
      
      dbus_connection_remove_filter(mDBusConn, Connection::messageFilter,
                                    this);
      if ( mPrivate )
      {
         dbus_connection_close(mDBusConn);
      }
      
      msConnCache.erase(this);
   }
   
   // We also need to explicitly decrement the reference count on the
   // associated underlying D-Bus connection object. This *could* be
   // the last reference to this connection object so we don't want to
   // reference it again from this point forward (including the destructor).
   dbus_connection_unref(mDBusConn);

   if ( 0 == mRefCount )
   {
      delete this;
   }
}


DBusHandlerResult Connection::messageFilter
   (
   DBusConnection*   dbusConn,
   DBusMessage*      msg,
   void*             data
   )
{
   DBusHandlerResult result(DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
   assert( 0 != dbusConn );
   assert( 0 != msg );
   assert( 0 != data );
   DBUSIPC_tConstStr msgName(0);
   DBUSIPC_tConstStr payload(0);
   
   Connection* conn = static_cast<Connection*>(data);
   if ( Connection::connectionExists(conn) )
   {
      DBUSIPC_tConstStr msgType = "unknown";
      switch ( dbus_message_get_type(msg) )
      {
         case DBUS_MESSAGE_TYPE_METHOD_CALL:
            msgType = "method call";
            break;
         case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            msgType = "method return";
            break;
         case DBUS_MESSAGE_TYPE_ERROR:
            msgType = "error";
            break;
         case DBUS_MESSAGE_TYPE_SIGNAL:
            msgType = "signal";
            break;
         default:
            break;
      }
      
      TRACE_INFO("messageFilter: msg type = %s", msgType);
      if ( DBUS_MESSAGE_TYPE_ERROR == dbus_message_get_type(msg) )
      {
         TRACE_INFO("messageFilter: errMsg = %s",
                     dbus_message_get_error_name(msg) ?
                     dbus_message_get_error_name(msg) : "unknown");
      }
      else
      {
         TRACE_INFO("messageFilter: interface = %s",
                     dbus_message_get_interface(msg) ?
                     dbus_message_get_interface(msg) : "unknown");
         TRACE_INFO("messageFilter: path = %s",
                     dbus_message_get_path(msg) ?
                     dbus_message_get_path(msg) : "unknown");
         TRACE_INFO("messageFilter: member = %s",
                     dbus_message_get_member(msg) ?
                     dbus_message_get_member(msg) : "unknown");
      }
      TRACE_INFO("messageFilter: sender = %s", dbus_message_get_sender(msg) ?
                              dbus_message_get_sender(msg) : "unknown");
      
      if ( dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected") &&
          dbus_message_has_path(msg, DBUS_PATH_LOCAL) )
      {
         TRACE_INFO("messageFilter: %p disconnected by local bus", dbusConn);
         if ( conn->mPrivate )
         {
            dbus_connection_close(conn->mDBusConn);
         }
   
         result = DBUS_HANDLER_RESULT_HANDLED;
      }
      //
      // Else look for signals directed to us
      //
      else if ( DBUS_MESSAGE_TYPE_SIGNAL == dbus_message_get_type(msg) )
      {  
         // Dispatch the signal to all subscribers. There can be more than
         // one subscriber for each signal.
         for ( tSigSubContainer::iterator it =
            conn->mSigSubscriptions.begin();
            it != conn->mSigSubscriptions.end(); ++it )
         {
            //
            // NOTE: This ASSUMES that object paths are UNIQUE in the
            // system since there is no way to match on the well-known
            // bus name (since the unique bus name is stored in the
            // 'sender' field of the D-Bus message header).
            //
            if ( (*it)->dispatchIfMatch(msg, conn->mMaxDispatchProcTime) )
            {
               result = DBUS_HANDLER_RESULT_HANDLED;
            }
         }
      }
      // See if this is an incoming request (e.g. we're hosting services)
      else if ( dbus_message_is_method_call(msg, DBUSIPC_INTERFACE_NAME,
         DBUSIPC_INTERFACE_METHOD_NAME) )
      {
         // Fish out the (actual) method name and JSON encoded payload
         if ( !dbus_message_get_args(msg, 0, DBUS_TYPE_STRING, &msgName,
            DBUS_TYPE_STRING, &payload, DBUS_TYPE_INVALID) )
         {
            TRACE_ERROR("messageFilter: failed to decode method arguments");
         }
         else
         {
            // Find the service that should receive this message
            for ( tSvcRegContainer::iterator it =
               conn->mSvcRegistrations.begin();
               it != conn->mSvcRegistrations.end(); ++it )
            {
               ServiceRegistration* reg = (*it);
               if ( dbus_message_has_path(msg, reg->getObjectPath()) )
               {
                  try
                  {
                     // Call the service message handler
                     reg->dispatch(msg, msgName, payload, conn->mMaxDispatchProcTime);
                  }
                  catch ( const std::exception& e )
                  {
                     TRACE_WARN("messageFilter: caught exception => %s",
                                 e.what());
                  }
                  result = DBUS_HANDLER_RESULT_HANDLED;
                  break;
               }
            }
         }
      }
      else if ( dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE,
            INTROSPECTION_INTERFACE_METHOD_NAME) )
      {
         result = conn->introspect(msg);
      }
   }
   
   return result;   
}


DBusHandlerResult Connection::introspect
   (
   DBusMessage*   msg
   )
{
   assert( 0 != msg );
   
   DBusHandlerResult result(DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
   DBUSIPC_tConstStr objPath = dbus_message_get_path(msg);
   if ( 0 != objPath )
   {
      DBusMessage* reply = dbus_message_new_method_return(msg);
      if ( 0 != reply )
      {
         try
         {  
            std::string xml(DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
            
            bool objectRegistered(false);
            // Determine if this is the complete path to one of the registered
            // objects
            for ( tSvcRegContainer::iterator it = mSvcRegistrations.begin();
               it != mSvcRegistrations.end(); ++it )
            {
               ServiceRegistration* reg = (*it);
               if ( dbus_message_has_path(msg, reg->getObjectPath()) )
               {
                  xml.append("<node name=\"");
                  xml.append(objPath);
                  xml.append("\">\n");
                  reg->introspect(xml);
                  objectRegistered = true;
                  break;
               }
            }
            
            // If the requested object path did not matched a registered
            // object path then ..
            if ( !objectRegistered )
            {
               xml.append("<node>\n");
            }
            
            // Now look for any matching child nodes
            typedef std::list<std::string> tPathList;
            tPathList pathList;
            std::string prefix(objPath);
            // Make sure the prefix ends in a path separator
            if ( 0 == prefix.length() )
            {
               prefix.append("/");
            }
            else if ( (prefix.rfind('/') != (prefix.length() - 1)) )
            {
               prefix.append("/");
            }
            
            // Collect all the unique child object paths
            for ( tSvcRegContainer::iterator it = mSvcRegistrations.begin();
               it != mSvcRegistrations.end(); ++it )
            {
               ServiceRegistration* reg = (*it);
               if ( 0 == std::strncmp(prefix.c_str(), reg->getObjectPath(),
                  prefix.length()) )
               {
                  std::string child(reg->getObjectPath() + prefix.length());
                  child = child.substr(0, child.find("/"));
                  pathList.push_back(child);
               }
            }
            pathList.sort();
            pathList.unique();
            
            for ( tPathList::iterator it = pathList.begin();
               it != pathList.end(); ++it )
            {
               xml.append("   <node name=\"");
               xml.append(*it);
               xml.append("\"/>\n");
            }
            
            xml.append("</node>");
            
            DBUSIPC_tConstStr ifData = xml.c_str();
            if ( dbus_message_append_args(reply, DBUS_TYPE_STRING, &ifData,
               DBUS_TYPE_INVALID) )
            {
               dbus_uint32_t serialNum;
               if ( dbus_connection_send(mDBusConn, reply, &serialNum) )
               {
                  result = DBUS_HANDLER_RESULT_HANDLED;
               }
            }
         }
         catch ( ... )
         {
            TRACE_ERROR("introspect: unexpected exception caught");
         }
         
         // Release the reply message
         dbus_message_unref(reply);
      }
   }
   
   return result;
}


void Connection::onDispatchStatusUpdate
   (
   DBusConnection*      dbusConn,
   DBusDispatchStatus   newStatus,
   void*                data
   )
{
   Connection* conn = static_cast<Connection*>(data);
   assert( 0 != conn );
   
   switch ( newStatus )
   {
      case DBUS_DISPATCH_DATA_REMAINS:
         TRACE_INFO("onDispatchStatusUpdate: needs dispatching on dbusConn=%p",
                     dbusConn);
         if ( connectionExists(conn) )
         {
            conn->mDispatcher->addPending(conn);
         }
         break;

      case DBUS_DISPATCH_COMPLETE:
         TRACE_INFO("onDispatchStatusUpdate: dispatch complete for dbusConn=%p",
                     dbusConn);
         break;

      case DBUS_DISPATCH_NEED_MEMORY:
         TRACE_WARN("onDispatchStatusUpdate: dispatch needs memory for "
                     "dbusConn=%p", dbusConn);
         break;
         
      default:
         TRACE_WARN("onDispatchStatusUpdate: Unknown status = %d", newStatus);
         break;
   }
}


bool Connection::isReadyForDispatch()
{
   return dbus_connection_get_dispatch_status(mDBusConn) ==
           DBUS_DISPATCH_DATA_REMAINS;
}


bool Connection::dispatchMessages()
{
   DBusDispatchStatus status(DBUS_DISPATCH_COMPLETE);
   
   // Only dispatch messages while we're connected
   if ( dbus_connection_get_is_connected(mDBusConn) )
   {
      // Dispatch messages while data remains to be processed
      do
      {
         status = dbus_connection_dispatch(mDBusConn);
      }
      while ( DBUS_DISPATCH_DATA_REMAINS == status);
      
      // If the dispatch indicates there was an error
      //(DBUS_DISPATCH_NEED_MEMORY) then we won't say the dispatch was
      // complete. Perhaps by the next go-around memory will have been freed
      // and the messages can be delivered.
   }
   
   return (DBUS_DISPATCH_COMPLETE == status);
}


void Connection::subscribeSignal
   (
   SignalSubscription*  sigSub
   )
{
   assert( 0 != sigSub );
   mSigSubscriptions.insert(sigSub);
}

void Connection::unsubscribeSignal
   (
   SignalSubscription*  sigSub
   )
{
   assert( 0 != sigSub );

   // If the signal was not found then ...
   if ( mSigSubscriptions.end() == mSigSubscriptions.find(sigSub) )
   {
      throw DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                        DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_BAD_ARGS),
                        "Subscription does not exist");
   }
   else
   {
      mSigSubscriptions.erase(sigSub);
      delete sigSub;
   }
}


void Connection::registerService
   (
   ServiceRegistration* reg
   )
{   
   assert( 0 != reg );
   mSvcRegistrations.insert(reg);
}


void Connection::unregisterService
   (
   ServiceRegistration* reg
   )
{
   DBusErrorHolder dbusError;

   assert( 0 != reg );

   // If the signal was not found then ...
   if ( mSvcRegistrations.end() == mSvcRegistrations.find(reg) )
   {
      throw DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                        DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_BAD_ARGS),
                        "Service registration does not exist");
   }
   else
   {
      mSvcRegistrations.erase(reg);
      delete reg;
   }
}


void Connection::registerPending
   (
   BaseCommand*   cmd
   )
{
   mPendingCmds.insert(cmd);
}


void Connection::unregisterPending
   (
   BaseCommand*   cmd
   )
{
   mPendingCmds.erase(cmd);
}
   
