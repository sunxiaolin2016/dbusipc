
#include <errno.h>
#include <stdlib.h>
#include "Thread.hpp"
#include "Exceptions.hpp"
#include <pthread.h>
// Min/max thread priority range for QNX
static const int32_t MIN_THREAD_PRIORITY = 8;
static const int32_t MAX_THREAD_PRIORITY = 22;

Thread::Thread()
   : mThread()
   , mStartedSem(new Semaphore(0 /* Not signaled */))
   , mMutex()
   , mPriority(10)
   , mRunning(false)
   , mQuit(false)
{
   int32_t schedPolicy;
   struct sched_param schedParam;
   if( EOK == pthread_getschedparam(pthread_self(), &schedPolicy, &schedParam) )
   {
      mPriority = schedParam.sched_priority;
   }
}

Thread::~Thread()
{
   stop();
   wait(INFINITE_WAIT);
}

int32_t Thread::start
   (
   bool  blockUntilRunning
   )
{
   int32_t rc = EOK;

   try
   {
      mMutex.lock();
      mQuit = false;
      if ( mRunning )
      {
         rc = EBUSY;
      }
      else
      {
         pthread_attr_t threadAttr;
         if ( (EOK == (rc = pthread_attr_init(&threadAttr))) )
         {
            if ( EOK == (rc = pthread_attr_setdetachstate(&threadAttr,
            PTHREAD_CREATE_JOINABLE)) )
            {
               rc = pthread_create(&mThread, &threadAttr, threadFunc, this);

               if ( EOK == rc )
               {
                  applyPriority(mPriority);

                  if ( blockUntilRunning )
                  {
                     mStartedSem->wait();
                     mRunning = true;
                  }
               }
            }
            // We no longer need the attribute structure
            (void)pthread_attr_destroy(&threadAttr);
         }
      }
      mMutex.unlock();
   }
   catch ( const PosixError & e )
   {
      rc = e.getError();
   }

   return rc;
}


void Thread::stop()
{
   mQuit = true;
}


int32_t Thread::wait
   (
   int32_t  msecWait
   )
{
   int32_t rc = EOK;

   if ( isRunning() )
   {
      if ( 0 <= msecWait )
      {
         struct timespec tm;
         tm.tv_sec = msecWait / 1000U;
         tm.tv_nsec = (msecWait % 1000U) * 1000000U;
#if OS_QNX		 
         rc = pthread_timedjoin(mThread, 0, &tm);
#elif OS_LINUX
    //rc = pthread_timedjoin_np(mThread, 0, &tm);
    rc = pthread_join(mThread, 0);
#endif
      }
      else
      {
         rc = pthread_join(mThread, 0);
      }
   }

   return rc;
}


bool Thread::isRunning()
{
   bool status = false;
   mMutex.lock();
   status = mRunning;
   mMutex.unlock();

   return status;
}

bool Thread::isCurrentThread() const
{
   return (0 != pthread_equal(mThread, pthread_self()));
}


bool Thread::setPriority
   (
   int32_t  level
   )
{
   bool status(false);

   if ( (level >= MIN_THREAD_PRIORITY) &&
      (level <= MAX_THREAD_PRIORITY) )
   {
      mPriority = level;
      status = true;
      if ( isRunning() )
      {
         status = applyPriority(mPriority);
      }
   }

   return status;
}


bool Thread::applyPriority
   (
   int32_t  level
   )
{
   bool status(false);

   int32_t schedPolicy;
   struct sched_param schedParam;
   if( EOK == pthread_getschedparam( mThread, &schedPolicy, &schedParam ) )
   {
      schedParam.sched_priority = level;
      if ( EOK == pthread_setschedparam(mThread, schedPolicy, &schedParam) )
      {
         status = true;
      }
   }

   return status;
}


void* Thread::threadFunc
   (
   void* d
   )
{
   Thread* self = static_cast<Thread*>(d);

   // If someone is waiting for the thread to start then indicate
   // that it has started
   self->mStartedSem->post();

   self->mMutex.lock();
   self->mRunning = true;
   self->mMutex.unlock();


   // Loop until told to quit or execution is done
   while ( !self->mQuit && self->execute() );

   self->mMutex.lock();
   self->mRunning = false;
   self->mStartedSem.reset(new Semaphore(0 /* Not signaled */));
   self->mMutex.unlock();

   return 0;
}


