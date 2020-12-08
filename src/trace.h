#ifndef TRACE_H_
#define TRACE_H_


#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdarg.h>
#include "dbusipc/dbusipc_types.h"
   

   
#ifdef DBUSIPC_DEBUG

void DBUSIPC_trace(FILE* fp, DBUSIPC_tConstStr name,
                  DBUSIPC_tConstStr fmt, va_list* list);
   
void DBUSIPC_traceInfo(DBUSIPC_tConstStr fmt, ...);
void DBUSIPC_traceWarn(DBUSIPC_tConstStr fmt, ...);
void DBUSIPC_traceError(DBUSIPC_tConstStr fmt, ...);

/*
 * These are the publically available macros
 */
#define TRACE_INFO   DBUSIPC_traceInfo
#define TRACE_WARN   DBUSIPC_traceWarn
#define TRACE_ERROR  DBUSIPC_traceError

#else /* Else no trace */

/*
 * NOTE: This is using a non-standard GCC varadic macro extension and may
 * not work with other compilers.
 */
#define TRACE_INFO(...)   (void(0))
#define TRACE_WARN(...)   (void(0))
#define TRACE_ERROR(...)  (void(0))
#endif

#ifdef __cplusplus
}
#endif

#endif /* Guard for TRACE_H_ */
