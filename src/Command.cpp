
#include "Command.hpp"

#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "trace.h"
#include "Connection.hpp"
#include "Exceptions.hpp"
#include "InterfaceDefs.hpp"
#include "Semaphore.hpp"
#include "SignalSubscription.hpp"
#include "ServiceRegistration.hpp"
#include "RequestContext.hpp"
#include "Dispatcher.hpp"
#include "NUtil.hpp"

// An empty object value returned if user passes in NULL for either
// method parameters, a signal payload, returned result, or
// an error message.
static DBUSIPC_tConstStr EMPTY_OBJECT = "{}";

//==================================
//
// BaseCommand Implementation
//
//==================================

BaseCommand::BaseCommand()
   : mHandle(DBUSIPC_INVALID_HANDLE)
{
}

BaseCommand::~BaseCommand()
{
}


//==================================
//
// OpenConnectionCmd Implementation
//
//==================================

OpenConnectionCmd::OpenConnectionCmd
   (
   DBUSIPC_tConstStr           address,
   DBUSIPC_tBool               openPrivate,
   DBUSIPC_tConnectionCallback onConnect,
   DBUSIPC_tUserToken          token
   )
   : BaseCommand()
   , mAddress(address ? address : "")
   , mPrivConn(openPrivate)
   , mOnConnect(onConnect)
   , mToken(token)
   , mSem(0)
   , mStatus(0)
   , mConn(0)
{  
}


OpenConnectionCmd::OpenConnectionCmd
   (
   DBUSIPC_tConstStr     address,
   DBUSIPC_tBool         openPrivate,
   Semaphore*           sem,
   DBUSIPC_tError*       status,
   DBUSIPC_tConnection*  conn
   )
   : BaseCommand()
   , mAddress(address ? address : "")
   , mPrivConn(openPrivate)
   , mOnConnect(0)
   , mToken(0)
   , mSem(sem)
   , mStatus(status)
   , mConn(conn)
{  
}


OpenConnectionCmd::~OpenConnectionCmd()
{   
	if (NULL != mToken)
	{
		mToken = NULL;
	}
}


void OpenConnectionCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                    DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_CANCELLED),
                                    DBUSIPC_ERR_NAME_CANCELLED, 0};
   dispatch(status, 0);
}


void OpenConnectionCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   Connection* conn(0);
   try
   {
      conn = Connection::create(mAddress.c_str(), mPrivConn, &dispatcher);
      assert( 0 != conn );
      DBUSIPC_tCallbackStatus status = {DBUSIPC_ERROR_NONE,
                                       DBUSIPC_ERR_NAME_OK, 0};
      dispatch(status, conn);
   }
   catch ( const DBUSIPCError& e )
   {
      DBUSIPC_tCallbackStatus status = {e.getError(), 0, e.what()};
      dispatch(status, 0);
   }
}


void OpenConnectionCmd::dispatch
   (
   const DBUSIPC_tCallbackStatus& status,
   DBUSIPC_tConnection            conn
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = status.errCode;
      }
      
      if ( 0 != mConn )
      {
         *mConn = conn;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnConnect )
   {
      mOnConnect(&status, conn, mToken);
   }
}


//==================================
//
// GetConnectionCmd Implementation
//
//==================================

GetConnectionCmd::GetConnectionCmd
   (
   DBUSIPC_tConnType           connType,
   DBUSIPC_tBool               openPrivate,
   DBUSIPC_tConnectionCallback onConnect,
   DBUSIPC_tUserToken          token
   )
   : BaseCommand()
   , mConnType(connType)
   , mPrivConn(openPrivate)
   , mOnConnect(onConnect)
   , mToken(token)
   , mSem(0)
   , mStatus(0)
   , mConn(0)
{  
}


GetConnectionCmd::GetConnectionCmd
   (
   DBUSIPC_tConnType     connType,
   DBUSIPC_tBool         openPrivate,
   Semaphore*           sem,
   DBUSIPC_tError*       status,
   DBUSIPC_tConnection*  conn
   )
   : BaseCommand()
   , mConnType(connType)
   , mPrivConn(openPrivate)
   , mOnConnect(0)
   , mToken(0)
   , mSem(sem)
   , mStatus(status)
   , mConn(conn)
{  
}


GetConnectionCmd::~GetConnectionCmd()
{   
}


void GetConnectionCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                    DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_CANCELLED),
                                    DBUSIPC_ERR_NAME_CANCELLED, 0};
   dispatch(status, 0);
}


void GetConnectionCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   Connection* conn(0);
   try
   {
      conn = Connection::create(mConnType, mPrivConn, &dispatcher);
      assert( 0 != conn );
      DBUSIPC_tCallbackStatus status = {DBUSIPC_ERROR_NONE,
                                       DBUSIPC_ERR_NAME_OK, 0};
      dispatch(status, conn);
   }
   catch ( const DBUSIPCError& e )
   {
      DBUSIPC_tCallbackStatus status = {e.getError(), 0, e.what()};
      dispatch(status, 0);
   }
}


void GetConnectionCmd::dispatch
   (
   const DBUSIPC_tCallbackStatus& status,
   DBUSIPC_tConnection            conn
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = status.errCode;
      }
      
      if ( 0 != mConn )
      {
         *mConn = conn;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnConnect )
   {
      mOnConnect(&status, conn, mToken);
   }
}


//===================================
//
// CloseConnectionCmd Implementation
//
//===================================

CloseConnectionCmd::CloseConnectionCmd
   (
   DBUSIPC_tConnection   conn,
   Semaphore*           sem,
   DBUSIPC_tError*       status
   )
   : BaseCommand()
   , mConn(conn)
   , mSem(sem)
   , mStatus(status)
{   
}


CloseConnectionCmd::~CloseConnectionCmd()
{
}


void CloseConnectionCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   dispatch(DBUSIPC_MAKE_ERROR(
            DBUSIPC_ERROR_LEVEL_ERROR,
            DBUSIPC_DOMAIN_IPC_LIB,
            DBUSIPC_ERR_CANCELLED));
}


void CloseConnectionCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   try
   {
      Connection::release(static_cast<Connection*>(mConn));
      dispatch(DBUSIPC_ERROR_NONE);
   }
   catch ( const DBUSIPCError& e )
   {
      dispatch(e.getError());
      TRACE_ERROR("CloseConnectionCmd: failed to release connection=%p", mConn);
   }
}


void CloseConnectionCmd::dispatch
   (
   DBUSIPC_tError  error
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = error;
      }
      
      mSem->post();
   }
}


//===================================
//
// SubscribeCmd Implementation
//
//===================================

SubscribeCmd::SubscribeCmd
   (
   DBUSIPC_tConnection            conn,
   DBUSIPC_tConstStr              objPath,
   DBUSIPC_tConstStr              sigName,
   DBUSIPC_tSignalCallback        onSignal,
   DBUSIPC_tSubscriptionCallback  onSubscription,
   DBUSIPC_tUserToken             token
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mObjectPath()
   , mSignalName(sigName ? sigName : "")
   , mOnSignal(onSignal)
   , mOnSubscription(onSubscription)
   , mUserToken(token)
   , mSem(0)
   , mStatus(0)
   , mSubHnd(0)
   , mPendingCall(0)
   , mExecAndDestroy(true)
   , mSigSub()
{
   if ( 0 == objPath )
   {
      mObjectPath = std::string("/");
   }
   else
   {
      mObjectPath = std::string(objPath);
   }
   
   mSigSub.reset(new DBUSIPCSubscription(mConn, mObjectPath, mSignalName,
                                             mOnSignal, mUserToken));
}


SubscribeCmd::SubscribeCmd
   (
   DBUSIPC_tConnection      conn,
   DBUSIPC_tConstStr        objPath,
   DBUSIPC_tConstStr        sigName,
   DBUSIPC_tSignalCallback  onSignal,
   DBUSIPC_tUserToken       token,
   Semaphore*              sem,
   DBUSIPC_tError*          status,
   DBUSIPC_tSigSubHnd*      subHnd
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mObjectPath()
   , mSignalName(sigName ? sigName : "")
   , mOnSignal(onSignal)
   , mOnSubscription(0)
   , mUserToken(token)
   , mSem(sem)
   , mStatus(status)
   , mSubHnd(subHnd)
   , mPendingCall(0)
   , mExecAndDestroy(true)
   , mSigSub()
{
   if ( 0 == objPath )
   {
      mObjectPath = std::string("/");
   }
   else
   {
      mObjectPath = std::string(objPath);
   }

   mSigSub.reset(new DBUSIPCSubscription(mConn, mObjectPath, mSignalName,
                                             mOnSignal, mUserToken));
}


SubscribeCmd::~SubscribeCmd()
{
   if ( 0 != mPendingCall )
   {
      dbus_pending_call_unref(mPendingCall);
   }
}


void SubscribeCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 != mPendingCall )
   {
      // If the call already completed by the time cancel was called then ...
      if ( dbus_pending_call_get_completed(mPendingCall) )
      {
         DBusMessage* reply = dbus_pending_call_steal_reply(mPendingCall);
         if ( 0 != reply )
         {
            // Free the up reply message
            dbus_message_unref(reply);
         }
      }
      else
      {
         dbus_pending_call_cancel(mPendingCall);
      }
   }
   
   DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                    DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_CANCELLED),
                                    DBUSIPC_ERR_NAME_CANCELLED, 0};
   dispatch(status, DBUSIPC_INVALID_HANDLE);
}


bool SubscribeCmd::execAndDestroy() const
{
   return mExecAndDestroy;
}

void SubscribeCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   DBusConnection* dbusConn = Connection::getDBusConnection(mConn);
   if ( !dbus_connection_get_is_connected(dbusConn) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_NOT_CONNECTED),
                                       DBUSIPC_ERR_NAME_NOT_CONNECTED,
                                       "Not connected"};
      dispatch(status, DBUSIPC_INVALID_HANDLE);
   }
   else
   {
      DBusMessage* reqMsg = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
                        DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                        "AddMatch");
      if ( 0 == reqMsg )
      {
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NO_MEMORY),
                                          DBUSIPC_ERR_NAME_NO_MEMORY,
                                          "Out of memory"};
         dispatch(status, DBUSIPC_INVALID_HANDLE);
      }
      else
      {
         dbus_message_set_no_reply(reqMsg, false);
         
         // Pack in the arguments
         DBUSIPC_tConstStr rule = mSigSub->getRule();
         if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
            &rule, DBUS_TYPE_INVALID) )
         {
            DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
            dispatch(status, DBUSIPC_INVALID_HANDLE);
         }
         else
         {
            if ( !dbus_connection_send_with_reply(dbusConn, reqMsg,
               &mPendingCall, -1) )
            {
               DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                DBUSIPC_ERROR_LEVEL_ERROR,
                                                DBUSIPC_DOMAIN_IPC_LIB,
                                                DBUSIPC_ERR_NO_MEMORY),
                                                DBUSIPC_ERR_NAME_NO_MEMORY,
                                                "Out of memory"};
               dispatch(status, DBUSIPC_INVALID_HANDLE);
            }
            else
            {
               if ( !dbus_pending_call_set_notify(mPendingCall,
                  SubscribeCmd::onPendingCallNotify, this, 0) )
               {
                  DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                   DBUSIPC_ERROR_LEVEL_ERROR,
                                                   DBUSIPC_DOMAIN_IPC_LIB,
                                                   DBUSIPC_ERR_NO_MEMORY),
                                                   DBUSIPC_ERR_NAME_NO_MEMORY,
                                                   "Out of memory"};
                  dispatch(status, DBUSIPC_INVALID_HANDLE);   
               }
               else
               {
                  try
                  {
                     // Register this pending command so we can
                     // remove it later
                     mConn->registerPending(this);
                     
                     // We don't want the command dispatcher to
                     // destroy this command after a request has been
                     // made and we're waiting on the result.
                     mExecAndDestroy = false;
                  }
                  catch ( ... )
                  {
                     // Failed to register the command in the pending
                     // command list so we'll cancel the request.
                     dbus_pending_call_cancel(mPendingCall);
                     
                     DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                      DBUSIPC_ERROR_LEVEL_ERROR,
                                                      DBUSIPC_DOMAIN_IPC_LIB,
                                                      DBUSIPC_ERR_NO_MEMORY),
                                                      DBUSIPC_ERR_NAME_NO_MEMORY,
                                                      "Out of memory"};
                     dispatch(status, DBUSIPC_INVALID_HANDLE);
                  }
               }
            }
         }
         // Free the request message (finally)
         dbus_message_unref(reqMsg);
      }
   }
}


void SubscribeCmd::dispatch
   (
   const DBUSIPC_tCallbackStatus& status,
   DBUSIPC_tSigSubHnd             subHnd
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = status.errCode;
      }
      
      if ( 0 != mSubHnd )
      {
         *mSubHnd = subHnd;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnSubscription )
   {
      mOnSubscription(&status, subHnd, mUserToken);
   }   
}


void SubscribeCmd::onPendingCallNotify
   (
   DBusPendingCall*  call,
   void*             userData
   )
{
   SubscribeCmd* cmd = static_cast<SubscribeCmd*>(userData);
   assert( 0 != cmd );

   DBusConnection* dbusConn = Connection::getDBusConnection(cmd->mConn);
   DBusMessage* reply = dbus_pending_call_steal_reply(call);
   if ( (0 == reply) || !dbus_connection_get_is_connected(dbusConn) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_INTERNAL),
                                       DBUSIPC_ERR_NAME_INTERNAL,
                                       "Failed to retrieve reply message"};
      cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
   }
   else
   {
      int32_t replyType = dbus_message_get_type(reply);
      
      // If this is an error message then ...
      if ( DBUS_MESSAGE_TYPE_ERROR == replyType )
      {
         DBUSIPC_tConstStr errName = dbus_message_get_error_name(reply);
         DBUSIPC_tConstStr errMsg(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &errMsg,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract error "
                       "message");
         }
         
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_DBUS),
                                          errName, errMsg};
         cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
      }
      else if ( DBUS_MESSAGE_TYPE_METHOD_RETURN == replyType )
      {
         try
         {
            cmd->mConn->subscribeSignal(cmd->mSigSub.get());
            DBUSIPC_tCallbackStatus status = {DBUSIPC_ERROR_NONE,
                                             DBUSIPC_ERR_NAME_OK, 0};
            cmd->dispatch(status, cmd->mSigSub.get());
            cmd->mSigSub.release();
         }
         catch ( ... )
         {
            // Make our best effort to remove the match we just set
            DBusMessage* reqMsg = dbus_message_new_method_call(
                              DBUS_SERVICE_DBUS,
                              DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                              "RemoveMatch");
            if ( 0 == reqMsg )
            {
               TRACE_WARN("onPendingCallNotify: "
                          "Failed to remove match on error");
            }
            else
            {
               // We don't care about the reply - we're making a best
               // effort attempt to remove the match
               dbus_message_set_no_reply(reqMsg, true);
               
               // Pack in the arguments
               DBUSIPC_tConstStr rule = cmd->mSigSub->getRule();
               if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
                  &rule, DBUS_TYPE_INVALID) )
               {
                  TRACE_WARN("onPendingCallNotify: "
                             "Failed to remove match on error");
               }
               else
               {
                  dbus_uint32_t serNum;
                  if ( !dbus_connection_send(dbusConn, reqMsg,
                     &serNum) )
                  {
                     TRACE_WARN("onPendingCallNotify: "
                                "Failed to remove match on error");
                  }
               }
               
               dbus_message_unref(reqMsg);
            }
            
            DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
            cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
         }
      }
      else
      {
         TRACE_WARN("onPendingCallNotify: Received unexpected D-Bus "
                    "message type (%d)", replyType);
      }

      // Unregister the pending command
      cmd->mConn->unregisterPending(cmd);
      
      // Free the up reply message
      dbus_message_unref(reply);
   }
   
   // This command is done - destroy ourselves
   delete cmd;
}


//===================================
//
// UnsubscribeCmd Implementation
//
//===================================

UnsubscribeCmd::UnsubscribeCmd
   (
   SignalSubscription*     sub,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
   : BaseCommand()
   , mSigSub(sub)
   , mOnStatus(onStatus)
   , mUserToken(token)
   , mSem(0)
   , mStatus(0)
   , mPendingCall(0)
   , mExecAndDestroy(true)
{  
}


UnsubscribeCmd::UnsubscribeCmd
   (
   SignalSubscription*     sub,
   Semaphore*              sem,
   DBUSIPC_tError*          status
   )
   : BaseCommand()
   , mSigSub(sub)
   , mOnStatus(0)
   , mUserToken(0)
   , mSem(sem)
   , mStatus(status)
   , mPendingCall(0)
   , mExecAndDestroy(true)
{  
}
      
   
UnsubscribeCmd::~UnsubscribeCmd()
{
   if ( 0 != mPendingCall )
   {
      dbus_pending_call_unref(mPendingCall);
   }
}


void UnsubscribeCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 != mPendingCall )
   {
      // If the call already completed by the time cancel was called then ...
      if ( dbus_pending_call_get_completed(mPendingCall) )
      {
         DBusMessage* reply = dbus_pending_call_steal_reply(mPendingCall);
         if ( 0 != reply )
         {
            // Free the up reply message
            dbus_message_unref(reply);
         }
      }
      else
      {
         dbus_pending_call_cancel(mPendingCall);
      }
   }
   
   DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                    DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_CANCELLED),
                                    DBUSIPC_ERR_NAME_CANCELLED, 0};
   dispatch(status);
}


bool UnsubscribeCmd::execAndDestroy() const
{
   return mExecAndDestroy;
}


void UnsubscribeCmd::dispatch
   (
   const DBUSIPC_tCallbackStatus& status
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = status.errCode;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnStatus )
   {
      mOnStatus(&status, mUserToken);
   }
}


void UnsubscribeCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   // See if we still have a record of this subscription
   if ( !Connection::signalSubExists(mSigSub) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NOT_FOUND),
                                             DBUSIPC_ERR_NAME_NOT_FOUND,
                                             "Subscription does not exist"};
      dispatch(status);
   }
   else
   {
      Connection* conn = mSigSub->getConnection();
      DBusConnection* dbusConn = Connection::getDBusConnection(conn);
      if ( !dbus_connection_get_is_connected(dbusConn) )
      {
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NOT_CONNECTED),
                                          DBUSIPC_ERR_NAME_NOT_CONNECTED,
                                          "Not connected"};
         dispatch(status);
      }
      else
      {
         DBusMessage* reqMsg = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
                           DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                           "RemoveMatch");
         if ( 0 == reqMsg )
         {
            DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
            dispatch(status);
         }
         else
         {
            dbus_message_set_no_reply(reqMsg, false);
            
            // Pack in the arguments
            DBUSIPC_tConstStr rule = mSigSub->getRule();
            if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
               &rule, DBUS_TYPE_INVALID) )
            {
               DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                DBUSIPC_ERROR_LEVEL_ERROR,
                                                DBUSIPC_DOMAIN_IPC_LIB,
                                                DBUSIPC_ERR_NO_MEMORY),
                                                DBUSIPC_ERR_NAME_NO_MEMORY,
                                                "Out of memory"};
               dispatch(status);
            }
            else
            {
               if ( !dbus_connection_send_with_reply(dbusConn, reqMsg,
                  &mPendingCall, -1) )
               {
                  DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                   DBUSIPC_ERROR_LEVEL_ERROR,
                                                   DBUSIPC_DOMAIN_IPC_LIB,
                                                   DBUSIPC_ERR_NO_MEMORY),
                                                   DBUSIPC_ERR_NAME_NO_MEMORY,
                                                   "Out of memory"};
                  dispatch(status);
               }
               else
               {
                  if ( !dbus_pending_call_set_notify(mPendingCall,
                     UnsubscribeCmd::onPendingCallNotify, this, 0) )
                  {
                     DBUSIPC_tCallbackStatus status = {
                                          DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NO_MEMORY),
                                          DBUSIPC_ERR_NAME_NO_MEMORY,
                                          "Out of memory"};
                     dispatch(status);   
                  }
                  else
                  {
                     try
                     {
                        // Register this pending command so we can
                        // remove it later
                        conn->registerPending(this);
                        
                        // We don't want the command dispatcher to
                        // destroy this command after a request has been
                        // made and we're waiting on the result.
                        mExecAndDestroy = false;
                     }
                     catch ( ... )
                     {
                        // Failed to register the command in the pending
                        // command list so we'll cancel the request.
                        dbus_pending_call_cancel(mPendingCall);
                        
                        DBUSIPC_tCallbackStatus status = {
                                             DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
                        dispatch(status);
                     }
                  }
               }
            }
            // Free the request message (finally)
            dbus_message_unref(reqMsg);
         }
      }
   }
}


void UnsubscribeCmd::onPendingCallNotify
   (
   DBusPendingCall*  call,
   void*             userData
   )
{
   UnsubscribeCmd* cmd = static_cast<UnsubscribeCmd*>(userData);
   assert( 0 != cmd );

   Connection* conn = cmd->mSigSub->getConnection();
   assert( 0 != conn );
   
   DBusMessage* reply = dbus_pending_call_steal_reply(call);
   if ( 0 == reply )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_INTERNAL),
                                       DBUSIPC_ERR_NAME_INTERNAL,
                                       "Failed to retrieve reply message"};
      cmd->dispatch(status);
   }
   else
   {
      int32_t replyType = dbus_message_get_type(reply);
      
      // If this is an error message then ...
      if ( DBUS_MESSAGE_TYPE_ERROR == replyType )
      {
         DBUSIPC_tConstStr errName = dbus_message_get_error_name(reply);
         DBUSIPC_tConstStr errMsg(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &errMsg,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract error "
                       "message");
         }
         
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_DBUS),
                                          errName, errMsg};
         cmd->dispatch(status);
      }
      else if ( DBUS_MESSAGE_TYPE_METHOD_RETURN == replyType )
      {
         try
         {
            // This will delete the subscription
            conn->unsubscribeSignal(cmd->mSigSub);
            DBUSIPC_tCallbackStatus status = {DBUSIPC_ERROR_NONE,
                                             DBUSIPC_ERR_NAME_OK, 0};
            cmd->dispatch(status);
         }
         catch ( const DBUSIPCError& e )
         {  
            DBUSIPC_tCallbackStatus status = {e.getError(),
                                             DBUSIPC_ERR_NAME_BAD_ARGS,
                                             e.what()};
            cmd->dispatch(status);
         }
      }
      else
      {
         TRACE_WARN("onPendingCallNotify: Received unexpected D-Bus "
                    "message type (%d)", replyType);
      }

      // Unregister the pending command
      conn->unregisterPending(cmd);
      
      // Free the up reply message
      dbus_message_unref(reply);
      
      // This command is done - destroy ourselves
      delete cmd;
   }
}

//===================================
//
// RegisterServiceCmd Implementation
//
//===================================

RegisterServiceCmd::RegisterServiceCmd
   (
   DBUSIPC_tConnection            conn,
   DBUSIPC_tConstStr              busName,
   DBUSIPC_tConstStr              objPath,
   DBUSIPC_tUInt32                flag,
   DBUSIPC_tRequestCallback       onRequest,
   DBUSIPC_tRegistrationCallback  onRegister,
   DBUSIPC_tUserToken             token
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mBusName(busName ? busName : "")
   , mObjectPath()
   , mFlag(flag)
   , mOnRequest(onRequest)
   , mOnRegister(onRegister)
   , mUserToken(token)
   , mSem(0)
   , mStatus(0)
   , mRegHnd(0)
   , mPendingCall(0)
   , mExecAndDestroy(true)
   , mSvcReg()
{
   if ( 0 == objPath )
   {
      mObjectPath = NUtil::busNameToObjPath(busName);
   }
   else
   {
      mObjectPath = std::string(objPath);
   }
   
   mSvcReg.reset(new ServiceRegistration(mConn, mBusName, mObjectPath,
                                         mFlag, mOnRequest, mUserToken));
}


RegisterServiceCmd::RegisterServiceCmd
   (
   DBUSIPC_tConnection         conn,
   DBUSIPC_tConstStr           busName,
   DBUSIPC_tConstStr           objPath,
   DBUSIPC_tUInt32             flag,
   DBUSIPC_tRequestCallback    onRequest,
   DBUSIPC_tUserToken          token,
   Semaphore*                 sem,
   DBUSIPC_tError*             status,
   DBUSIPC_tSvcRegHnd*         regHnd
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mBusName(busName ? busName : "")
   , mObjectPath()
   , mFlag(flag)
   , mOnRequest(onRequest)
   , mOnRegister(0)
   , mUserToken(token)
   , mSem(sem)
   , mStatus(status)
   , mRegHnd(regHnd)
   , mPendingCall(0)
   , mExecAndDestroy(true)
   , mSvcReg()
{
   if ( 0 == objPath )
   {
      mObjectPath = NUtil::busNameToObjPath(busName);
   }
   else
   {
      mObjectPath = std::string(objPath);
   }
   
   mSvcReg.reset(new ServiceRegistration(mConn, mBusName, mObjectPath,
                                      mFlag, mOnRequest, mUserToken));
}

RegisterServiceCmd::~RegisterServiceCmd()
{
   if ( 0 != mPendingCall )
   {
      dbus_pending_call_unref(mPendingCall);
   }   
}


void RegisterServiceCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 != mPendingCall )
   {
      // If the call already completed by the time cancel was called then ...
      if ( dbus_pending_call_get_completed(mPendingCall) )
      {
         DBusMessage* reply = dbus_pending_call_steal_reply(mPendingCall);
         if ( 0 != reply )
         {
            // Free the up reply message
            dbus_message_unref(reply);
         }
      }
      else
      {
         dbus_pending_call_cancel(mPendingCall);
      }
   }
   
   DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                    DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_CANCELLED),
                                    DBUSIPC_ERR_NAME_CANCELLED, 0};
   dispatch(status, DBUSIPC_INVALID_HANDLE);
}


bool RegisterServiceCmd::execAndDestroy() const
{
   return mExecAndDestroy;
}


void RegisterServiceCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   DBusConnection* dbusConn = Connection::getDBusConnection(mConn);
   if ( !dbus_connection_get_is_connected(dbusConn) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_NOT_CONNECTED),
                                       DBUSIPC_ERR_NAME_NOT_CONNECTED,
                                       "Not connected"};
      dispatch(status, DBUSIPC_INVALID_HANDLE);
   }
   else
   {
      uint32_t dbusFlags(DBUS_NAME_FLAG_DO_NOT_QUEUE |
                         DBUS_NAME_FLAG_REPLACE_EXISTING);
      DBusMessage* reqMsg = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
                        DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                        "RequestName");
      if ( 0 == reqMsg )
      {
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NO_MEMORY),
                                          DBUSIPC_ERR_NAME_NO_MEMORY,
                                          "Out of memory"};
         dispatch(status, DBUSIPC_INVALID_HANDLE);
      }
      else
      {
         dbus_message_set_no_reply(reqMsg, false);
         
         // Pack in the arguments
         DBUSIPC_tConstStr busName = mSvcReg->getBusName();
         if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
            &busName, DBUS_TYPE_UINT32, &dbusFlags,
            DBUS_TYPE_INVALID) )
         {
            DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
            dispatch(status, DBUSIPC_INVALID_HANDLE);
         }
         else
         {
            if ( !dbus_connection_send_with_reply(dbusConn, reqMsg,
               &mPendingCall, -1) )
            {
               DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                DBUSIPC_ERROR_LEVEL_ERROR,
                                                DBUSIPC_DOMAIN_IPC_LIB,
                                                DBUSIPC_ERR_NO_MEMORY),
                                                DBUSIPC_ERR_NAME_NO_MEMORY,
                                                "Out of memory"};
               dispatch(status, DBUSIPC_INVALID_HANDLE);
            }
            else
            {
               if ( !dbus_pending_call_set_notify(mPendingCall,
                  RegisterServiceCmd::onPendingCallNotify, this, 0) )
               {
                  DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                   DBUSIPC_ERROR_LEVEL_ERROR,
                                                   DBUSIPC_DOMAIN_IPC_LIB,
                                                   DBUSIPC_ERR_NO_MEMORY),
                                                   DBUSIPC_ERR_NAME_NO_MEMORY,
                                                   "Out of memory"};
                  dispatch(status, DBUSIPC_INVALID_HANDLE);   
               }
               else
               {
                  try
                  {
                     // Register this pending command so we can
                     // remove it later
                     mConn->registerPending(this);
                     
                     // We don't want the command dispatcher to
                     // destroy this command after a request has been
                     // made and we're waiting on the result.
                     mExecAndDestroy = false;
                  }
                  catch ( ... )
                  {
                     // Failed to register the command in the pending
                     // command list so we'll cancel the request.
                     dbus_pending_call_cancel(mPendingCall);
                     
                     DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                      DBUSIPC_ERROR_LEVEL_ERROR,
                                                      DBUSIPC_DOMAIN_IPC_LIB,
                                                      DBUSIPC_ERR_NO_MEMORY),
                                                      DBUSIPC_ERR_NAME_NO_MEMORY,
                                                      "Out of memory"};
                     dispatch(status, DBUSIPC_INVALID_HANDLE);
                  }
               }
            }
         }
         // Free the request message (finally)
         dbus_message_unref(reqMsg);
      }
   }
}


void RegisterServiceCmd::dispatch
   (
   const DBUSIPC_tCallbackStatus& status,
   DBUSIPC_tSvcRegHnd             regHnd
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = status.errCode;
      }
      
      if ( 0 != mRegHnd )
      {
         *mRegHnd = regHnd;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnRegister )
   {
      mOnRegister(&status, regHnd, mUserToken);
   }   
}


void RegisterServiceCmd::onPendingCallNotify
   (
   DBusPendingCall*  call,
   void*             userData
   )
{
   RegisterServiceCmd* cmd = static_cast<RegisterServiceCmd*>(userData);
   assert( 0 != cmd );

   DBusConnection* dbusConn = Connection::getDBusConnection(cmd->mConn);
   DBusMessage* reply = dbus_pending_call_steal_reply(call);
   if ( (0 == reply) || !dbus_connection_get_is_connected(dbusConn) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_INTERNAL),
                                       DBUSIPC_ERR_NAME_INTERNAL,
                                       "Failed to retrieve reply message"};
      cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
   }
   else
   {
      int32_t replyType = dbus_message_get_type(reply);
      
      // If this is an error message then ...
      if ( DBUS_MESSAGE_TYPE_ERROR == replyType )
      {
         DBUSIPC_tConstStr errName = dbus_message_get_error_name(reply);
         DBUSIPC_tConstStr errMsg(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &errMsg,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract error "
                       "message");
         }
         
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_DBUS),
                                          errName, errMsg};
         cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
      }
      else if ( DBUS_MESSAGE_TYPE_METHOD_RETURN == replyType )
      {
         try
         {
            dbus_int32_t result;
            if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_UINT32, &result,
               DBUS_TYPE_INVALID) )
            {
               throw DBUSIPCError(DBUSIPC_MAKE_ERROR(
                                 DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_DBUS),
                                 "Failed to extract result from reply");
            }
            
            // If we acquired the bus name (or already own it) then ...
            if ( (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER == result) ||
               (DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER == result) )
            {
               cmd->mConn->registerService(cmd->mSvcReg.get());
               DBUSIPC_tCallbackStatus status = {DBUSIPC_ERROR_NONE,
                                                DBUSIPC_ERR_NAME_OK, 0};
               cmd->dispatch(status, cmd->mSvcReg.get());
               cmd->mSvcReg.release();
            }
            else
            {
               DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                DBUSIPC_ERROR_LEVEL_ERROR,
                                                DBUSIPC_DOMAIN_IPC_LIB,
                                                DBUSIPC_ERR_DBUS),
                                                DBUSIPC_ERR_NAME_DBUS, 
                                                "Cannot acquire bus name"};
               cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);               
            }
         }
         catch ( ... )
         {
            // Make our best effort to release the bus name we acquired
            DBusMessage* reqMsg = dbus_message_new_method_call(
                              DBUS_SERVICE_DBUS,
                              DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                              "ReleaseName");
            if ( 0 == reqMsg )
            {
               TRACE_WARN("onPendingCallNotify: "
                          "Failed to release bus name on error");
            }
            else
            {
               // We don't care about the reply - we're making a best
               // effort attempt to remove the match
               dbus_message_set_no_reply(reqMsg, true);
               
               // Pack in the arguments
               DBUSIPC_tConstStr busName = cmd->mSvcReg->getBusName();
               if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
                  &busName, DBUS_TYPE_INVALID) )
               {
                  TRACE_WARN("onPendingCallNotify: "
                             "Failed to release bus name on error");
               }
               else
               {
                  dbus_uint32_t serNum;
                  if ( !dbus_connection_send(dbusConn, reqMsg,
                     &serNum) )
                  {
                     TRACE_WARN("onPendingCallNotify: "
                                "Failed to release bus name on error");
                  }
               }
               
               dbus_message_unref(reqMsg);
            }
            
            DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
            cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
         }
      }
      else
      {
         TRACE_WARN("onPendingCallNotify: Received unexpected D-Bus "
                    "message type (%d)", replyType);
      }

      // Unregister the pending command
      cmd->mConn->unregisterPending(cmd);
      
      // Free the up reply message
      dbus_message_unref(reply);
      
      // This command is done - destroy ourselves
      delete cmd;
   }
}

//=====================================
//
// UnregisterServiceCmd Implementation
//
//=====================================

UnregisterServiceCmd::UnregisterServiceCmd
   (
   ServiceRegistration*    reg,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
   : BaseCommand()
   , mSvcReg(reg)
   , mOnStatus(onStatus)
   , mUserToken(token)
   , mSem(0)
   , mStatus(0)
   , mPendingCall(0)
   , mExecAndDestroy(true)
{   
}


UnregisterServiceCmd::UnregisterServiceCmd
   (
   ServiceRegistration*    reg,
   Semaphore*              sem,
   DBUSIPC_tError*          status
   )
   : BaseCommand()
   , mSvcReg(reg)
   , mOnStatus(0)
   , mUserToken(0)
   , mSem(sem)
   , mStatus(status)
   , mPendingCall(0)
   , mExecAndDestroy(true)
{   
}
      
      
UnregisterServiceCmd::~UnregisterServiceCmd()
{
   if ( 0 != mPendingCall )
   {
      dbus_pending_call_unref(mPendingCall);
   }
}


void UnregisterServiceCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 != mPendingCall )
   {
      // If the call already completed by the time cancel was called then ...
      if ( dbus_pending_call_get_completed(mPendingCall) )
      {
         DBusMessage* reply = dbus_pending_call_steal_reply(mPendingCall);
         if ( 0 != reply )
         {
            // Free the up reply message
            dbus_message_unref(reply);
         }
      }
      else
      {
         dbus_pending_call_cancel(mPendingCall);
      }
   }
   
   DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                    DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_CANCELLED),
                                    DBUSIPC_ERR_NAME_CANCELLED, 0};
   dispatch(status);
}


bool UnregisterServiceCmd::execAndDestroy() const
{
   return mExecAndDestroy;
}


void UnregisterServiceCmd::dispatch
   (
   const DBUSIPC_tCallbackStatus& status
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = status.errCode;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnStatus )
   {
      mOnStatus(&status, mUserToken);
   }
}


void UnregisterServiceCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   // See if we still have a record of this service reservation
   if ( !Connection::serviceRegExists(mSvcReg) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_NOT_FOUND),
                                       DBUSIPC_ERR_NAME_NOT_FOUND,
                                       "Service registration does not exist"};
      dispatch(status);
   }
   else
   {
      Connection* conn = mSvcReg->getConnection();
      DBusConnection* dbusConn = Connection::getDBusConnection(conn);
      if ( !dbus_connection_get_is_connected(dbusConn) )
      {
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NOT_CONNECTED),
                                          DBUSIPC_ERR_NAME_NOT_CONNECTED,
                                          "Not connected"};
         dispatch(status);
      }
      else
      {
         DBusMessage* reqMsg = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
                           DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                           "ReleaseName");
         if ( 0 == reqMsg )
         {
            DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
            dispatch(status);
         }
         else
         {
            dbus_message_set_no_reply(reqMsg, false);
            
            // Pack in the arguments
            DBUSIPC_tConstStr busName = mSvcReg->getBusName();
            if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
               &busName, DBUS_TYPE_INVALID) )
            {
               DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                DBUSIPC_ERROR_LEVEL_ERROR,
                                                DBUSIPC_DOMAIN_IPC_LIB,
                                                DBUSIPC_ERR_NO_MEMORY),
                                                DBUSIPC_ERR_NAME_NO_MEMORY,
                                                "Out of memory"};
               dispatch(status);
            }
            else
            {
               if ( !dbus_connection_send_with_reply(dbusConn, reqMsg,
                  &mPendingCall, -1) )
               {
                  DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                   DBUSIPC_ERROR_LEVEL_ERROR,
                                                   DBUSIPC_DOMAIN_IPC_LIB,
                                                   DBUSIPC_ERR_NO_MEMORY),
                                                   DBUSIPC_ERR_NAME_NO_MEMORY,
                                                   "Out of memory"};
                  dispatch(status);
               }
               else
               {
                  if ( !dbus_pending_call_set_notify(mPendingCall,
                     UnregisterServiceCmd::onPendingCallNotify, this, 0) )
                  {
                     DBUSIPC_tCallbackStatus status = {
                                          DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NO_MEMORY),
                                          DBUSIPC_ERR_NAME_NO_MEMORY,
                                          "Out of memory"};
                     dispatch(status);   
                  }
                  else
                  {
                     try
                     {
                        // Register this pending command so we can
                        // remove it later
                        conn->registerPending(this);
                        
                        // We don't want the command dispatcher to
                        // destroy this command after a request has been
                        // made and we're waiting on the result.
                        mExecAndDestroy = false;
                     }
                     catch ( ... )
                     {
                        // Failed to register the command in the pending
                        // command list so we'll cancel the request.
                        dbus_pending_call_cancel(mPendingCall);
                        
                        DBUSIPC_tCallbackStatus status = {
                                             DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
                        dispatch(status);
                     }
                  }
               }
            }
            // Free the request message (finally)
            dbus_message_unref(reqMsg);
         }
      }
   }
}


void UnregisterServiceCmd::onPendingCallNotify
   (
   DBusPendingCall*  call,
   void*             userData
   )
{
   UnregisterServiceCmd* cmd = static_cast<UnregisterServiceCmd*>(userData);
   assert( 0 != cmd );

   Connection* conn = cmd->mSvcReg->getConnection();
   
   DBusMessage* reply = dbus_pending_call_steal_reply(call);
   if ( 0 == reply || (0 == conn) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_INTERNAL),
                                       DBUSIPC_ERR_NAME_INTERNAL,
                                       "Failed to retrieve reply message"};
      
      cmd->dispatch(status);
   }
   else
   {
      int32_t replyType = dbus_message_get_type(reply);
      
      // If this is an error message then ...
      if ( DBUS_MESSAGE_TYPE_ERROR == replyType )
      {
         DBUSIPC_tConstStr errName = dbus_message_get_error_name(reply);
         DBUSIPC_tConstStr errMsg(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &errMsg,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract error "
                       "message");
         }
         
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_DBUS),
                                          errName,
                                          errMsg};
         cmd->dispatch(status);
      }
      else if ( DBUS_MESSAGE_TYPE_METHOD_RETURN == replyType )
      {
         dbus_uint32_t result(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_UINT32, &result,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract results");
         }
         
         // If it wasn't released, what can we really do about it?
         if ( DBUS_RELEASE_NAME_REPLY_RELEASED != result )
         {
            // Let's just log a warning ...
            TRACE_WARN("OnPendingCallNotify: Failed to release bus name");
         }
         
         // Now we'll try to unregister the service internally
         try
         {
            DBUSIPC_tCallbackStatus status = {DBUSIPC_ERROR_NONE,
                                             DBUSIPC_ERR_NAME_OK,
                                             0};
            // This will delete the registration
            conn->unregisterService(cmd->mSvcReg);
            cmd->dispatch(status);
         }
         catch ( const DBUSIPCError& e )
         {
            TRACE_WARN("onPendingCallNotify: Failed to unregister service");
            DBUSIPC_tCallbackStatus status = { e.getError(),
                                             DBUSIPC_ERR_NAME_NOT_FOUND,
                                             0 };
            cmd->dispatch(status);
         }
      }
      else
      {
         TRACE_WARN("onPendingCallNotify: Received unexpected D-Bus "
                    "message type (%d)", replyType);
      }

      // Unregister the pending command
      conn->unregisterPending(cmd);
      
      // Free the up reply message
      dbus_message_unref(reply);
      
      // This command is done - destroy ourselves
      delete cmd;
   }
}

//=====================================
//
// InvokeCmd Implementation
//
//=====================================

// Constructor for asynchronous calls
InvokeCmd::InvokeCmd
   (
   DBUSIPC_tConnection      conn,
   DBUSIPC_tConstStr        busName,
   DBUSIPC_tConstStr        objPath,
   DBUSIPC_tConstStr        method,
   DBUSIPC_tConstStr        parameters,
   bool                    noReplyExpected,
   DBUSIPC_tUInt32          msecTimeout,
   DBUSIPC_tResultCallback  onResult,
   DBUSIPC_tUserToken       token
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mBusName(busName)
   , mObjectPath()
   , mMethod(method)
   , mParms(parameters ? parameters : EMPTY_OBJECT)
   , mNoReplyExpected(noReplyExpected)
   , mMsecTimeout(msecTimeout)
   , mOnResult(onResult)
   , mUserToken(token)
   , mResponse(0)
   , mSem(0)
   , mPendingCall(0)
   , mExecAndDestroy(true)
   , mSerialNum(0U)
{
   if ( 0 == objPath )
   {
      mObjectPath = NUtil::busNameToObjPath(busName);
   }
   else
   {
      mObjectPath = std::string(objPath);
   }
}


// Constructor for synchronous calls
InvokeCmd::InvokeCmd
   (
   DBUSIPC_tConnection      conn,
   DBUSIPC_tConstStr        busName,
   DBUSIPC_tConstStr        objPath,
   DBUSIPC_tConstStr        method,
   DBUSIPC_tConstStr        parameters,
   DBUSIPC_tUInt32          msecTimeout,
   DBUSIPC_tResponse**      response,
   Semaphore*              sem
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mBusName(busName)
   , mObjectPath()
   , mMethod(method)
   , mParms(parameters ? parameters : EMPTY_OBJECT)
   , mNoReplyExpected(false)
   , mMsecTimeout(msecTimeout)
   , mOnResult(0)
   , mUserToken(0)
   , mResponse(response)
   , mSem(sem)
   , mPendingCall(0)
   , mExecAndDestroy(true)
   , mSerialNum(0U)
{
   if ( 0 == objPath )
   {
      mObjectPath = NUtil::busNameToObjPath(busName);
   }
   else
   {
      mObjectPath = std::string(objPath);
   }
   
   if ( 0 == mResponse )
   {
      throw DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                                   DBUSIPC_DOMAIN_IPC_LIB,
                                                   DBUSIPC_ERR_BAD_ARGS), 
                                                   "Bad response pointer");
   }
   else
   {
      *mResponse = static_cast<DBUSIPC_tResponse*>
                              (std::calloc(1, sizeof(DBUSIPC_tResponse)));
      if ( 0 == *mResponse )
      {
         throw DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY), 
                                             "Cannot allocate response");
      }
      
      (*mResponse)->status.errCode = DBUSIPC_ERROR_NONE;
   }
}


InvokeCmd::~InvokeCmd()
{
   if ( 0 != mPendingCall )
   {
      dbus_pending_call_unref(mPendingCall);
   }
}


void InvokeCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   DBusConnection* dbusConn = Connection::getDBusConnection(mConn);
   if ( !dbus_connection_get_is_connected(dbusConn) )
   {
      dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                     DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NOT_CONNECTED),
                     DBUSIPC_ERR_NAME_NOT_CONNECTED, "Not connected", 0);
   }
   else
   {
      DBusMessage* reqMsg = dbus_message_new_method_call(mBusName.c_str(),
                        mObjectPath.c_str(), DBUSIPC_INTERFACE_NAME,
                        DBUSIPC_INTERFACE_METHOD_NAME);
      if ( 0 == reqMsg )
      {
         dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                        DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                        DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
      }
      else
      {
         dbus_message_set_no_reply(reqMsg, mNoReplyExpected);
         
         // Pack in the arguments
         DBUSIPC_tConstStr dbusMethod = mMethod.c_str();
         DBUSIPC_tConstStr dbusParms = mParms.c_str();
         if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
            &dbusMethod, DBUS_TYPE_STRING, &dbusParms,
            DBUS_TYPE_INVALID) )
         {
            dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                           DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                           DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
         }
         else
         {
            if ( mNoReplyExpected )
            {
               if ( !dbus_connection_send(dbusConn, reqMsg,
                  reinterpret_cast<dbus_uint32_t*>(&mSerialNum)) )
               {
                  dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                                 DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
               }
            }
            else
            {
               if ( !dbus_connection_send_with_reply(dbusConn, reqMsg,
                  &mPendingCall, mMsecTimeout) )
               {
                  dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                                 DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
               }
               else
               {
                  if ( !dbus_pending_call_set_notify(mPendingCall,
                     InvokeCmd::onPendingCallNotify, this, 0) )
                  {
                     dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                                 DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);   
                  }
                  else
                  {
                     try
                     {
                        // Register this pending command so we can
                        // remove it later
                        mConn->registerPending(this);
                        
                        // We don't want the command dispatcher to
                        // destroy this command after a request has been
                        // made and we're waiting on the result.
                        mExecAndDestroy = false;
                     }
                     catch ( ... )
                     {
                        // Failed to register the command in the pending
                        // command list so we'll cancel the request.
                        dbus_pending_call_cancel(mPendingCall);
                        
                        dispatchResult(DBUSIPC_MAKE_ERROR(
                              DBUSIPC_ERROR_LEVEL_ERROR,
                              DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                              DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
                     }
                  }
               }
            }
         }
         // Free the request message (finally)
         dbus_message_unref(reqMsg);
      }
   }
}

void InvokeCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 != mPendingCall )
   {
      // If the call already completed by the time cancel was called then ...
      if ( dbus_pending_call_get_completed(mPendingCall) )
      {
         DBusMessage* reply = dbus_pending_call_steal_reply(mPendingCall);
         if ( 0 != reply )
         {
            // Free the up reply message
            dbus_message_unref(reply);
         }
      }
      else
      {
         dbus_pending_call_cancel(mPendingCall);
      }
   }
   
   dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                  DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_CANCELLED),
                  DBUSIPC_ERR_NAME_CANCELLED, "Method cancelled", 0);
}


void InvokeCmd::dispatchResult
   (
   DBUSIPC_tError     errCode,
   DBUSIPC_tConstStr  errName,
   DBUSIPC_tConstStr  errMsg,
   DBUSIPC_tConstStr  result
   )
{
   // If this is a synchronous request then ...
   if ( 0 != mSem )
   {
      // The order of these operations is important . . .
      if ( (0 != mResponse) && (0 != *mResponse) )
      {
         (*mResponse)->result = (0 == result) ? 0 : strdup(result);
         (*mResponse)->status.errCode = errCode;
         (*mResponse)->status.errName = ( 0 == errName ) ? 0 : strdup(errName);
         (*mResponse)->status.errMsg = (0 == errMsg) ? 0 : strdup(errMsg);
      }
      // Wake up the blocked client thread
      mSem->post();
   }
   
   if ( mOnResult )
   {
      DBUSIPC_tCallbackStatus status = { errCode, errName, errMsg };
      mOnResult(&status, result, mUserToken);
   }   
}


void InvokeCmd::onPendingCallNotify
   (
   DBusPendingCall*  call,
   void*             userData
   )
{
   InvokeCmd* cmd = static_cast<InvokeCmd*>(userData);
   assert( 0 != cmd );

   DBusMessage* reply = dbus_pending_call_steal_reply(call);
   if ( 0 == reply )
   {
      cmd->dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                        DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL),
                        DBUSIPC_ERR_NAME_INTERNAL,
                        "Failed to retrieve reply message", 0);
   }
   else
   {
      int32_t replyType = dbus_message_get_type(reply);
      
      // If this is an error message then ...
      if ( DBUS_MESSAGE_TYPE_ERROR == replyType )
      {
         DBUSIPC_tConstStr errName = dbus_message_get_error_name(reply);
         DBUSIPC_tConstStr errMsg(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &errMsg,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract error "
                       "message");
         }
         
         cmd->dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
               DBUSIPC_DOMAIN_DBUS_LIB, DBUSIPC_ERR_DBUS), errName, errMsg, 0);
      }
      else if ( DBUS_MESSAGE_TYPE_METHOD_RETURN == replyType )
      {
         DBUSIPC_tConstStr result(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &result,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract results");
         }
         
         cmd->dispatchResult(DBUSIPC_ERROR_NONE, DBUSIPC_ERR_NAME_OK, 0,
                              result);
      }
      else
      {
         TRACE_WARN("onPendingCallNotify: Received unexpected D-Bus "
                    "message type (%d)", replyType);
      }

      // Unregister the pending command
      cmd->mConn->unregisterPending(cmd);
      
      // Free the up reply message
      dbus_message_unref(reply);
   }
   
   // This command is done - destroy ourselves
   delete cmd;
}


bool InvokeCmd::execAndDestroy() const
{
   return mExecAndDestroy;
}


//=====================================
//
// EmitCmd Implementation
//
//=====================================

EmitCmd::EmitCmd
   (
   DBUSIPC_tSvcRegHnd       regHnd,
   DBUSIPC_tConstStr        sigName,
   DBUSIPC_tConstStr        parameters,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
   : BaseCommand()
   , mSvcReg(static_cast<ServiceRegistration*>(regHnd))
   , mSignalName(sigName ? sigName : "")
   , mParams(parameters ? parameters : EMPTY_OBJECT)
   , mOnStatus(onStatus)
   , mUserToken(token)
   , mSem(0)
   , mStatus(0)
{
   
}


EmitCmd::EmitCmd
   (
   DBUSIPC_tSvcRegHnd       regHnd,
   DBUSIPC_tConstStr        sigName,
   DBUSIPC_tConstStr        parameters,
   Semaphore*              sem,
   DBUSIPC_tError*          status
   )
   : BaseCommand()
   , mSvcReg(static_cast<ServiceRegistration*>(regHnd))
   , mSignalName(sigName ? sigName : "")
   , mParams(parameters ? parameters : EMPTY_OBJECT)
   , mOnStatus(0)
   , mUserToken(0)
   , mSem(sem)
   , mStatus(status)
{
   
}
      
      
EmitCmd::~EmitCmd()
{
   
}


void EmitCmd::dispatchStatus
   (
   DBUSIPC_tError     errCode,
   DBUSIPC_tConstStr  errName,
   DBUSIPC_tConstStr  errMsg
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = errCode;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnStatus )
   {
      DBUSIPC_tCallbackStatus status = {errCode, errName, errMsg};
      mOnStatus(&status, mUserToken);
   }   
}


void EmitCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   dispatchStatus(DBUSIPC_MAKE_ERROR(
                  DBUSIPC_ERROR_LEVEL_ERROR,
                  DBUSIPC_DOMAIN_IPC_LIB,
                  DBUSIPC_ERR_CANCELLED),
                  DBUSIPC_ERR_NAME_CANCELLED, 0);
}


void EmitCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 != mSvcReg )
   {
      DBusMessage* pSignal = dbus_message_new_signal(mSvcReg->getObjectPath(),
                                       DBUSIPC_INTERFACE_NAME,
                                       DBUSIPC_INTERFACE_SIGNAL_NAME);
      if ( 0 == pSignal )
      {
         dispatchStatus(DBUSIPC_MAKE_ERROR(
                        DBUSIPC_ERROR_LEVEL_ERROR,
                        DBUSIPC_DOMAIN_IPC_LIB,
                        DBUSIPC_ERR_NO_MEMORY),
                        DBUSIPC_ERR_NAME_NO_MEMORY,
                        "Cannot allocate signal");
      }
      else
      {
         DBUSIPC_tConstStr dbusSignal = mSignalName.c_str();
         DBUSIPC_tConstStr dbusParams = mParams.c_str();
         if ( !dbus_message_append_args(pSignal, DBUS_TYPE_STRING,
               &dbusSignal, DBUS_TYPE_STRING, &dbusParams,
               DBUS_TYPE_INVALID) )
         {
            dispatchStatus(DBUSIPC_MAKE_ERROR(
                           DBUSIPC_ERROR_LEVEL_ERROR,
                           DBUSIPC_DOMAIN_IPC_LIB,
                           DBUSIPC_ERR_NO_MEMORY),
                           DBUSIPC_ERR_NAME_NO_MEMORY,
                           "Cannot attach parameters to signal");   
         }
         else
         {
            dbus_uint32_t serialNum;
            DBusConnection* dbusConn = Connection::getDBusConnection(
                                                   mSvcReg->getConnection());
            // If we're not connected then ...
            if ( (0 == dbusConn) ||
               !dbus_connection_get_is_connected(dbusConn) )
            {
               dispatchStatus(DBUSIPC_MAKE_ERROR(
                              DBUSIPC_ERROR_LEVEL_ERROR,
                              DBUSIPC_DOMAIN_IPC_LIB,
                              DBUSIPC_ERR_NOT_CONNECTED),
                              DBUSIPC_ERR_NAME_NOT_CONNECTED,
                              "Not connected to the bus");
            }
            else if ( !dbus_connection_send(dbusConn, pSignal, &serialNum) )
            {
               dispatchStatus(DBUSIPC_MAKE_ERROR(
                              DBUSIPC_ERROR_LEVEL_ERROR,
                              DBUSIPC_DOMAIN_IPC_LIB,
                              DBUSIPC_ERR_CONN_SEND),
                              DBUSIPC_ERR_NAME_CONN_SEND,
                              "Failed to send message over bus");   
            }
            else
            {
               dispatchStatus(DBUSIPC_ERROR_NONE, DBUSIPC_ERR_NAME_OK, 0);
            }
         }
         
         // Free the signal
         dbus_message_unref(pSignal);
      }
   }
}


//=====================================
//
// CancelCmd Implementation
//
//=====================================

CancelCmd::CancelCmd
   (
   DBUSIPC_tHandle handle,
   Semaphore*     sem,
   DBUSIPC_tError* status
   )
   : BaseCommand()
   , mHandleWillCancel(handle)
   , mSem(sem)
   , mStatus(status)
{  
}


CancelCmd::~CancelCmd()
{
   
}


void CancelCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   dispatch(DBUSIPC_MAKE_ERROR(
            DBUSIPC_ERROR_LEVEL_ERROR,
            DBUSIPC_DOMAIN_IPC_LIB,
            DBUSIPC_ERR_CANCELLED));
}


void CancelCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   DBUSIPC_tError status = dispatcher.cancelCommand(mHandleWillCancel);
   dispatch(status);
   if ( DBUSIPC_IS_ERROR(status ) )
   {
      TRACE_WARN("CancelCmd.execute: Failed to cancel command "
            "(hnd=%d, err=0x%0X", mHandleWillCancel, status);
   }
}

void CancelCmd::dispatch
   (
   DBUSIPC_tError error
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = error;
      }
      
      mSem->post();
   }
}

//=====================================
//
// ReturnResultCmd Implementation
//
//=====================================

ReturnResultCmd::ReturnResultCmd
   (
   DBUSIPC_tReqContext      context,
   DBUSIPC_tConstStr        result,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
   : BaseCommand()
   , mReqContext(static_cast<RequestContext*>(context))
   , mResult(result ? result : EMPTY_OBJECT)
   , mOnStatus(onStatus)
   , mUserToken(token)
   , mSem(0)
   , mStatus(0)
{
}


ReturnResultCmd::ReturnResultCmd
   (
   DBUSIPC_tReqContext      context,
   DBUSIPC_tConstStr        result,
   Semaphore*              sem,
   DBUSIPC_tError*          status
   )
   : BaseCommand()
   , mReqContext(static_cast<RequestContext*>(context))
   , mResult(result ? result : EMPTY_OBJECT)
   , mOnStatus(0)
   , mUserToken(0)
   , mSem(sem)
   , mStatus(status)
{
}


ReturnResultCmd::~ReturnResultCmd()
{   
}


void ReturnResultCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   dispatchStatus(DBUSIPC_MAKE_ERROR(
                  DBUSIPC_ERROR_LEVEL_ERROR,
                  DBUSIPC_DOMAIN_IPC_LIB,
                  DBUSIPC_ERR_CANCELLED),
                  DBUSIPC_ERR_NAME_CANCELLED, 0);   
}


void ReturnResultCmd::dispatchStatus
   (
   DBUSIPC_tError     errCode,
   DBUSIPC_tConstStr  errName,
   DBUSIPC_tConstStr  errMsg
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = errCode;
      }
      
      mSem->post();
   }
   
   
   if ( 0 != mOnStatus )
   {
      DBUSIPC_tCallbackStatus status = {errCode, errName, errMsg};
      mOnStatus(&status, mUserToken);
   }   
}


void ReturnResultCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 == mReqContext )
   {
      dispatchStatus(DBUSIPC_MAKE_ERROR(
                     DBUSIPC_ERROR_LEVEL_ERROR,
                     DBUSIPC_DOMAIN_IPC_LIB,
                     DBUSIPC_ERR_BAD_ARGS),
                     DBUSIPC_ERR_NAME_BAD_ARGS,
                     "NULL request context");
   }
   else
   {
      DBUSIPC_tError status = mReqContext->sendReply(mResult.c_str());
      dispatchStatus(status, 0, 0);
   }
}


//=====================================
//
// ReturnErrorCmd Implementation
//
//=====================================

ReturnErrorCmd::ReturnErrorCmd
   (
   DBUSIPC_tReqContext      context,
   DBUSIPC_tConstStr        name,
   DBUSIPC_tConstStr        msg,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
   : BaseCommand()
   , mReqContext(static_cast<RequestContext*>(context))
   , mErrName(name ? name : "")
   , mErrMsg(msg ? msg : EMPTY_OBJECT)
   , mOnStatus(onStatus)
   , mUserToken(token)
   , mSem(0)
   , mStatus(0)
{
}


ReturnErrorCmd::ReturnErrorCmd
   (
   DBUSIPC_tReqContext      context,
   DBUSIPC_tConstStr        name,
   DBUSIPC_tConstStr        msg,
   Semaphore*              sem,
   DBUSIPC_tError*          status
   )
   : BaseCommand()
   , mReqContext(static_cast<RequestContext*>(context))
   , mErrName(name ? name : "")
   , mErrMsg(msg ? msg : EMPTY_OBJECT)
   , mOnStatus(0)
   , mUserToken(0)
   , mSem(sem)
   , mStatus(status)
{
}


ReturnErrorCmd::~ReturnErrorCmd()
{   
}


void ReturnErrorCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   dispatchStatus(DBUSIPC_MAKE_ERROR(
                  DBUSIPC_ERROR_LEVEL_ERROR,
                  DBUSIPC_DOMAIN_IPC_LIB,
                  DBUSIPC_ERR_CANCELLED),
                  DBUSIPC_ERR_NAME_CANCELLED, 0);   
}


void ReturnErrorCmd::dispatchStatus
   (
   DBUSIPC_tError     errCode,
   DBUSIPC_tConstStr  errName,
   DBUSIPC_tConstStr  errMsg
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = errCode;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnStatus )
   {
      DBUSIPC_tCallbackStatus status = {errCode, errName, errMsg};
      mOnStatus(&status, mUserToken);
   }   
}


void ReturnErrorCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 == mReqContext )
   {
      dispatchStatus(DBUSIPC_MAKE_ERROR(
                     DBUSIPC_ERROR_LEVEL_ERROR,
                     DBUSIPC_DOMAIN_IPC_LIB,
                     DBUSIPC_ERR_BAD_ARGS),
                     DBUSIPC_ERR_NAME_BAD_ARGS,
                     "NULL request context");
   }
   else
   {
      DBUSIPC_tError status = mReqContext->sendError(mErrName.c_str(),
                                                    mErrMsg.c_str());
      dispatchStatus(status, 0, 0);
   }
}


//======================================
//
// FreeRequestContextCmd Implementation
//
//======================================

FreeRequestContextCmd::FreeRequestContextCmd
   (
   DBUSIPC_tReqContext   context
   )
   : BaseCommand()
   , mReqContext(static_cast<RequestContext*>(context))
{
}
   

FreeRequestContextCmd::~FreeRequestContextCmd()
{   
}


void FreeRequestContextCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   delete mReqContext;
}


//======================================
//
// ShutdownCmd Implementation
//
//======================================

ShutdownCmd::ShutdownCmd
   (
   Semaphore*  sem
   )
   : BaseCommand()
   , mSem(sem)
{
   
}


ShutdownCmd::~ShutdownCmd()
{
   
}


void ShutdownCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   assert( 0 != mSem );

   // Release all the connections
   Connection::forceReleaseAll();
   
   // Tell the dispatcher thread to stop. We CANNOT wait for it
   // to stop since we're running inside this thread and a
   // deadlock condition would occur.
   dispatcher.stop();
   
   // Wake up the shutdown function in the client thread
   mSem->post();
}


//======================================
//
// NameHasOwnerCmd Implementation
//
//======================================

NameHasOwnerCmd::NameHasOwnerCmd
   (
   DBUSIPC_tConnection   conn,
   DBUSIPC_tConstStr     busName,
   DBUSIPC_tBool*        hasOwner,
   Semaphore*           sem,
   DBUSIPC_tError*       status
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mBusName(busName ? busName : "")
   , mHasOwner(hasOwner)
   , mOnHasOwner(0)
   , mUserToken(0)
   , mSem(sem)
   , mStatus(status)
   , mPendingCall(0)
   , mExecAndDestroy(true)
{   
}



NameHasOwnerCmd::NameHasOwnerCmd
   (
   DBUSIPC_tConnection            conn,
   DBUSIPC_tConstStr              busName,
   DBUSIPC_tNameHasOwnerCallback  onHasOwner,
   DBUSIPC_tUserToken             token
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mBusName(busName ? busName : "")
   , mHasOwner(0)
   , mOnHasOwner(onHasOwner)
   , mUserToken(token)
   , mSem(0)
   , mStatus(0)
   , mPendingCall(0)
   , mExecAndDestroy(true)
{   
}


NameHasOwnerCmd::~NameHasOwnerCmd()
{
   if ( 0 != mPendingCall )
   {
      dbus_pending_call_unref(mPendingCall);
   }
}


void NameHasOwnerCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   DBusConnection* dbusConn = Connection::getDBusConnection(mConn);
   if ( !dbus_connection_get_is_connected(dbusConn) )
   {
      dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                     DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NOT_CONNECTED),
                     DBUSIPC_ERR_NAME_NOT_CONNECTED, "Not connected", 0);
   }
   else
   {
      DBusMessage* reqMsg = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
                        DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                        "NameHasOwner");
      if ( 0 == reqMsg )
      {
         dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                        DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                        DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
      }
      else
      {
         dbus_message_set_no_reply(reqMsg, false);
         
         // Pack in the arguments
         DBUSIPC_tConstStr busName = mBusName.c_str();
         if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
            &busName, DBUS_TYPE_INVALID) )
         {
            dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                           DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                           DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
         }
         else
         {
            if ( !dbus_connection_send_with_reply(dbusConn, reqMsg,
               &mPendingCall, -1) )
            {
               dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                              DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                              DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
            }
            else
            {
               if ( !dbus_pending_call_set_notify(mPendingCall,
                  NameHasOwnerCmd::onPendingCallNotify, this, 0) )
               {
                  dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                              DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                              DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);   
               }
               else
               {
                  try
                  {
                     // Register this pending command so we can
                     // remove it later
                     mConn->registerPending(this);
                     
                     // We don't want the command dispatcher to
                     // destroy this command after a request has been
                     // made and we're waiting on the result.
                     mExecAndDestroy = false;
                  }
                  catch ( ... )
                  {
                     // Failed to register the command in the pending
                     // command list so we'll cancel the request.
                     dbus_pending_call_cancel(mPendingCall);
                     
                     dispatchResult(DBUSIPC_MAKE_ERROR(
                           DBUSIPC_ERROR_LEVEL_ERROR,
                           DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_NO_MEMORY),
                           DBUSIPC_ERR_NAME_NO_MEMORY, "Out of memory", 0);
                  }
               }
            }
         }
         // Free the request message (finally)
         dbus_message_unref(reqMsg);
      }
   }
}


void NameHasOwnerCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   if ( 0 != mPendingCall )
   {
      // If the call already completed by the time cancel was called then ...
      if ( dbus_pending_call_get_completed(mPendingCall) )
      {
         DBusMessage* reply = dbus_pending_call_steal_reply(mPendingCall);
         if ( 0 != reply )
         {
            // Free the up reply message
            dbus_message_unref(reply);
         }
      }
      else
      {
         dbus_pending_call_cancel(mPendingCall);
      }
   }
   
   dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                  DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_CANCELLED),
                  DBUSIPC_ERR_NAME_CANCELLED, "Method cancelled", 0);   
}


bool NameHasOwnerCmd::execAndDestroy() const
{
   return mExecAndDestroy;
}


void NameHasOwnerCmd::dispatchResult
   (
   DBUSIPC_tError     errCode,
   DBUSIPC_tConstStr  errName,
   DBUSIPC_tConstStr  errMsg,
   DBUSIPC_tBool      hasOwner
   )
{
   // If this is a synchronous request then ...
   if ( 0 != mSem )
   {
      if ( 0 != mStatus)
      {
         *mStatus = errCode;
      }
      
      if ( 0 != mHasOwner )
      {
         *mHasOwner = hasOwner;
      }
      
      // Wake up the blocked client thread
      mSem->post();
   }
   
   if ( 0 != mOnHasOwner )
   {
      DBUSIPC_tCallbackStatus status = { errCode, errName, errMsg };
      mOnHasOwner(&status, mBusName.c_str(), hasOwner, mUserToken);
   }   
}


void NameHasOwnerCmd::onPendingCallNotify
   (
   DBusPendingCall*  call,
   void*             userData
   )
{
   NameHasOwnerCmd* cmd = static_cast<NameHasOwnerCmd*>(userData);
   assert( 0 != cmd );

   DBusMessage* reply = dbus_pending_call_steal_reply(call);
   if ( 0 == reply )
   {
      cmd->dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                        DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL),
                        DBUSIPC_ERR_NAME_INTERNAL,
                        "Failed to retrieve reply message", 0);
   }
   else
   {
      int32_t replyType = dbus_message_get_type(reply);
      
      // If this is an error message then ...
      if ( DBUS_MESSAGE_TYPE_ERROR == replyType )
      {
         DBUSIPC_tConstStr errName = dbus_message_get_error_name(reply);
         DBUSIPC_tConstStr errMsg(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &errMsg,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract error message");
         }
         
         cmd->dispatchResult(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
               DBUSIPC_DOMAIN_DBUS_LIB, DBUSIPC_ERR_DBUS), errName, errMsg, 0);
      }
      else if ( DBUS_MESSAGE_TYPE_METHOD_RETURN == replyType )
      {
         dbus_bool_t result(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_BOOLEAN, &result,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract results");
         }
         
         cmd->dispatchResult(DBUSIPC_ERROR_NONE, DBUSIPC_ERR_NAME_OK, 0, result);
      }
      else
      {
         TRACE_WARN("onPendingCallNotify: Received unexpected D-Bus "
                    "message type (%d)", replyType);
      }

      // Unregister the pending command
      cmd->mConn->unregisterPending(cmd);
      
      // Free the up reply message
      dbus_message_unref(reply);
      
      // This command is done - destroy ourselves
      delete cmd;
   }   
}


//======================================
//
// SubscribeOwnerChangedCmd Implementation
//
//======================================

SubscribeOwnerChangedCmd::SubscribeOwnerChangedCmd
   (
   DBUSIPC_tConnection               conn,
   DBUSIPC_tConstStr                 busName,
   DBUSIPC_tNameOwnerChangedCallback onOwnerChanged,
   DBUSIPC_tSubscriptionCallback     onSubscription,
   DBUSIPC_tUserToken                token
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mBusName(busName ? busName : "")
   , mOnOwnerChanged(onOwnerChanged)
   , mOnSubscription(onSubscription)
   , mUserToken(token)
   , mSubHnd(0)
   , mSem(0)
   , mStatus(0)
   , mPendingCall(0)
   , mExecAndDestroy(true)
   , mSigSub()
{
   mSigSub.reset(new NameOwnerChangedSubscription(mConn, mBusName,
                        mOnOwnerChanged, mUserToken));
}


SubscribeOwnerChangedCmd::SubscribeOwnerChangedCmd
   (
   DBUSIPC_tConnection               conn,
   DBUSIPC_tConstStr                 busName,
   DBUSIPC_tNameOwnerChangedCallback onOwnerChanged,
   DBUSIPC_tUserToken                token,
   DBUSIPC_tSigSubHnd*               subHnd,
   Semaphore*                       sem,
   DBUSIPC_tError*                   status
   )
   : BaseCommand()
   , mConn(static_cast<Connection*>(conn))
   , mBusName(busName ? busName : "")
   , mOnOwnerChanged(onOwnerChanged)
   , mOnSubscription(0)
   , mUserToken(token)
   , mSubHnd(subHnd)
   , mSem(sem)
   , mStatus(status)
   , mPendingCall(0)
   , mExecAndDestroy(true)
   , mSigSub()
{
   mSigSub.reset(new NameOwnerChangedSubscription(mConn, mBusName,
                           mOnOwnerChanged, mUserToken));
}


SubscribeOwnerChangedCmd::~SubscribeOwnerChangedCmd()
{
   if ( 0 != mPendingCall )
   {
      dbus_pending_call_unref(mPendingCall);
   }
}


void SubscribeOwnerChangedCmd::cancel
   (
   Dispatcher& dispatcher
   )
{
   DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                    DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_CANCELLED),
                                    DBUSIPC_ERR_NAME_CANCELLED, 0};
   dispatch(status, 0);
}


bool SubscribeOwnerChangedCmd::execAndDestroy() const
{
   return mExecAndDestroy;
}


void SubscribeOwnerChangedCmd::execute
   (
   Dispatcher& dispatcher
   )
{
   DBusConnection* dbusConn = Connection::getDBusConnection(mConn);
   // Let's make sure this connection is still open/exists
   if ( !Connection::connectionExists(mConn) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_NOT_FOUND),
                                       DBUSIPC_ERR_NAME_NOT_FOUND, 0};
      dispatch(status, 0);         
   }
   else
   {
      DBusMessage* reqMsg = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
                        DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                        "AddMatch");
      if ( 0 == reqMsg )
      {
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NO_MEMORY),
                                          DBUSIPC_ERR_NAME_NO_MEMORY,
                                          "Out of memory"};
         dispatch(status, DBUSIPC_INVALID_HANDLE);
      }
      else
      {
         dbus_message_set_no_reply(reqMsg, false);
         
         // Pack in the arguments
         DBUSIPC_tConstStr rule = mSigSub->getRule();
         if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
            &rule, DBUS_TYPE_INVALID) )
         {
            DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
            dispatch(status, DBUSIPC_INVALID_HANDLE);
         }
         else
         {
            if ( !dbus_connection_send_with_reply(dbusConn, reqMsg,
               &mPendingCall, -1) )
            {
               DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                DBUSIPC_ERROR_LEVEL_ERROR,
                                                DBUSIPC_DOMAIN_IPC_LIB,
                                                DBUSIPC_ERR_NO_MEMORY),
                                                DBUSIPC_ERR_NAME_NO_MEMORY,
                                                "Out of memory"};
               dispatch(status, DBUSIPC_INVALID_HANDLE);
            }
            else
            {
               if ( !dbus_pending_call_set_notify(mPendingCall,
                  SubscribeOwnerChangedCmd::onPendingCallNotify, this, 0) )
               {
                  DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                   DBUSIPC_ERROR_LEVEL_ERROR,
                                                   DBUSIPC_DOMAIN_IPC_LIB,
                                                   DBUSIPC_ERR_NO_MEMORY),
                                                   DBUSIPC_ERR_NAME_NO_MEMORY,
                                                   "Out of memory"};
                  dispatch(status, DBUSIPC_INVALID_HANDLE);   
               }
               else
               {
                  try
                  {
                     // Register this pending command so we can
                     // remove it later
                     mConn->registerPending(this);
                     
                     // We don't want the command dispatcher to
                     // destroy this command after a request has been
                     // made and we're waiting on the result.
                     mExecAndDestroy = false;
                  }
                  catch ( ... )
                  {
                     // Failed to register the command in the pending
                     // command list so we'll cancel the request.
                     dbus_pending_call_cancel(mPendingCall);
                     
                     DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                                      DBUSIPC_ERROR_LEVEL_ERROR,
                                                      DBUSIPC_DOMAIN_IPC_LIB,
                                                      DBUSIPC_ERR_NO_MEMORY),
                                                      DBUSIPC_ERR_NAME_NO_MEMORY,
                                                      "Out of memory"};
                     dispatch(status, DBUSIPC_INVALID_HANDLE);
                  }
               }
            }
         }
         // Free the request message (finally)
         dbus_message_unref(reqMsg);
      }
   }      
}


void SubscribeOwnerChangedCmd::dispatch
   (
   const DBUSIPC_tCallbackStatus& status,
   DBUSIPC_tSigSubHnd             subHnd
   )
{
   if ( 0 != mSem )
   {
      if ( 0 != mStatus )
      {
         *mStatus = status.errCode;
      }
      
      if ( 0 != mSubHnd )
      {
         *mSubHnd = subHnd;
      }
      
      mSem->post();
   }
   
   if ( 0 != mOnSubscription )
   {
      mOnSubscription(&status, subHnd, mUserToken);
   }   
}


void SubscribeOwnerChangedCmd::onPendingCallNotify
   (
   DBusPendingCall*  call,
   void*             userData
   )
{
   SubscribeOwnerChangedCmd* cmd =
                        static_cast<SubscribeOwnerChangedCmd*>(userData);
   assert( 0 != cmd );

   DBusConnection* dbusConn = Connection::getDBusConnection(cmd->mConn);
   DBusMessage* reply = dbus_pending_call_steal_reply(call);
   if ( (0 == reply) || !dbus_connection_get_is_connected(dbusConn) )
   {
      DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                       DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_INTERNAL),
                                       DBUSIPC_ERR_NAME_INTERNAL,
                                       "Failed to retrieve reply message"};
      cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
   }
   else
   {
      int32_t replyType = dbus_message_get_type(reply);
      
      // If this is an error message then ...
      if ( DBUS_MESSAGE_TYPE_ERROR == replyType )
      {
         DBUSIPC_tConstStr errName = dbus_message_get_error_name(reply);
         DBUSIPC_tConstStr errMsg(0);
         if ( !dbus_message_get_args(reply, 0, DBUS_TYPE_STRING, &errMsg,
            DBUS_TYPE_INVALID) )
         {
            TRACE_INFO("onPendingCallNotify: Failed to extract error "
                       "message");
         }
         
         DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                          DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_DBUS),
                                          errName, errMsg};
         cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
      }
      else if ( DBUS_MESSAGE_TYPE_METHOD_RETURN == replyType )
      {
         try
         {
            cmd->mConn->subscribeSignal(cmd->mSigSub.get());
            DBUSIPC_tCallbackStatus status = {DBUSIPC_ERROR_NONE,
                                             DBUSIPC_ERR_NAME_OK, 0};
            cmd->dispatch(status, cmd->mSigSub.get());
            // The connection now owns the subscription
            cmd->mSigSub.release();
         }
         catch ( ... )
         {
            // Make our best effort to remove the match we just set
            DBusMessage* reqMsg = dbus_message_new_method_call(
                              DBUS_SERVICE_DBUS,
                              DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS,
                              "RemoveMatch");
            if ( 0 == reqMsg )
            {
               TRACE_WARN("onPendingCallNotify: "
                          "Failed to remove match on error");
            }
            else
            {
               // We don't care about the reply - we're making a best
               // effort attempt to remove the match
               dbus_message_set_no_reply(reqMsg, true);
               
               // Pack in the arguments
               DBUSIPC_tConstStr rule = cmd->mSigSub->getRule();
               if ( !dbus_message_append_args(reqMsg, DBUS_TYPE_STRING,
                  &rule, DBUS_TYPE_INVALID) )
               {
                  TRACE_WARN("onPendingCallNotify: "
                             "Failed to remove match on error");
               }
               else
               {
                  dbus_uint32_t serNum;
                  if ( !dbus_connection_send(dbusConn, reqMsg,
                     &serNum) )
                  {
                     TRACE_WARN("onPendingCallNotify: "
                                "Failed to remove match on error");
                  }
               }
               
               dbus_message_unref(reqMsg);
            }
            
            DBUSIPC_tCallbackStatus status = {DBUSIPC_MAKE_ERROR(
                                             DBUSIPC_ERROR_LEVEL_ERROR,
                                             DBUSIPC_DOMAIN_IPC_LIB,
                                             DBUSIPC_ERR_NO_MEMORY),
                                             DBUSIPC_ERR_NAME_NO_MEMORY,
                                             "Out of memory"};
            cmd->dispatch(status, DBUSIPC_INVALID_HANDLE);
         }
      }
      else
      {
         TRACE_WARN("onPendingCallNotify: Received unexpected D-Bus "
                    "message type (%d)", replyType);
      }

      // Unregister the pending command
      cmd->mConn->unregisterPending(cmd);
      
      // Free the up reply message
      dbus_message_unref(reply);
   }
   
   // This command is done - destroy ourselves
   delete cmd;
}


