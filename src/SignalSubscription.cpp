#include "SignalSubscription.hpp"

#include <cassert>
#include <cstring>
#include "dbus/dbus.h"
#include "trace.h"

#include "InterfaceDefs.hpp"
#include "NSysDep.hpp"

SignalSubscription::SignalSubscription
   (
   Connection* conn
   )
   : mConn(conn)
{
   
}
   
   
SignalSubscription::~SignalSubscription()
{
}


NameOwnerChangedSubscription::NameOwnerChangedSubscription
   (
   Connection*                      conn,
   const std::string&               busName,
   DBUSIPC_tNameOwnerChangedCallback onNameChange,
   DBUSIPC_tUserToken                token
   )
   : SignalSubscription(conn)
   , mRule()
   , mBusName(busName)
   , mOnNameChange(onNameChange)
   , mUserToken(token)
{
      std::stringstream buffer;
      // If no bus name is specified we'll match on ANY bus name change
      if ( mBusName.empty() )
      {
         buffer << "type='signal',interface='" << DBUS_INTERFACE_DBUS <<
               "',member='NameOwnerChanged',path='" <<
               DBUS_PATH_DBUS << "'"; 
      }
      // Else we only monitor the activity of the named bus
      else
      {
         buffer << "type='signal',interface='" << DBUS_INTERFACE_DBUS <<
               "',member='NameOwnerChanged',path='" <<
               DBUS_PATH_DBUS << "',arg0='" << mBusName.c_str() << "'";
      }
      
      mRule = buffer.str();
}

NameOwnerChangedSubscription::~NameOwnerChangedSubscription()
{
}

const char* NameOwnerChangedSubscription::getRule() const
{
   return mRule.c_str();
}

bool NameOwnerChangedSubscription::dispatchIfMatch
   (
   DBusMessage*   msg,
   uint64_t       timeout
   )
{
   bool match(false);
   DBUSIPC_tConstStr newName(0);
   DBUSIPC_tConstStr oldOwner(0);
   DBUSIPC_tConstStr newOwner(0);
   
   assert( 0 != msg );
   
   if ( dbus_message_is_signal(msg, DBUS_INTERFACE_DBUS, "NameOwnerChanged") &&
      (dbus_message_has_path(msg, DBUS_PATH_DBUS)) )
   {
      // Parse out the arguments for the signal
      if ( dbus_message_get_args(msg, 0, DBUS_TYPE_STRING, &newName,
         DBUS_TYPE_STRING, &oldOwner, DBUS_TYPE_STRING, &newOwner,
         DBUS_TYPE_INVALID) )
      {
         // If we're not trying to match for a particular bus name
         if ( mBusName.empty() )
         {
            match = true;
         }
         else if ( (0 != newName) &&
            (0 == std::strcmp(mBusName.c_str(), newName)) )
         {
            match = true;
         }
      }
   }
   
   if ( match && ( 0 != mOnNameChange) )
   {
      uint64_t now = NSysDep::DBUSIPC_getSystemTime();
      mOnNameChange(newName ? newName : "",
                    oldOwner ? oldOwner : "",
                    newOwner ? newOwner : "",
                    mUserToken);
      uint64_t elapsed = NSysDep::DBUSIPC_getSystemTime() - now;
      if (  elapsed > timeout )
      {
         NSysDep::DBUSIPC_slog(NSysDep::SLOG_SEV_WARNING,
               "Failed to process D-Bus OnNameChange within %" PRIu64 " msec [PID=%u]",
               timeout, NSysDep::DBUSIPC_getProcId());
      }
   }
   return match;
}

   

DBUSIPCSubscription::DBUSIPCSubscription
   (
   Connection*             conn,
   const std::string&      objPath,
   const std::string&      sigName,
   DBUSIPC_tSignalCallback  onSignal,
   DBUSIPC_tUserToken       token
   )
   : SignalSubscription(conn)
   , mRule()
   , mObjectPath(objPath)
   , mSignalName(sigName)
   , mOnSignal(onSignal)
   , mUserToken(token)
{
   std::stringstream buffer;
   buffer << "type='signal',interface='" << DBUSIPC_INTERFACE_NAME <<
            "',member='" << DBUSIPC_INTERFACE_SIGNAL_NAME <<
            "',path='" << mObjectPath << "',arg0='" << mSignalName << "'";
   mRule = buffer.str();
}

   
DBUSIPCSubscription::~DBUSIPCSubscription()
{
}


const char* DBUSIPCSubscription::getRule() const
{
   return mRule.c_str();
}


bool DBUSIPCSubscription::dispatchIfMatch
   (
   DBusMessage*   msg,
   uint64_t       timeout
   )
{
   bool match(false);
   DBUSIPC_tConstStr name(0);
   DBUSIPC_tConstStr data(0);
   
   assert( 0 != msg );
   
   if ( dbus_message_is_signal(msg, DBUSIPC_INTERFACE_NAME,
      DBUSIPC_INTERFACE_SIGNAL_NAME) &&
      dbus_message_has_path(msg, mObjectPath.c_str()) )
   {
      // Parse out the arguments for the signal
      if ( dbus_message_get_args(msg, 0, DBUS_TYPE_STRING, &name,
         DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID) )
      {
         if ( (0 != name) && (0 == std::strcmp(name, mSignalName.c_str())) )
         {
            match = true;
            if ( 0!= mOnSignal )
            {
               uint64_t now = NSysDep::DBUSIPC_getSystemTime();
               mOnSignal(mSignalName.c_str(),
                         data ? data : "",
                         mUserToken);
               uint64_t elapsed = NSysDep::DBUSIPC_getSystemTime() - now;
               if (  elapsed > timeout )
               {
                  NSysDep::DBUSIPC_slog(NSysDep::SLOG_SEV_WARNING,
                        "Failed to process D-Bus signal (%s) within %" PRIu64 " msec [PID=%u]",
                        mSignalName.c_str(), timeout, NSysDep::DBUSIPC_getProcId());
               }
            }
         }
      }
   }
   
   return match; 
}

