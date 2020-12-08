#ifndef _DBUSIPC_TYPES_H_
#define _DBUSIPC_TYPES_H_


#ifdef __cplusplus
extern "C" {
#endif


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

// Basic integer types
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef unsigned long      uint32_t;
typedef long               int32_t;
typedef __int64            int64_t;
typedef unsigned __int64   uint64_t;

#else
#include <stdint.h>
#endif


/**
 * @brief Fundamental type definitions
 */
typedef char                        DBUSIPC_tChar;
typedef DBUSIPC_tChar*              DBUSIPC_tString;
typedef const DBUSIPC_tChar*        DBUSIPC_tConstStr;
typedef uint32_t                    DBUSIPC_tUInt32;
typedef int32_t                     DBUSIPC_tInt32;
typedef int32_t                     DBUSIPC_tBool;
typedef void*                       DBUSIPC_tUserToken;
typedef void*                       DBUSIPC_tConnection;
typedef void*                       DBUSIPC_tReqContext;
typedef void*                       DBUSIPC_tSvcRegHnd;
typedef void*                       DBUSIPC_tSigSubHnd;
typedef uint32_t                    DBUSIPC_tError;
typedef uint32_t                    DBUSIPC_tHandle;

/**
 * @brief Define boolean values for library
 */
#define DBUSIPC_TRUE     (1)
#define DBUSIPC_FALSE    (!DBUSIPC_TRUE)

/*
 * @brief Define the maximum value for an uint64 number
 */
#ifndef DBUSIPC_MAX_UINT64
#define DBUSIPC_MAX_UINT64      (0xffffffffffffffffULL)
#endif

/**
 * @brief Callback status returned with all callback
 */
typedef struct DBUSIPC_tCallbackStatus
{
   DBUSIPC_tError     errCode;
   DBUSIPC_tConstStr  errName; /* The name of the error (can be NULL */
   DBUSIPC_tConstStr  errMsg;  /* An error message (can be NULL) */
} DBUSIPC_tCallbackStatus;


/*
 * @brief Define the reply returned from a call to block and wait
 *        for the response from a method invocation.
 */
typedef struct DBUSIPC_tResponse
{
   // This structure is identical to DBUSIPC_tCallbackStatus but must
   // use non-const pointers so that the resources can be freed.
   struct DBUSIPC_tResponseStatus
   {
      DBUSIPC_tError   errCode;
      DBUSIPC_tString  errName; /* The name of the error (can be NULL */
      DBUSIPC_tString  errMsg;  /* An error message (can be NULL) */
   } status;
   DBUSIPC_tString     result;
} DBUSIPC_tResponse;

/**
 * @brief Define the basic callback types
 */

/* @brief Called to provide the status of an operation */
typedef void (*DBUSIPC_tStatusCallback)(const DBUSIPC_tCallbackStatus* status,
                                       DBUSIPC_tUserToken token);

/* @brief Called to return a newly allocated connection */
typedef void (*DBUSIPC_tConnectionCallback)(const DBUSIPC_tCallbackStatus* status,
                                      DBUSIPC_tConnection conn,
                                      DBUSIPC_tUserToken token);

/* @brief Called to return the result from a service */
typedef void (*DBUSIPC_tResultCallback)(const DBUSIPC_tCallbackStatus* status,
                                   DBUSIPC_tConstStr result,
                                   DBUSIPC_tUserToken token);

/* @brief Called to deliver a request from a client */
typedef void (*DBUSIPC_tRequestCallback)(DBUSIPC_tReqContext context,
                                     DBUSIPC_tConstStr method,
                                     DBUSIPC_tConstStr parms,
                                     DBUSIPC_tBool noReplyExpected,
                                     DBUSIPC_tUserToken token);

/* @brief Called to deliver a signal from a service */
typedef void (*DBUSIPC_tSignalCallback)(DBUSIPC_tConstStr sigName,
                                     DBUSIPC_tConstStr data,
                                     DBUSIPC_tUserToken token);

/* @brief Called to deliver a name owner changed signal */
typedef void (*DBUSIPC_tNameOwnerChangedCallback)(DBUSIPC_tConstStr newName,
                                          DBUSIPC_tConstStr oldOwner,
                                          DBUSIPC_tConstStr newOwner,
                                          DBUSIPC_tUserToken token);

/* @brief Called to deliver an indication of whether a bus name is owned */
typedef void (*DBUSIPC_tNameHasOwnerCallback)(
                                    const DBUSIPC_tCallbackStatus* status,
                                    DBUSIPC_tConstStr busName,
                                    DBUSIPC_tBool hasOwner,
                                    DBUSIPC_tUserToken token);

/* @brief Called to return a newly allocated service registration */
typedef void (*DBUSIPC_tRegistrationCallback)(
                                    const DBUSIPC_tCallbackStatus* status,
                                    DBUSIPC_tSvcRegHnd regHnd,
                                    DBUSIPC_tUserToken token);

/* @brief Called to return a newly allocated signal subscription */
typedef void (*DBUSIPC_tSubscriptionCallback)(
                                    const DBUSIPC_tCallbackStatus* status,
                                    DBUSIPC_tSigSubHnd subHnd,
                                    DBUSIPC_tUserToken token);

/**
 * @brief Define the well-known message buses
 */
typedef enum
{
   DBUSIPC_CONNECTION_SESSION,    /* Per-user login message bus */
   DBUSIPC_CONNECTION_SYSTEM,     /* System-wide message bus */
   DBUSIPC_CONNECTION_STARTER     /* Message bus that started/launched service */
} DBUSIPC_tConnType;

/**
 * @brief Define an invalid handle value
 */
#define DBUSIPC_INVALID_HANDLE (0U)

#ifdef __cplusplus
}
#endif

#endif /* Guard for DBUSIPC_TYPES_H_ */
