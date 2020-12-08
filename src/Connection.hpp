#ifndef CONNECTION_HPP_
#define CONNECTION_HPP_

#include <map>
#include <set>
#include "dbus/dbus.h"
#include "dbusipc/dbusipc.h"


//
// Forward Declarations
//
class Dispatcher;
class SignalSubscription;
class ServiceRegistration;
class BaseCommand;

class Connection
{
public:

   static Connection* create(DBUSIPC_tConstStr address,
                             DBUSIPC_tBool openPrivate,
                             Dispatcher* disp);
   
   static Connection* create(DBUSIPC_tConnType connType,
                             DBUSIPC_tBool openPrivate,
                             Dispatcher* disp);   
   static void release(Connection*);
   static void forceReleaseAll();
   static DBusConnection* getDBusConnection(Connection* conn);
   static bool connectionExists(Connection* conn);
   static DBUSIPC_tError cancelPendingByHandle(DBUSIPC_tHandle hnd);
   static bool signalSubExists(SignalSubscription* sigSub);
   static bool serviceRegExists(ServiceRegistration* svcReg);
	
   bool isReadyForDispatch();
   bool dispatchMessages();
   
   // This can throw exceptions
   void subscribeSignal(SignalSubscription* sigSub);
   void unsubscribeSignal(SignalSubscription* sigSub);
   
   void registerService(ServiceRegistration* reg);
   void unregisterService(ServiceRegistration* reg);
   
   // Tracks pending method requests
   void registerPending(BaseCommand* cmd);
   void unregisterPending(BaseCommand* cmd);
   
private:
   
   Connection(DBusConnection* conn, bool priv, Dispatcher* disp);
   ~Connection();
   void incRef();
   void decRef();

   DBusHandlerResult introspect(DBusMessage* msg);      
   static DBusHandlerResult messageFilter(DBusConnection* dbusConn,
                                    DBusMessage *msg, void* data);
   static void onDispatchStatusUpdate(DBusConnection* dbusConn,
                                      DBusDispatchStatus newStatus,
                                      void* data);
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   Connection(const Connection& other);
   Connection& operator=(const Connection& rhs);

   //
   // Member variables
   //
   
   typedef std::map<Connection*,DBusConnection*> tConnCache;
   typedef std::set<SignalSubscription*> tSigSubContainer;
   typedef std::set<ServiceRegistration*> tSvcRegContainer;
   typedef std::set<BaseCommand*> tPendingContainer;
   
	DBusConnection*           mDBusConn;
	bool                      mPrivate;
	uint32_t                  mRefCount;
	static tConnCache         msConnCache;
	Dispatcher*               mDispatcher;
	tSigSubContainer          mSigSubscriptions;
	tSvcRegContainer          mSvcRegistrations;
	tPendingContainer         mPendingCmds;
	uint64_t                  mMaxDispatchProcTime;
};


inline bool Connection::connectionExists
   (
   Connection* conn
   )
{
   return 0 != getDBusConnection(conn);
}

#endif /* Guard for CONNECTION_HPP_ */
