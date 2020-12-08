#ifndef EXCEPTIONS_HPP_
#define EXCEPTIONS_HPP_

#include <string.h>
#include <exception>

#include "dbusipc/dbusipc.h"
#include "InterfaceDefs.hpp"

class DBUSIPCError : public std::exception
{
public:
   DBUSIPCError(DBUSIPC_tError e, DBUSIPC_tConstStr why = "")
      : std::exception()
      , mErrCode(e)
   {
      strncpy(mWhy, why ? why : "", sizeof(mWhy)/sizeof(mWhy[0]) - 1);
      // Make sure it's NULL terminated
      mWhy[sizeof(mWhy)/sizeof(mWhy[0]) - 1] = '\0';
   }

   const char *what() const throw()
   {
      return mWhy;
   }
   
   DBUSIPC_tError getError() const { return mErrCode; }

protected:
   enum { DBUSIPC_ERROR_MAX_BUF_SIZE = 128 };
   
   DBUSIPC_tChar   mWhy[DBUSIPC_ERROR_MAX_BUF_SIZE];
   DBUSIPC_tError  mErrCode;
};

#if OS_QNX
class PosixError : public DBUSIPCError
{
public:
   PosixError(int errorCode = 0, DBUSIPC_tConstStr why = "Posix error")
      : DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                    DBUSIPC_DOMAIN_C_LIB, errorCode), why)
      {}
};

class LockError : public PosixError
{
public:
   LockError(int errorCode = 0)
      : PosixError(errorCode, "Failed to acquire lock")
      {}
};

class UnlockError : public PosixError
{
public:
   UnlockError(int errorCode = 0)
      : PosixError(errorCode, "Failed to release lock")
      {}
};

#elif OS_LINUX

class PosixError : public DBUSIPCError
{
public:
   PosixError(int errorCode = 0, DBUSIPC_tConstStr why = "Posix error")
      : DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                    DBUSIPC_DOMAIN_C_LIB, errorCode), why)
      {}
};

class LockError : public PosixError
{
public:
   LockError(int errorCode = 0)
      : PosixError(errorCode, "Failed to acquire lock")
      {}
};

class UnlockError : public PosixError
{
public:
   UnlockError(int errorCode = 0)
      : PosixError(errorCode, "Failed to release lock")
      {}
};

#elif WIN32

class Win32Error : public DBUSIPCError
{
public:
   Win32Error(int errorCode = 0, DBUSIPC_tConstStr why = "Win32 error")
      : DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                    DBUSIPC_DOMAIN_WIN32_LIB, errorCode), why)
      {}
};

class LockError : public Win32Error
{
public:
   LockError(int errorCode = 0)
      : Win32Error(errorCode, "Failed to acquire lock")
      {}
};

class UnlockError : public Win32Error
{
public:
   UnlockError(int errorCode = 0)
      : Win32Error(errorCode, "Failed to release lock")
      {}
};

#endif

class DBusException : public DBUSIPCError
{
public:
   DBusException(DBUSIPC_tConstStr errName = DBUSIPC_INTERFACE_ERROR_NAME,
               DBUSIPC_tConstStr errMsg = "DBus error")
      : DBUSIPCError(DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_ERROR,
                     DBUSIPC_DOMAIN_DBUS_LIB, DBUSIPC_ERR_DBUS), 0)
      {
         const size_t bufLen = sizeof(mWhy) / sizeof(mWhy[0]);
         size_t bufAvail = bufLen;
         if ( 0 != errName )
         {
            strncat(mWhy, errName, bufAvail);
            bufAvail = bufLen - strlen(mWhy);
            if (( 0 != errMsg ) && (bufAvail >0))
            {
               strncat(mWhy, ": ", bufAvail);
               bufAvail = bufLen - strlen(mWhy);
            }
         }
         
         if (( 0 != errMsg ) && (bufAvail > 0))
         {
            strncat(mWhy, errMsg, bufAvail);
         }
         
         // Make sure it's NULL terminated
         mWhy[bufLen-1] = '\0';
      }
};

#endif /*EXCEPTIONS_HPP_*/
