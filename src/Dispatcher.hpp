#ifndef DISPATCHER_HPP_
#define DISPATCHER_HPP_

#include <set>
#include <list>
#include <vector>
#include <deque>
#include <memory>
#include "NSysDep.hpp"
#include "Pipe.hpp"
#include "dbus/dbus.h"
#include "Thread.hpp"
#include "MutexLock.hpp"
#include "dbusipc/dbusipc.h"

//
// Forward Declarations
//
class BaseCommand;
class Watch;
class Timeout;
class PipeWatch;
class Connection;

class Dispatcher : public Thread
{
public:
	Dispatcher();
	~Dispatcher();

	void addPending(Connection* conn);
	void removePending(Connection* conn);
	void addWatch(Watch* watch);
	void removeWatch(Watch* watch);
	void addTimeout(Timeout* timeout);
	void removeTimeout(Timeout* timeout);
   DBUSIPC_tHandle submitCommand(BaseCommand* cmd);
   DBUSIPC_tError cancelCommand(DBUSIPC_tHandle hnd);

   //
   // Callbacks for handling D-Bus Watch and Timeouts
   //
   static dbus_bool_t onAddWatchCallback(DBusWatch* watch, void* data);
   static void onRemoveWatchCallBack(DBusWatch* watch, void* data);
   static void onToggleWatchCallback(DBusWatch* watch, void* data);
   static dbus_bool_t onAddTimeoutCallback(DBusTimeout* timeout, void* data);
   static void onRemoveTimeoutCallback(DBusTimeout* timeout, void* data);
   static void onToggleTimeoutCallback(DBusTimeout* timeout, void* data);

private:
   // Default amount of time to block in a call to poll()
   enum {
         DEFAULT_POLL_MSEC_WAIT = 3000,
         // Time to sleep (msec) on error from calling poll()
         DEFAULT_SLEEP_ON_POLL_ERROR = 10
        };

   bool execute();
   void dispatchPending();
   void dispatch();
   DBUSIPC_tHandle getNextHandle();
   static bool onCommand(uint32_t flags, void* data);

   // Convenient typedefs for containers
   typedef std::deque<BaseCommand*> tCmdContainer;
   typedef std::list<Connection*> tConnList;
   typedef std::set<Watch*> tWatchContainer;
   typedef std::set<Timeout*> tTimeoutContainer;

   tConnList                        mPendingConnList;
   DBUSIPC_tHandle                   mCmdHandleCounter;
   tCmdContainer                    mCmdQueue;
   MutexLock                        mCmdQLock;
   tWatchContainer                  mWatches;
   tTimeoutContainer                mTimeouts;
   Pipe                             mPipe;
   std::auto_ptr<PipeWatch>         mPipeWatch;
   NSysDep::DBUSIPC_tPollFdContainer mPollFds;
};

#endif /* Guard for DISPATCHER_HPP_ */
