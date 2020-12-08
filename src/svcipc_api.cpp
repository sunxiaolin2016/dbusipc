
#include <memory>
#include <cstdlib>
#include <errno.h>
#include "dbusipc/dbusipc.h"
#include "dbus/dbus.h"
#include "Dispatcher.hpp"
#include "SingletonHolder.hpp"
#include "Exceptions.hpp"
#include "Command.hpp"
#include "Connection.hpp"
#include "Semaphore.hpp"
#include "trace.h"


// Global Dispatcher holder
static SingletonHolder<Dispatcher> gDispatcher;

//
// Helper function for submitting commands
//
static DBUSIPC_tError DBUSIPC_submitCmd
   (
   BaseCommand*      cmd,
   DBUSIPC_tHandle&   hnd
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);

   try
   {

      hnd = gDispatcher.get()->submitCommand(cmd);
      // If the handle was valid then ...
      if ( hnd != DBUSIPC_INVALID_HANDLE )
      {
         // The dispatcher now owns the command
         cmd = 0;
      }
      else
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_CMD_SUBMISSION);
      }
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   if ( 0 != cmd )
   {
      delete cmd;
   }

   return status;
}


DBUSIPC_tError DBUSIPC_initialize(void)
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   
   NSysDep::DBUSIPC_setEnvironmentVariable();

   if ( !NSysDep::DBUSIPC_startupNetworkStack() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_C_LIB, DBUSIPC_ERR_INTERNAL);
   }
   // It's safe to call this function more than once
   else if ( !dbus_threads_init_default() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_DBUS);
   }
   else
   {
      try
      {
         // If the dispatcher is not already running then ...
         if ( !gDispatcher.get()->isRunning() )
         {
            // Start the thread and block until it's running
            int32_t rc = gDispatcher.get()->start(true);
#if   OS_QNX_
            if ( EOK != rc )
#elif OS_LINUX
	     if ( EOK != rc )
#elif WIN32
            if ( ERROR_SUCCESS != rc )
#endif
            {
               status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_C_LIB, rc);
            }
            else
            {
               // Check if we should modify the thread's priority
               std::string newPriorityString =
                     NSysDep::DBUSIPC_getenv("DBUSIPC_DISPATCH_PRIORITY");

               if ( !newPriorityString.empty() )
               {
                  // Set the thread to new priority
                  int32_t priority = std::atoi(newPriorityString.c_str());
                  if ( !gDispatcher.get()->setPriority(priority) )
                  {
                     TRACE_WARN("DBUSIPC_initialize: "
                                "failed to set priority to %d", priority);
                  }
               }
            }
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


void DBUSIPC_shutdown(void)
{
   if ( gDispatcher.get()->isRunning() )
   {
      try
      {
         DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
         DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<ShutdownCmd> cmd(new ShutdownCmd(&sem));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            // Block waiting for the shutdown request to complete
            sem.wait();
         }
      }
      catch ( const std::exception& e )
      {
         TRACE_WARN("DBUSIPC_shutdown: caught execption: %s", e.what());
      }

      // Now wait for the dispatcher to exit
      gDispatcher.get()->wait(Thread::INFINITE_WAIT);
   }

   // Release D-Bus related resources
   dbus_shutdown();

   // Explicitly destroy the dispatcher. If user forgets to shutdown
   // then the Singleton holder will destroy the dispatcher when
   // the static destructors are called.
   //                      *** Note ***
   // The dispatcher *cannot* be destroyed before D-Bus has been
   // shut down (dbus_shutdown) because while shutting down it
   // invokes several callbacks into code owned by the dispatcher.
   // If you reverse this order then SEG-FAULTs occur when terminating
   // programs.
   try
   {
      gDispatcher.destroy();
   }
   catch ( const std::exception& e )
   {
      TRACE_WARN("DBUSIPC_shutdown: execption destroying "
                 "dispatcher: %s", e.what());
   }

   // Shutdown the network stack if necessary
   (void)NSysDep::DBUSIPC_shutdownNetworkStack();
}


DBUSIPC_tError DBUSIPC_asyncOpenConnection
   (
   DBUSIPC_tConstStr           address,
   DBUSIPC_tBool               openPrivate,
   DBUSIPC_tConnectionCallback onConnect,
   DBUSIPC_tUserToken          token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<OpenConnectionCmd> cmd(
               new OpenConnectionCmd(address,
                                    openPrivate,
                                    onConnect,
                                    token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_openConnection
   (
   DBUSIPC_tConstStr     address,
   DBUSIPC_tBool         openPrivate,
   DBUSIPC_tConnection*  conn
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 == conn )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<OpenConnectionCmd> cmd(
                  new OpenConnectionCmd(address, openPrivate, &sem, &opStatus,
                                       conn));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncGetConnection
   (
   DBUSIPC_tConnType           connType,
   DBUSIPC_tBool               openPrivate,
   DBUSIPC_tConnectionCallback onConnect,
   DBUSIPC_tUserToken          token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<GetConnectionCmd> cmd(
               new GetConnectionCmd(connType,
                                    openPrivate,
                                    onConnect,
                                    token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_getConnection
   (
   DBUSIPC_tConnType     connType,
   DBUSIPC_tBool         openPrivate,
   DBUSIPC_tConnection*  conn
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 == conn )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<GetConnectionCmd> cmd(
                  new GetConnectionCmd(connType, openPrivate, &sem, &opStatus,
                                       conn));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncCloseConnection
   (
   DBUSIPC_tConnection  conn
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<CloseConnectionCmd> cmd(new CloseConnectionCmd(conn));
      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}

DBUSIPC_tError DBUSIPC_closeConnection
   (
   DBUSIPC_tConnection  conn
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<CloseConnectionCmd> cmd(new CloseConnectionCmd(
                                                conn, &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncInvoke
   (
   DBUSIPC_tConnection      conn,
   DBUSIPC_tConstStr        busName,
   DBUSIPC_tConstStr        objPath,
   DBUSIPC_tConstStr        method,
   DBUSIPC_tConstStr        parameters,
   DBUSIPC_tBool            noReplyExpected,
   DBUSIPC_tUInt32          msecTimeout,
   DBUSIPC_tResultCallback  onResult,
   DBUSIPC_tHandle*         handle,
   DBUSIPC_tUserToken       token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 != handle )
   {
      *handle = DBUSIPC_INVALID_HANDLE;
   }

   if ( (0 == busName) || (0 == method) )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else
   {
      try
      {
         std::auto_ptr<InvokeCmd> cmd(new InvokeCmd(conn, busName, objPath,
               method, parameters, noReplyExpected, msecTimeout, onResult,
               token));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( (0 != handle) && !DBUSIPC_IS_ERROR(status) )
         {
            *handle = hnd;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_invoke
   (
   DBUSIPC_tConnection   conn,
   DBUSIPC_tConstStr     busName,
   DBUSIPC_tConstStr     objPath,
   DBUSIPC_tConstStr     method,
   DBUSIPC_tConstStr     parameters,
   DBUSIPC_tUInt32       msecTimeout,
   DBUSIPC_tResponse**   response
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( (0 == busName) || (0 == method) || (0 == response) )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<InvokeCmd> cmd(new InvokeCmd(conn, busName, objPath,
               method, parameters, msecTimeout, response, &sem));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            // Block waiting for the request to complete
            sem.wait();
            if ( 0 == *response )
            {
               status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                          DBUSIPC_DOMAIN_IPC_LIB,
                                          DBUSIPC_ERR_NO_MEMORY);
            }
            else
            {
               status = (*response)->status.errCode;
            }
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB,
                                    DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


void DBUSIPC_freeResponse
   (
   DBUSIPC_tResponse* response
   )
{
   if ( 0 != response )
   {
      if ( 0 != response->status.errName )
      {
         std::free(response->status.errName);
		 response->status.errName = 0;
      }

      if ( 0 != response->status.errMsg )
      {
         std::free(response->status.errMsg);
		 response->status.errMsg =0;
      }

      if ( 0 != response->result )
      {
         std::free(response->result);
		 response->result = 0;
      }

      std::free(response);
	  response = 0;
   }
   return;
}


DBUSIPC_tError DBUSIPC_asyncCancel
   (
   DBUSIPC_tHandle handle
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<CancelCmd> cmd(new CancelCmd(handle));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_cancel
   (
   DBUSIPC_tHandle handle
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<CancelCmd> cmd(new CancelCmd(handle, &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}

DBUSIPC_tError DBUSIPC_asyncEmit
   (
   DBUSIPC_tSvcRegHnd       regHnd,
   DBUSIPC_tConstStr        sigName,
   DBUSIPC_tConstStr        parameters,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<EmitCmd> cmd(new EmitCmd
                     (regHnd, sigName, parameters, onStatus, token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_emit
   (
   DBUSIPC_tSvcRegHnd regHnd,
   DBUSIPC_tConstStr  sigName,
   DBUSIPC_tConstStr  parameters
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 == regHnd )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<EmitCmd> cmd(new EmitCmd(regHnd, sigName, parameters,
                                                &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncSubscribe
   (
   DBUSIPC_tConnection            conn,
   DBUSIPC_tConstStr              objPath,
   DBUSIPC_tConstStr              sigName,
   DBUSIPC_tSignalCallback        onSignal,
   DBUSIPC_tSubscriptionCallback  onSubscription,
   DBUSIPC_tUserToken             token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      if ( 0 == objPath )
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_BAD_ARGS);
      }
      else
      {
         std::auto_ptr<SubscribeCmd> cmd(new SubscribeCmd(conn, objPath,
                                         sigName, onSignal, onSubscription,
                                         token));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
      }
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_subscribe
   (
   DBUSIPC_tConnection      conn,
   DBUSIPC_tConstStr        objPath,
   DBUSIPC_tConstStr        sigName,
   DBUSIPC_tSignalCallback  onSignal,
   DBUSIPC_tUserToken       token,
   DBUSIPC_tSigSubHnd*      subHnd
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 == subHnd )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<SubscribeCmd> cmd(new SubscribeCmd(conn, objPath,
                                   sigName, onSignal, token, &sem, &opStatus,
                                   subHnd));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncUnsubscribe
   (
   DBUSIPC_tSigSubHnd       subHnd,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<UnsubscribeCmd> cmd(new UnsubscribeCmd(
            static_cast<SignalSubscription*>(subHnd),
            onStatus, token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_unsubscribe
   (
   DBUSIPC_tSigSubHnd subHnd
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 == subHnd )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<UnsubscribeCmd> cmd(new UnsubscribeCmd(
               static_cast<SignalSubscription*>(subHnd), &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncRegisterService
   (
   DBUSIPC_tConnection            conn,
   DBUSIPC_tConstStr              busName,
   DBUSIPC_tConstStr              objPath,
   DBUSIPC_tUInt32                flag,
   DBUSIPC_tRequestCallback       onRequest,
   DBUSIPC_tRegistrationCallback  onRegister,
   DBUSIPC_tUserToken             token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<RegisterServiceCmd> cmd(new RegisterServiceCmd
                     (conn, busName, objPath, flag, onRequest,
                     onRegister, token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_registerService
   (
   DBUSIPC_tConnection      conn,
   DBUSIPC_tConstStr        busName,
   DBUSIPC_tConstStr        objPath,
   DBUSIPC_tUInt32          flag,
   DBUSIPC_tRequestCallback onRequest,
   DBUSIPC_tUserToken       token,
   DBUSIPC_tSvcRegHnd*      regHnd
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 == regHnd )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<RegisterServiceCmd> cmd(new RegisterServiceCmd
                              (conn, busName, objPath, flag, onRequest,
                              token, &sem, &opStatus, regHnd));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncUnregisterService
   (
   DBUSIPC_tSvcRegHnd       regHnd,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<UnregisterServiceCmd> cmd(new UnregisterServiceCmd
                     (static_cast<ServiceRegistration*>(regHnd), onStatus,
                     token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_unregisterService
   (
   DBUSIPC_tSvcRegHnd regHnd
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 == regHnd )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<UnregisterServiceCmd> cmd(new UnregisterServiceCmd(
               static_cast<ServiceRegistration*>(regHnd), &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncReturnResult
   (
   DBUSIPC_tReqContext      context,
   DBUSIPC_tConstStr        result,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<ReturnResultCmd> cmd(new ReturnResultCmd(context,
                                          result, onStatus, token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_returnResult
   (
   DBUSIPC_tReqContext   context,
   DBUSIPC_tConstStr     result
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<ReturnResultCmd> cmd(new ReturnResultCmd(
                                 context, result, &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncReturnError
   (
   DBUSIPC_tReqContext      context,
   DBUSIPC_tConstStr        name,
   DBUSIPC_tConstStr        msg,
   DBUSIPC_tStatusCallback  onStatus,
   DBUSIPC_tUserToken       token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<ReturnErrorCmd> cmd(new ReturnErrorCmd(context,
                                    name ? name : DBUSIPC_INTERFACE_ERROR_NAME,
                                    msg, onStatus, token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_returnError
   (
   DBUSIPC_tReqContext   context,
   DBUSIPC_tConstStr     name,
   DBUSIPC_tConstStr     msg
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<ReturnErrorCmd> cmd(new ReturnErrorCmd(
                                    context,
                                    name ? name : DBUSIPC_INTERFACE_ERROR_NAME,
                                    msg, &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


void DBUSIPC_freeReqContext
   (
   DBUSIPC_tReqContext   context
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<FreeRequestContextCmd>
                                    cmd(new FreeRequestContextCmd(context));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }
}


DBUSIPC_tError DBUSIPC_nameHasOwner
   (
   DBUSIPC_tConnection   conn,
   DBUSIPC_tConstStr     busName,
   DBUSIPC_tBool*        hasOwner
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( 0 == hasOwner )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<NameHasOwnerCmd> cmd(new NameHasOwnerCmd
                              (conn, busName, hasOwner, &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncNameHasOwner
   (
   DBUSIPC_tConnection            conn,
   DBUSIPC_tConstStr              busName,
   DBUSIPC_tNameHasOwnerCallback  onHasOwner,
   DBUSIPC_tUserToken             token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<NameHasOwnerCmd> cmd(new NameHasOwnerCmd(conn,
                  busName, onHasOwner, token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_subscribeOwnerChanged
   (
   DBUSIPC_tConnection               conn,
   DBUSIPC_tConstStr                 busName,
   DBUSIPC_tNameOwnerChangedCallback onOwnerChanged,
   DBUSIPC_tUserToken                token,
   DBUSIPC_tSigSubHnd*               subHnd
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tError opStatus(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   if ( (0 == onOwnerChanged) || (0 == subHnd) )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB,
                                 DBUSIPC_ERR_BAD_ARGS);
   }
   else if ( gDispatcher.get()->isCurrentThread() )
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                       DBUSIPC_DOMAIN_IPC_LIB,
                                       DBUSIPC_ERR_DEADLOCK);
   }
   else
   {
      try
      {
         Semaphore sem(0 /* initially locked */);
         std::auto_ptr<SubscribeOwnerChangedCmd> cmd(new
               SubscribeOwnerChangedCmd(conn, busName, onOwnerChanged, token,
                                       subHnd, &sem, &opStatus));

         status = DBUSIPC_submitCmd(cmd.release(), hnd);
         if ( !DBUSIPC_IS_ERROR(status) )
         {
            sem.wait();
            status = opStatus;
         }
      }
      catch (const DBUSIPCError& e)
      {
         status = e.getError();
      }
      catch (const std::exception&)
      {
         status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                    DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
      }
   }

   return status;
}


DBUSIPC_tError DBUSIPC_asyncSubscribeOwnerChanged
   (
   DBUSIPC_tConnection               conn,
   DBUSIPC_tConstStr                 busName,
   DBUSIPC_tNameOwnerChangedCallback onOwnerChanged,
   DBUSIPC_tSubscriptionCallback     onSubscription,
   DBUSIPC_tUserToken                token
   )
{
   DBUSIPC_tError status(DBUSIPC_ERROR_NONE);
   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);

   try
   {
      std::auto_ptr<SubscribeOwnerChangedCmd> cmd(new
            SubscribeOwnerChangedCmd(conn, busName, onOwnerChanged,
                                      onSubscription, token));

      status = DBUSIPC_submitCmd(cmd.release(), hnd);
   }
   catch (const DBUSIPCError& e)
   {
      status = e.getError();
   }
   catch (const std::exception&)
   {
      status = DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                                 DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_INTERNAL);
   }

   return status;
}


DBUSIPC_tError DBUSIPC_validateUtf8
   (
   DBUSIPC_tConstStr            str
   )
{
	//if ( (0 != str) && dbus_str_validate_utf8( str))
	//{
    //    return DBUSIPC_ERROR_NONE;
    //}

	return DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                             DBUSIPC_DOMAIN_DBUS_LIB, DBUSIPC_ERR_FORMAT);
}

