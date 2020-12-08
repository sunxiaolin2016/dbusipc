
#include <cerrno>
#include <algorithm>
#include <assert.h>
#include "Exceptions.hpp"
#include "Dispatcher.hpp"
#include "Command.hpp"
#include "ScopedLock.hpp"
#include "DBusTimeoutWrapper.hpp"
#include "DBusWatchWrapper.hpp"
#include "PipeWatch.hpp"
#include "Connection.hpp"
#include "trace.h"

Dispatcher::Dispatcher()
   : Thread()
   , mPendingConnList()
   , mCmdHandleCounter(DBUSIPC_INVALID_HANDLE)
   , mCmdQueue()
   , mCmdQLock()
   , mPipe()
   , mPipeWatch(0)
{
   if ( !mPipe.open(true, false) )
   {
#ifdef __QNX__
      throw PosixError(errno);
#elif defined(WIN32)
      throw Win32Error(WSAGetLastError());
#endif
   }

   mPipeWatch.reset(new PipeWatch(mPipe.getReadFd(),
         DBUS_WATCH_READABLE | DBUS_WATCH_HANGUP | DBUS_WATCH_ERROR, true,
         Dispatcher::onCommand, this, this));

}

Dispatcher::~Dispatcher()
{
   // Stop and wait for the dispatching thread to exit
   stop();
   wait(INFINITE_WAIT);

   // Cancel and delete any remaining commands still queued
   ScopedLock lock(mCmdQLock);
   while ( !mCmdQueue.empty() )
   {
      BaseCommand* cmd = mCmdQueue.front();
      mCmdQueue.pop_front();
      assert( 0 != cmd );
      cmd->cancel(*this);
      delete cmd;
   }
}

bool Dispatcher::onCommand
   (
   uint32_t flags,
   void*    data
   )
{
   DBUSIPC_tChar temp;
   Dispatcher* disp = static_cast<Dispatcher*>(data);
   if ( 0 == disp )
   {
      TRACE_ERROR("onCommand: Dispatcher is NULL!");
   }
   else
   {
      BaseCommand* cmd(0);
      // Lock the command queue while we process commands
      ScopedLock lock(disp->mCmdQLock);

      // Process commands while the queue is not empty and
      // the dispatcher is running
      while ( !disp->mCmdQueue.empty() && disp->isRunning() )
      {
         // Drain the pipe - one byte for each command read
         disp->mPipe.read(&temp, 1);

         // This will delete any previous command that the auto-ptr might
         // already reference.
         cmd = disp->mCmdQueue.front();

         disp->mCmdQueue.pop_front();
         if ( 0 != cmd )
         {
            // Execute the command
            cmd->execute(*disp);

            // See if the command should be destroyed after executing or
            // it's made arrangements to save itself somewhere
            // else while it was executing.
            if ( cmd->execAndDestroy() )
            {
               delete cmd;
            }
         }
      }
   }

   // We always say we've handled the command
   return true;
}


bool Dispatcher::execute()
{
   try
   {
      dispatchPending();
      dispatch();
   }
   catch ( const std::exception& e)
   {
      TRACE_ERROR("Dispatcher::execute - caught exception: %s", e.what());
   }

   return true;
}

void Dispatcher::dispatchPending()
{
   tConnList::iterator it;
   tConnList::iterator doneIt;

   it = mPendingConnList.begin();
   while ( it != mPendingConnList.end() )
   {
      // If dispatching messages on this connection is NOT complete then ...
      if ( !(*it)->dispatchMessages() )
      {
         // Advance to the next one
         ++it;
      }
      else  // Else this connection has dispatched its messages
      {
         // Remember the connection we want to remove
         doneIt = it;

         // Move to the next one in the list
         ++it;

         // Remove the completed connection
         mPendingConnList.erase(doneIt);
      }
   }
}


void Dispatcher::dispatch()
{
   NSysDep::DBUSIPC_tPollFd fds;
   tWatchContainer::iterator wIt;

   mPollFds.resize(0);
   if ( mWatches.size() > mPollFds.capacity() )
   {
      mPollFds.reserve(mWatches.size());
   }

   for ( wIt = mWatches.begin(); wIt != mWatches.end(); ++wIt )
   {
      if ( (*wIt)->enabled() )
      {
         fds.fd = (*wIt)->descriptor();
         fds.revents = 0;
         fds.events = static_cast<int16_t>((*wIt)->flags());

         mPollFds.push_back(fds);
      }
   }

   int32_t minWait = DEFAULT_POLL_MSEC_WAIT;
   uint64_t remaining = DEFAULT_POLL_MSEC_WAIT;
   uint64_t now = NSysDep::DBUSIPC_getSystemTime();

   tTimeoutContainer::iterator tIt;

   for ( tIt = mTimeouts.begin(); tIt != mTimeouts.end(); ++tIt )
   {
      // If this timeout is still enabled then ...
      if ( (*tIt)->enabled() )
      {
         // If this timeout has already expired then ...
         if ( now >= (*tIt)->expiry() )
         {
            // We don't want to wait in the call to poll since a timeout
            // is already pending.
            minWait = 0;
            break;
         }
         else
         {
            // Calculate the time remaining before this timeout expires
            remaining = (*tIt)->expiry() - now;
            if ( (int32_t)remaining < minWait )
            {
               // This is the new minimum amount of time to block in the
               // call to poll.
               minWait = (int32_t)remaining;
            }
         }
      }
   }

   int32_t nSelected = NSysDep::DBUSIPC_poll(mPollFds, minWait);

   // If there was an error polling the file descriptors then
   if ( 0 > nSelected )
   {
      // If we haven't waited at least a minimum amount of time then yield to
      // other threads in the system so we don't (potentially) consume all
      // available CPU cycles.
      int32_t elapsed = static_cast<int32_t>(NSysDep::DBUSIPC_getSystemTime() - now);
      if ( minWait > elapsed )
      {
         NSysDep::DBUSIPC_sleep(std::min<int32_t>(minWait - elapsed, DEFAULT_SLEEP_ON_POLL_ERROR));
      }
   }
   
   now = NSysDep::DBUSIPC_getSystemTime();

   typedef std::list<Timeout*> tExpiredTimerContainer;
   tExpiredTimerContainer expiredTimers;
   
   // We *cannot* "handle" the timers as we iterate over the timer
   // container because in the process of handling them they may
   // be destroyed and removed from the container which
   // breaks the iteration process. First we collect just the
   // pointers to the expired timers and then we loop through
   // this (new) list and handle them separately.
   for ( tIt = mTimeouts.begin(); tIt != mTimeouts.end(); ++tIt )
   {
      // Collected a list of timers that are enabled and expired so
      // that we can process them later
      if ( (*tIt)->enabled() && (now >= (*tIt)->expiry()) )
      {
         expiredTimers.push_back(*tIt);
      }
   }

   // Now we loop through all the expired timers and handle them
   // separately. As part of being "handled" the timers themselves
   // may be removed from the 'mTimeouts' container.
   //tExpiredTimerContainer::iterator expTmrIt = expiredTimers.begin();
   for ( tExpiredTimerContainer::iterator expTmrIt = expiredTimers.begin();
      expTmrIt != expiredTimers.end(); ++expTmrIt )
   {
      // If this timer is periodic then we must reset the
      // expiration timer BEFORE handling it in case it's
      // deleted in the handler. At that point (after the
      // call to the handler) the pointer would no longer
      // be valid.
      if ( (*expTmrIt)->repeat() )
      {
         (*expTmrIt)->resetExpiry();
      }
      
      // Do what needs to be done to handle this timer
      (*expTmrIt)->handle();
   }
   
   // If any of the descriptors was selected then ...
   if ( 0 < nSelected )
   {
      // Loop through the descriptors and remember the watches that are active
      // and need to be handled
      NSysDep::DBUSIPC_tPollFdContainer::iterator pollIt;
      typedef std::list<std::pair<Watch*, NSysDep::DBUSIPC_tPollFd*> >
                                                tActiveWatchContainer;
      tActiveWatchContainer activeWatches;
      for ( pollIt = mPollFds.begin(); pollIt != mPollFds.end(); ++pollIt )
      {
         // Search for the watch with the matching descriptor
         for ( wIt = mWatches.begin(); wIt != mWatches.end(); ++wIt )
         {
            // If the descriptors match then ...
            if ( (*wIt)->descriptor() == (*pollIt).fd )
            {
               // If the watch is enabled and there is activity on
               // descriptor then ...
               if ( (*wIt)->enabled() && (*pollIt).revents )
               {
                  // We need to remember and process this watch later
                  activeWatches.push_back(std::make_pair(*wIt, &(*pollIt)));
               }
            }
         }
      }

      // Now loop through the "active" watches and handle the activity
      // appropriately. It's entirely possible that while handling the
      // activity one of the watches could be removed from the watch list.
      // As a result, before actually handling a watch we need to make sure
      // it still exists in the master list of watches. It may no longer
      // be present.
      for ( tActiveWatchContainer::iterator actIt = activeWatches.begin();
            actIt != activeWatches.end(); ++actIt )
      {
         // If the watch still exists in the master list then ...
         if ( mWatches.find((*actIt).first) != mWatches.end() )
         {
            // Invoke the handler for this watch passing in the events. This
            // call *could* result in one of the watches (in the active list)
            // being deleted.
            (*actIt).first->handle((*actIt).second->revents);
         }
      }
   }
}


void Dispatcher::addPending
   (
   Connection*   conn
   )
{
   mPendingConnList.push_back(conn);
}


void Dispatcher::removePending
   (
   Connection* conn
   )
{
   mPendingConnList.remove(conn);
}


void Dispatcher::addWatch
   (
   Watch*   watch
   )
{
   assert( 0 != watch );
   mWatches.insert(watch);

   return;
}


void Dispatcher::removeWatch
   (
   Watch*   watch
   )
{
   assert( 0 != watch );
   mWatches.erase(watch);
}


void Dispatcher::addTimeout
   (
   Timeout* timeout
   )
{
   assert( 0 != timeout );
   mTimeouts.insert(timeout);

   return;
}


void Dispatcher::removeTimeout
   (
   Timeout* timeout
   )
{
   assert( 0 != timeout );
   mTimeouts.erase(timeout);
}


dbus_bool_t Dispatcher::onAddWatchCallback
   (
   DBusWatch*  watch,
   void*       data
   )
{
   dbus_bool_t isAdded = FALSE;

   Dispatcher* disp = static_cast<Dispatcher*>(data);
   if ( 0 == disp )
   {
      TRACE_ERROR("onAddWatchCallback: Dispatcher is NULL!");
   }
   else
   {
      try
      {
         std::auto_ptr<DBusWatchWrapper> w(new DBusWatchWrapper(watch, disp));
         w.release();
         isAdded = TRUE;
      }
      catch ( ... )
      {
         TRACE_ERROR("onAddWatchCallback: failed to create watch!");
      }
   }
   return isAdded;
}


void Dispatcher::onRemoveWatchCallBack
   (
   DBusWatch*  watch,
   void*       data
   )
{
   Dispatcher* disp = static_cast<Dispatcher*>(data);
   if ( 0 == disp )
   {
      TRACE_ERROR("onRemoveWatchCallBack: Dispatcher is NULL!");
   }
   else
   {
      DBusWatchWrapper* w = static_cast<DBusWatchWrapper*>
                           (dbus_watch_get_data(watch));
      if ( 0 == w )
      {
         TRACE_ERROR("onRemoveWatchCallBack: DBusWatchWrapper is NULL!");
      }
      else
      {
         // When the watch is deleted it will removes itself from the
         // list of watches.
         delete w;
      }
   }
}


void Dispatcher::onToggleWatchCallback
   (
   DBusWatch*  watch,
   void*       data
   )
{
   Dispatcher* disp = static_cast<Dispatcher*>(data);
   if ( 0 == disp )
   {
      TRACE_ERROR("onToggleWatchCallback: Dispatcher is NULL!");
   }
   else
   {
      DBusWatchWrapper* w = static_cast<DBusWatchWrapper*>
                           (dbus_watch_get_data(watch));
      if ( 0 != w )
      {
         w->toggle();
      }
      else
      {
         TRACE_ERROR("onToggleWatchCallback: DBusWatchWrapper is NULL!");
      }
   }
}


dbus_bool_t Dispatcher::onAddTimeoutCallback
   (
   DBusTimeout*   timeout,
   void*          data
   )
{
   dbus_bool_t isAdded = FALSE;

   Dispatcher* disp = static_cast<Dispatcher*>(data);
   if ( 0 == disp )
   {
      TRACE_ERROR("onAddTimeoutCallback: Dispatcher is NULL!");
   }
   else
   {
      try
      {
         std::auto_ptr<DBusTimeoutWrapper> t(new DBusTimeoutWrapper(timeout,
                                                                     disp));
         t.release();
         isAdded = TRUE;
      }
      catch ( ... )
      {
         TRACE_ERROR("onAddTimeoutCallback: failed to create timeout!");
      }
   }
   return isAdded;
}


void Dispatcher::onRemoveTimeoutCallback
   (
   DBusTimeout*   timeout,
   void*          data
   )
{
   Dispatcher* disp = static_cast<Dispatcher*>(data);
   if ( 0 == disp )
   {
      TRACE_ERROR("onRemoveTimeoutCallback: Dispatcher is NULL!");
   }
   else
   {
      DBusTimeoutWrapper* t = static_cast<DBusTimeoutWrapper*>
                           (dbus_timeout_get_data(timeout));
      if ( 0 == t )
      {
         TRACE_ERROR("onRemoveTimeoutCallback: DBusTimeoutWrapper is NULL!");
      }
      else
      {
         // When the timeout is deleted it will remove itself from the
         // list of timeouts
         delete t;
      }
   }
}


void Dispatcher::onToggleTimeoutCallback
   (
   DBusTimeout*   timeout,
   void*          data
   )
{
   Dispatcher* disp = static_cast<Dispatcher*>(data);
   if ( 0 == disp )
   {
      TRACE_ERROR("onToggleTimeoutCallback: Dispatcher is NULL!");
   }
   else
   {
      DBusTimeoutWrapper* t = static_cast<DBusTimeoutWrapper*>
                           (dbus_timeout_get_data(timeout));
      if ( 0 != t )
      {
         t->toggle();
      }
      else
      {
         TRACE_ERROR("onToggleTimeoutCallback: DBusTimeoutWrapper is NULL!");
      }
   }
}


DBUSIPC_tHandle Dispatcher::getNextHandle()
{
   mCmdHandleCounter++;
   if ( DBUSIPC_INVALID_HANDLE == mCmdHandleCounter )
   {
      mCmdHandleCounter++;
   }

   return mCmdHandleCounter;
}


DBUSIPC_tHandle Dispatcher::submitCommand
   (
   BaseCommand*   cmd
   )
{
   ScopedLock lock(mCmdQLock);
   DBUSIPC_tChar dummy(0);

   DBUSIPC_tHandle hnd(DBUSIPC_INVALID_HANDLE);
   // Submit a command if we have one and the dispatcher is running
   if ( (0 != cmd) && isRunning() )
   {
      hnd = getNextHandle();
      mCmdQueue.push_back(cmd);

      int32_t nBytes = mPipe.write(&dummy, 1);
      if ( -1 == nBytes )
      {
         TRACE_WARN("submitCommand: Failed to submit command!");
         hnd = DBUSIPC_INVALID_HANDLE;
         mCmdQueue.pop_back();
      }

      // Update the command handle
      cmd->setHandle(hnd);
   }
   return hnd;
}


DBUSIPC_tError Dispatcher::cancelCommand
   (
   DBUSIPC_tHandle hnd
   )
{
   return Connection::cancelPendingByHandle(hnd);
}

