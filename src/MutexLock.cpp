
#include <errno.h>
#include <pthread.h>
#include "MutexLock.hpp"
#include "Exceptions.hpp"

#if OS_LINUX
#define EOK 0
#endif

MutexLock::MutexLock()
   : ILockable()
{
   int rc = EOK;
   pthread_mutexattr_t mutexAttr;

   rc = pthread_mutexattr_init(&mutexAttr);
   if ( EOK != rc )
   {
      throw PosixError(rc);
   }

#if OS_QNX
   rc = pthread_mutexattr_setrecursive(&mutexAttr, PTHREAD_RECURSIVE_ENABLE);
#elif OS_LINUX
   rc = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif

   if ( EOK != rc )
   {
      pthread_mutexattr_destroy(&mutexAttr);
      throw PosixError(rc);
   }

   rc = pthread_mutex_init(&mMutex, &mutexAttr);

   if ( EOK != rc )
   {
      pthread_mutexattr_destroy(&mutexAttr);
      throw PosixError(rc);
   }
}


MutexLock::~MutexLock()
{
   (void)pthread_mutex_destroy(&mMutex);
}

void MutexLock::lock()
{
   int rc = pthread_mutex_lock(&mMutex);
   if ( EOK != rc )
   {
      throw LockError(rc);
   }
}


void MutexLock::unlock()
{
   int rc = pthread_mutex_unlock(&mMutex);
   if ( EOK != rc )
   {
      throw UnlockError(rc);
   }
}
