#ifndef NOSTYPES_HPP_
#define NOSTYPES_HPP_


#if WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <winsock2.h>
#elif OS_QNX
#include <pthread.h>
#include <semaphore.h>
#include <process.h>
#elif OS_LINUX
#include <pthread.h>
#include <semaphore.h>
#endif

namespace NOsTypes {

#if WIN32

typedef HANDLE DBUSIPC_tMutexHnd;
typedef HANDLE DBUSIPC_tSemHnd;
typedef HANDLE DBUSIPC_tThreadHnd;
typedef DWORD DBUSIPC_tProcId;

#ifndef PRIu64
#define PRIu64 "I64u"
#endif

#elif OS_QNX

typedef pthread_mutex_t DBUSIPC_tMutexHnd;
typedef sem_t DBUSIPC_tSemHnd;
typedef pthread_t DBUSIPC_tThreadHnd;
typedef pid_t DBUSIPC_tProcId;

#ifndef PRIu64
#define PRIu64 "llu"
#endif

#elif OS_LINUX
typedef pthread_mutex_t DBUSIPC_tMutexHnd;
typedef sem_t DBUSIPC_tSemHnd;
typedef pthread_t DBUSIPC_tThreadHnd;
typedef pid_t DBUSIPC_tProcId;

#ifndef PRIu64
#define PRIu64 "llu"
#endif
#endif

}  // End NOsTypes

#endif /* Guard for NOSTYPES_HPP_ */
