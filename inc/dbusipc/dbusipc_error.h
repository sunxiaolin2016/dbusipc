#ifndef _DBUSIPC_ERROR_H_
#define _DBUSIPC_ERROR_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "dbusipc/dbusipc_export.h"
#include "dbusipc/dbusipc_types.h"

/*
 * @brief   Define the logical domains within the service IPC library
 */
typedef enum {
   DBUSIPC_DOMAIN_IPC_LIB       = 0,
   DBUSIPC_DOMAIN_DBUS_LIB      = 1,
   DBUSIPC_DOMAIN_C_LIB         = 2,
   DBUSIPC_DOMAIN_WIN32_LIB     = 3
} DBUSIPC_tDomain;

/*
 * @brief   Define the available error levels
 */
typedef enum {
   DBUSIPC_ERROR_LEVEL_NONE  = 0,
   DBUSIPC_ERROR_LEVEL_WARN  = 1,
   DBUSIPC_ERROR_LEVEL_ERROR = 2,
   DBUSIPC_ERROR_LEVEL_FATAL = 4
} DBUSIPC_tErrorLevel;

/*
 * @brief   Define the error type for the library. It's composed of a level,
 *          domain, and code.
 */

#define DBUSIPC_MAKE_ERROR(level, domain, code) \
         ( (((DBUSIPC_tUInt32)(level) & 0x00000003U) << 30U) | \
           (((DBUSIPC_tUInt32)(domain) & 0x00000003U) << 28U) | \
           (((DBUSIPC_tUInt32)(code) & 0x0FFFFFFFU)) )

#define DBUSIPC_ERR_GET_LEVEL(error)   \
         (((DBUSIPC_tUInt32)(error) >> 30U) & 0x00000003U)
#define DBUSIPC_ERR_GET_DOMAIN(error)  \
         (((DBUSIPC_tUInt32)(error) >> 28U) & 0x00000003U)
#define DBUSIPC_ERR_GET_CODE(error)    \
         ((DBUSIPC_tUInt32)(error) & 0x0FFFFFFFU)

#define DBUSIPC_IS_ERROR(error) (0 != (DBUSIPC_ERR_GET_LEVEL(error) & \
                           ((DBUSIPC_tUInt32)DBUSIPC_ERROR_LEVEL_ERROR | \
                           (DBUSIPC_tUInt32)DBUSIPC_ERROR_LEVEL_FATAL)))

/*
 * Define errors associated with the service IPC library
 */
typedef enum {
  DBUSIPC_ERR_OK,
  DBUSIPC_ERR_NOT_SUPPORTED,
  DBUSIPC_ERR_NO_MEMORY,
  DBUSIPC_ERR_BAD_ARGS,
  DBUSIPC_ERR_INTERNAL,
  DBUSIPC_ERR_DBUS,
  DBUSIPC_ERR_CMD_SUBMISSION,
  DBUSIPC_ERR_NOT_CONNECTED,
  DBUSIPC_ERR_CANCELLED,
  DBUSIPC_ERR_CONN_SEND,
  DBUSIPC_ERR_NOT_FOUND,
  DBUSIPC_ERR_DEADLOCK,
  DBUSIPC_ERR_FORMAT
} DBUSIPC_tErrorCode;

extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_OK;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_NOT_SUPPORTED;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_NO_MEMORY;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_BAD_ARGS;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_INTERNAL;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_DBUS;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_CMD_SUBMISSION;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_NOT_CONNECTED;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_CANCELLED;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_CONN_SEND;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_NOT_FOUND;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_DEADLOCK;
extern DBUSIPC_API DBUSIPC_tConstStr DBUSIPC_ERR_NAME_FORMAT;

/*
 * Convenience defintion for no errors
 */
#define DBUSIPC_ERROR_NONE  (DBUSIPC_MAKE_ERROR(DBUSIPC_ERROR_LEVEL_NONE, \
                        DBUSIPC_DOMAIN_IPC_LIB, DBUSIPC_ERR_OK))

#ifdef __cplusplus
}
#endif

#endif /* Guard for _DBUSIPC_ERROR_H_ */
