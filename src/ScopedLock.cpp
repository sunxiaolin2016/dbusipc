

#include "ScopedLock.hpp"

ScopedLock::ScopedLock
   (
   ILockable&  lock
   )
   : scopedLock(lock)
{
   scopedLock.lock();
}

ScopedLock::~ScopedLock()
{
   scopedLock.unlock();
}
