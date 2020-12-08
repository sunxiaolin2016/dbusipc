#ifndef SIGNALSUBSCRIPTION_HPP_
#define SIGNALSUBSCRIPTION_HPP_

#include <string>
#include <sstream>
#include "dbusipc/dbusipc.h"

//
// Forward Declarations
//
class Connection;
struct DBusMessage;

class SignalSubscription
{
public:
   SignalSubscription(Connection* conn);
   
   virtual ~SignalSubscription();

   Connection* getConnection() const;
   virtual const char* getRule() const = 0;

   virtual bool dispatchIfMatch(DBusMessage* msg,
                uint64_t timeout = DBUSIPC_MAX_UINT64) = 0;
   
private:
   Connection* mConn;
};


inline Connection* SignalSubscription::getConnection() const
{
   return mConn;
}


class NameOwnerChangedSubscription : public SignalSubscription
{
public:
   NameOwnerChangedSubscription(Connection* conn,
                                const std::string& busName,
                                DBUSIPC_tNameOwnerChangedCallback onNameChange,
                                DBUSIPC_tUserToken token);
   virtual ~NameOwnerChangedSubscription();
   
   virtual const char* getRule() const;
   
   virtual bool dispatchIfMatch(DBusMessage* msg,
                     uint64_t timeout = DBUSIPC_MAX_UINT64);

private:
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   NameOwnerChangedSubscription(const NameOwnerChangedSubscription& other);
   NameOwnerChangedSubscription& operator=
                                 (const NameOwnerChangedSubscription& rhs);
   
   std::string                      mRule;
   std::string                      mBusName;
   DBUSIPC_tNameOwnerChangedCallback mOnNameChange;
   DBUSIPC_tUserToken                mUserToken;
};


class DBUSIPCSubscription : public SignalSubscription
{
public:
	DBUSIPCSubscription(Connection* conn,
	                  const std::string& objPath,
	                  const std::string& sigName,
                     DBUSIPC_tSignalCallback onSignal,
                     DBUSIPC_tUserToken token);
	virtual ~DBUSIPCSubscription();

   virtual const char* getRule() const;

   virtual bool dispatchIfMatch(DBusMessage* msg,
                  uint64_t timeout = DBUSIPC_MAX_UINT64);
	
	
private:
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   DBUSIPCSubscription(const DBUSIPCSubscription& other);
   DBUSIPCSubscription& operator=(const DBUSIPCSubscription& rhs);
   
   std::string             mRule;
   std::string             mObjectPath;
   std::string             mSignalName;
   DBUSIPC_tSignalCallback  mOnSignal;
   DBUSIPC_tUserToken       mUserToken;
};


#endif /* Guard for SIGNALSUBSCRIPTION_HPP_*/
