#ifndef THREAD_HPP_
#define THREAD_HPP_

#include <memory>
#include "dbusipc/dbusipc.h"
#include "NOsTypes.hpp"
#include "Semaphore.hpp"
#include "MutexLock.hpp"
#include <pthread.h>

class Thread
{
public:
   static const int32_t INFINITE_WAIT = -1;

	Thread();
	virtual ~Thread();

	int32_t start(bool blockUntilRunning = false);
	void stop();
	int32_t wait(int32_t msecWait = INFINITE_WAIT);
	bool isRunning();
	bool isCurrentThread() const;
   bool setPriority(int32_t level);

protected:
   // Return false to terminate
   virtual bool execute() { return false; }
   bool applyPriority(int32_t level);

private:
   NOsTypes::DBUSIPC_tThreadHnd   mThread;
   std::auto_ptr<Semaphore>      mStartedSem;
   MutexLock                     mMutex;
   int32_t                       mPriority;
   volatile bool                 mRunning;
   volatile bool                 mQuit;

#if OS_QNX
   static void* threadFunc(void* d);
#elif OS_LINUX
  static void* threadFunc(void* d);
#elif WIN32
   static unsigned int __stdcall threadFunc(void* d);
   DWORD                         mThreadId;
#endif
};

#endif /* Guard for THREAD_H_ */
