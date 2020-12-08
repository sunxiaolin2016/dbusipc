#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include <string>
#include <memory>
#include "dbusipc/dbusipc.h"

//
// Forward Declarations
//
class Dispatcher;
class Semaphore;
class Connection;
class SignalSubscription;
class ServiceRegistration;
class DBUSIPCSubscription;
class NameOwnerChangedSubscription;
struct DBusPendingCall;
class RequestContext;

class BaseCommand
{
public:
	BaseCommand();
	virtual ~BaseCommand();
	
	void setHandle(DBUSIPC_tHandle hnd) { mHandle = hnd; }
	DBUSIPC_tHandle getHandle() const { return mHandle; }
	virtual void execute(Dispatcher& dispatcher) = 0;
	virtual void cancel(Dispatcher& dispatcher) {}
	virtual bool execAndDestroy() const { return true; }
	
private:
   // Private copy constructor and assignment operator to prevent misuse
   BaseCommand(const BaseCommand& rhs);
   BaseCommand& operator=(const BaseCommand& rhs);
   
   DBUSIPC_tHandle mHandle;
};

class OpenConnectionCmd : public BaseCommand
{
public:
   OpenConnectionCmd(DBUSIPC_tConstStr address,
                     DBUSIPC_tBool openPrivate,
                     DBUSIPC_tConnectionCallback onConnect,
                     DBUSIPC_tUserToken token);
   
   OpenConnectionCmd(DBUSIPC_tConstStr address,
                     DBUSIPC_tBool openPrivate,
                     Semaphore* sem,
                     DBUSIPC_tError* status,
                     DBUSIPC_tConnection* conn);
   ~OpenConnectionCmd();

   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   
private:
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   OpenConnectionCmd(const OpenConnectionCmd& other);
   OpenConnectionCmd& operator=(const OpenConnectionCmd& rhs);

   void dispatch(const DBUSIPC_tCallbackStatus& status,
                 DBUSIPC_tConnection conn);
   
   std::string                mAddress;
   DBUSIPC_tBool               mPrivConn;
   DBUSIPC_tConnectionCallback mOnConnect;
   DBUSIPC_tUserToken          mToken;
   Semaphore*                 mSem;
   DBUSIPC_tError*             mStatus;
   DBUSIPC_tConnection*        mConn;
};


class GetConnectionCmd : public BaseCommand
{
public:
   GetConnectionCmd(DBUSIPC_tConnType connType,
                  DBUSIPC_tBool openPrivate,
                  DBUSIPC_tConnectionCallback onConnect,
                  DBUSIPC_tUserToken token);
   
   GetConnectionCmd(DBUSIPC_tConnType connType,
                     DBUSIPC_tBool openPrivate,
                     Semaphore* sem,
                     DBUSIPC_tError* status,
                     DBUSIPC_tConnection* conn);
   
   ~GetConnectionCmd();

   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   
private:
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   GetConnectionCmd(const GetConnectionCmd& other);
   GetConnectionCmd& operator=(const GetConnectionCmd& rhs);
   
   void dispatch(const DBUSIPC_tCallbackStatus& status,
                    DBUSIPC_tConnection conn);
   
   DBUSIPC_tConnType           mConnType;
   DBUSIPC_tBool               mPrivConn;
   DBUSIPC_tConnectionCallback mOnConnect;
   DBUSIPC_tUserToken          mToken;
   Semaphore*                 mSem;
   DBUSIPC_tError*             mStatus;
   DBUSIPC_tConnection*        mConn;
};


class CloseConnectionCmd : public BaseCommand
{
public:
   CloseConnectionCmd(DBUSIPC_tConnection conn,
                     Semaphore* sem = 0,
                     DBUSIPC_tError* status = 0);
   ~CloseConnectionCmd();

   virtual void cancel(Dispatcher& dispatcher);
   virtual void execute(Dispatcher& dispatcher);
   
private:
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   CloseConnectionCmd(const CloseConnectionCmd& other);
   CloseConnectionCmd& operator=(const CloseConnectionCmd& rhs);
   
   void dispatch(DBUSIPC_tError error);
   
   DBUSIPC_tConnection   mConn;
   Semaphore*           mSem;
   DBUSIPC_tError*       mStatus;
};


class SubscribeCmd : public BaseCommand
{
public:
   SubscribeCmd(DBUSIPC_tConnection conn,
                DBUSIPC_tConstStr objPath,
                DBUSIPC_tConstStr sigName,
                DBUSIPC_tSignalCallback onSignal,
                DBUSIPC_tSubscriptionCallback onSubscription,
                DBUSIPC_tUserToken token);

   SubscribeCmd(DBUSIPC_tConnection conn,
                DBUSIPC_tConstStr objPath,
                DBUSIPC_tConstStr sigName,
                DBUSIPC_tSignalCallback onSignal,
                DBUSIPC_tUserToken token,
                Semaphore* sem,
                DBUSIPC_tError* status,
                DBUSIPC_tSigSubHnd* subHnd);
   
   ~SubscribeCmd();
   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   virtual bool execAndDestroy() const;
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   SubscribeCmd(const SubscribeCmd& other);
   SubscribeCmd& operator=(const SubscribeCmd& rhs);
 
   void dispatch(const DBUSIPC_tCallbackStatus& status,
                  DBUSIPC_tSigSubHnd subHnd);   
   static void onPendingCallNotify(DBusPendingCall* call, void* userData);
   
   Connection*                         mConn;
   std::string                         mObjectPath;
   std::string                         mSignalName;
   DBUSIPC_tSignalCallback              mOnSignal;
   DBUSIPC_tSubscriptionCallback        mOnSubscription;
   DBUSIPC_tUserToken                   mUserToken;
   Semaphore*                          mSem;
   DBUSIPC_tError*                      mStatus;
   DBUSIPC_tSigSubHnd*                  mSubHnd;
   DBusPendingCall*                    mPendingCall;
   bool                                mExecAndDestroy;
   std::auto_ptr<DBUSIPCSubscription>   mSigSub;
};


class UnsubscribeCmd : public BaseCommand
{
public:
   UnsubscribeCmd(SignalSubscription* sub,
                  DBUSIPC_tStatusCallback onStatus,
                  DBUSIPC_tUserToken token);
   UnsubscribeCmd(SignalSubscription* sub,
                  Semaphore* sem,
                  DBUSIPC_tError* status);
   
   ~UnsubscribeCmd();
   virtual void cancel(Dispatcher& dispatcher);
   virtual void execute(Dispatcher& dispatcher);
   virtual bool execAndDestroy() const;
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   UnsubscribeCmd(const UnsubscribeCmd& other);
   UnsubscribeCmd& operator=(const UnsubscribeCmd& rhs);

   void dispatch(const DBUSIPC_tCallbackStatus& status);
   static void onPendingCallNotify(DBusPendingCall* call, void* userData);
   
   SignalSubscription*     mSigSub;
   DBUSIPC_tStatusCallback  mOnStatus;
   DBUSIPC_tUserToken       mUserToken;
   Semaphore*              mSem;
   DBUSIPC_tError*          mStatus;
   DBusPendingCall*        mPendingCall;
   bool                    mExecAndDestroy;
};


class RegisterServiceCmd : public BaseCommand
{
public:
   RegisterServiceCmd(DBUSIPC_tConnection conn,
                     DBUSIPC_tConstStr busName,
                     DBUSIPC_tConstStr objPath,
                     DBUSIPC_tUInt32 flag,
                     DBUSIPC_tRequestCallback onRequest,
                     DBUSIPC_tRegistrationCallback onRegister,
                     DBUSIPC_tUserToken token);

   RegisterServiceCmd(DBUSIPC_tConnection conn,
                     DBUSIPC_tConstStr busName,
                     DBUSIPC_tConstStr objPath,
                     DBUSIPC_tUInt32 flag,
                     DBUSIPC_tRequestCallback onRequest,
                     DBUSIPC_tUserToken token,
                     Semaphore* sem,
                     DBUSIPC_tError* status,
                     DBUSIPC_tSvcRegHnd* regHnd);
   
   ~RegisterServiceCmd();
   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   virtual bool execAndDestroy() const;
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   RegisterServiceCmd(const RegisterServiceCmd& other);
   RegisterServiceCmd& operator=(const RegisterServiceCmd& rhs);

   void dispatch(const DBUSIPC_tCallbackStatus& status,
                  DBUSIPC_tSvcRegHnd regHnd);
   static void onPendingCallNotify(DBusPendingCall* call, void* userData);
   
   Connection*                         mConn;
   std::string                         mBusName;
   std::string                         mObjectPath;
   DBUSIPC_tUInt32                      mFlag;
   DBUSIPC_tRequestCallback             mOnRequest;
   DBUSIPC_tRegistrationCallback        mOnRegister;
   DBUSIPC_tUserToken                   mUserToken;
   Semaphore*                          mSem;
   DBUSIPC_tError*                      mStatus;
   DBUSIPC_tSvcRegHnd*                  mRegHnd;
   DBusPendingCall*                    mPendingCall;
   bool                                mExecAndDestroy;
   std::auto_ptr<ServiceRegistration>  mSvcReg;
};


class UnregisterServiceCmd : public BaseCommand
{
public:
   UnregisterServiceCmd(ServiceRegistration* reg,
                        DBUSIPC_tStatusCallback onStatus,
                        DBUSIPC_tUserToken token);
   
   UnregisterServiceCmd(ServiceRegistration* reg,
                        Semaphore* sem,
                        DBUSIPC_tError* status);
   
   ~UnregisterServiceCmd();
   virtual void cancel(Dispatcher& dispatcher);
   virtual void execute(Dispatcher& dispatcher);
   virtual bool execAndDestroy() const;
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   UnregisterServiceCmd(const UnregisterServiceCmd& other);
   UnregisterServiceCmd& operator=(const UnregisterServiceCmd& rhs);

   void dispatch(const DBUSIPC_tCallbackStatus& status);
   static void onPendingCallNotify(DBusPendingCall* call, void* userData);
   
   ServiceRegistration*          mSvcReg;
   DBUSIPC_tStatusCallback        mOnStatus;
   DBUSIPC_tUserToken             mUserToken;
   Semaphore*                    mSem;
   DBUSIPC_tError*                mStatus;
   DBusPendingCall*              mPendingCall;
   bool                          mExecAndDestroy;   
};


class InvokeCmd : public BaseCommand
{
public:
   // Constructor for the asynchronous command
   InvokeCmd(DBUSIPC_tConnection conn,
            DBUSIPC_tConstStr busName,
            DBUSIPC_tConstStr objPath,
            DBUSIPC_tConstStr method,
            DBUSIPC_tConstStr parameters,
            bool noReplyExpected,
            DBUSIPC_tUInt32 msecTimeout,
            DBUSIPC_tResultCallback onResult,
            DBUSIPC_tUserToken token);

   // Constructor for the synchronous command
   InvokeCmd(DBUSIPC_tConnection conn,
            DBUSIPC_tConstStr busName,
            DBUSIPC_tConstStr objPath,
            DBUSIPC_tConstStr method,
            DBUSIPC_tConstStr parameters,
            DBUSIPC_tUInt32 msecTimeout,
            DBUSIPC_tResponse** response,
            Semaphore* sem);
   
   ~InvokeCmd();
   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   virtual bool execAndDestroy() const;
   
   void dispatchResult(DBUSIPC_tError errCode, DBUSIPC_tConstStr errName,
                       DBUSIPC_tConstStr errMsg, DBUSIPC_tConstStr result);
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   InvokeCmd(const InvokeCmd& other);
   InvokeCmd& operator=(const InvokeCmd& rhs);
   
   static void onPendingCallNotify(DBusPendingCall* call, void* userData);
   
   Connection*                   mConn;
   std::string                   mBusName;
   std::string                   mObjectPath;
   std::string                   mMethod;
   std::string                   mParms;
   bool                          mNoReplyExpected;
   DBUSIPC_tUInt32                mMsecTimeout;
   DBUSIPC_tResultCallback        mOnResult;
   DBUSIPC_tUserToken             mUserToken;
   DBUSIPC_tResponse**            mResponse;
   //bool                          mAsync;
   Semaphore*                    mSem;
   DBusPendingCall*              mPendingCall;
   bool                          mExecAndDestroy;
   DBUSIPC_tUInt32                mSerialNum;
};


class EmitCmd : public BaseCommand
{
public:
   EmitCmd(DBUSIPC_tSvcRegHnd regHnd,
           DBUSIPC_tConstStr sigName,
           DBUSIPC_tConstStr parameters,
           DBUSIPC_tStatusCallback onStatus,
           DBUSIPC_tUserToken token);

   EmitCmd(DBUSIPC_tSvcRegHnd regHnd,
           DBUSIPC_tConstStr sigName,
           DBUSIPC_tConstStr parameters,
           Semaphore* sem,
           DBUSIPC_tError* status);
   
   ~EmitCmd();
   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   EmitCmd(const EmitCmd& other);
   EmitCmd& operator=(const EmitCmd& rhs);
   
   void dispatchStatus(DBUSIPC_tError errCode,
                       DBUSIPC_tConstStr errName,
                       DBUSIPC_tConstStr errMsg);
   
   ServiceRegistration*          mSvcReg;
   std::string                   mSignalName;
   std::string                   mParams;
   DBUSIPC_tStatusCallback        mOnStatus;
   DBUSIPC_tUserToken             mUserToken;
   Semaphore*                    mSem;
   DBUSIPC_tError*                mStatus;
};


class CancelCmd : public BaseCommand
{
public:
   CancelCmd(DBUSIPC_tHandle handle,
             Semaphore* sem = 0,
             DBUSIPC_tError* status = 0);
   ~CancelCmd();
   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   CancelCmd(const CancelCmd& other);
   CancelCmd& operator=(const CancelCmd& rhs);
   
   void dispatch(DBUSIPC_tError error);
   
   DBUSIPC_tHandle mHandleWillCancel;
   Semaphore*     mSem;
   DBUSIPC_tError* mStatus;
};


class ReturnResultCmd : public BaseCommand
{
public:
   ReturnResultCmd(DBUSIPC_tReqContext context,
                   DBUSIPC_tConstStr result,
                   DBUSIPC_tStatusCallback onStatus,
                   DBUSIPC_tUserToken token);
   
   ReturnResultCmd(DBUSIPC_tReqContext context,
                   DBUSIPC_tConstStr result,
                   Semaphore* sem,
                   DBUSIPC_tError* status);
   
   ~ReturnResultCmd();
   
   virtual void cancel(Dispatcher& dispatcher);
   virtual void execute(Dispatcher& dispatcher);
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   ReturnResultCmd(const ReturnResultCmd& other);
   ReturnResultCmd& operator=(const ReturnResultCmd& rhs);
   
   void dispatchStatus(DBUSIPC_tError errCode, DBUSIPC_tConstStr errName,
                       DBUSIPC_tConstStr errMsg);
   
   RequestContext*         mReqContext;
   std::string             mResult;
   DBUSIPC_tStatusCallback  mOnStatus;
   DBUSIPC_tUserToken       mUserToken;
   Semaphore*              mSem;
   DBUSIPC_tError*          mStatus;
};


class ReturnErrorCmd : public BaseCommand
{
public:
   ReturnErrorCmd(DBUSIPC_tReqContext context,
                  DBUSIPC_tConstStr name,
                  DBUSIPC_tConstStr msg,
                  DBUSIPC_tStatusCallback onStatus,
                  DBUSIPC_tUserToken token);
   
   ReturnErrorCmd(DBUSIPC_tReqContext context,
                  DBUSIPC_tConstStr name,
                  DBUSIPC_tConstStr msg,
                  Semaphore* sem,
                  DBUSIPC_tError* status);
   
   ~ReturnErrorCmd();
   
   virtual void cancel(Dispatcher& dispatcher);
   virtual void execute(Dispatcher& dispatcher);
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   ReturnErrorCmd(const ReturnErrorCmd& other);
   ReturnErrorCmd& operator=(const ReturnErrorCmd& rhs);

   void dispatchStatus(DBUSIPC_tError errCode, DBUSIPC_tConstStr errName,
                       DBUSIPC_tConstStr errMsg);
   
   RequestContext*         mReqContext;
   std::string             mErrName;
   std::string             mErrMsg;
   DBUSIPC_tStatusCallback  mOnStatus;
   DBUSIPC_tUserToken       mUserToken;
   Semaphore*              mSem;
   DBUSIPC_tError*          mStatus;
};


class FreeRequestContextCmd : public BaseCommand
{
public:
   FreeRequestContextCmd(DBUSIPC_tReqContext context);
   ~FreeRequestContextCmd();
   virtual void execute(Dispatcher& dispatcher);
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   FreeRequestContextCmd(const FreeRequestContextCmd& other);
   FreeRequestContextCmd& operator=(const FreeRequestContextCmd& rhs);
   
   RequestContext* mReqContext;   
};


class ShutdownCmd : public BaseCommand
{
public:
   ShutdownCmd(Semaphore* sem);
   ~ShutdownCmd();
   virtual void execute(Dispatcher& dispatcher);
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   ShutdownCmd(const ShutdownCmd& other);
   ShutdownCmd& operator=(const ShutdownCmd& rhs);
   
   Semaphore*  mSem;
};


class NameHasOwnerCmd : public BaseCommand
{
public:
   NameHasOwnerCmd(DBUSIPC_tConnection conn,
                   DBUSIPC_tConstStr busName,
                   DBUSIPC_tBool* hasOwner,
                   Semaphore* sem,
                   DBUSIPC_tError* status);
   
   NameHasOwnerCmd(DBUSIPC_tConnection conn,
                   DBUSIPC_tConstStr busName,
                   DBUSIPC_tNameHasOwnerCallback onHasOwner,
                   DBUSIPC_tUserToken token);
   
   ~NameHasOwnerCmd();
   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   virtual bool execAndDestroy() const;
   
   void dispatchResult(DBUSIPC_tError errCode, DBUSIPC_tConstStr errName,
                       DBUSIPC_tConstStr errMsg, DBUSIPC_tBool hasOwner);
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   NameHasOwnerCmd(const NameHasOwnerCmd& other);
   NameHasOwnerCmd& operator=(const NameHasOwnerCmd& rhs);
   
   static void onPendingCallNotify(DBusPendingCall* call, void* userData);
   
   Connection*                   mConn;
   std::string                   mBusName;
   DBUSIPC_tBool*                 mHasOwner;
   DBUSIPC_tNameHasOwnerCallback  mOnHasOwner;
   DBUSIPC_tUserToken             mUserToken;
   Semaphore*                    mSem;
   DBUSIPC_tError*                mStatus;
   DBusPendingCall*              mPendingCall;
   bool                          mExecAndDestroy;
};


class SubscribeOwnerChangedCmd : public BaseCommand
{
public:
   SubscribeOwnerChangedCmd(DBUSIPC_tConnection conn,
                         DBUSIPC_tConstStr busName,
                         DBUSIPC_tNameOwnerChangedCallback onOwnerChanged,
                         DBUSIPC_tSubscriptionCallback onSubscription,
                         DBUSIPC_tUserToken token);

   SubscribeOwnerChangedCmd(DBUSIPC_tConnection conn,
                         DBUSIPC_tConstStr busName,
                         DBUSIPC_tNameOwnerChangedCallback onOwnerChanged,
                         DBUSIPC_tUserToken token,
                         DBUSIPC_tSigSubHnd* subHnd,
                         Semaphore* sem,
                         DBUSIPC_tError* status);
   
   ~SubscribeOwnerChangedCmd();
   virtual void execute(Dispatcher& dispatcher);
   virtual void cancel(Dispatcher& dispatcher);
   virtual bool execAndDestroy() const;
   
private:
   
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   SubscribeOwnerChangedCmd(const SubscribeOwnerChangedCmd& other);
   SubscribeOwnerChangedCmd& operator=(const SubscribeOwnerChangedCmd& rhs);
 
   void dispatch(const DBUSIPC_tCallbackStatus& status,
                  DBUSIPC_tSigSubHnd subHnd);
   
   static void onPendingCallNotify(DBusPendingCall* call, void* userData);
   
   Connection*                                  mConn;
   std::string                                  mBusName;
   DBUSIPC_tNameOwnerChangedCallback             mOnOwnerChanged;
   DBUSIPC_tSubscriptionCallback                 mOnSubscription;
   DBUSIPC_tUserToken                            mUserToken;
   DBUSIPC_tSigSubHnd*                           mSubHnd;
   Semaphore*                                   mSem;
   DBUSIPC_tError*                               mStatus;
   DBusPendingCall*                             mPendingCall;
   bool                                         mExecAndDestroy;
   std::auto_ptr<NameOwnerChangedSubscription>  mSigSub;
};

#endif /*COMMAND_HPP_*/
