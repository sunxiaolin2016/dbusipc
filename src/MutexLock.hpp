#ifndef MUTEXLOCK_HPP_
#define MUTEXLOCK_HPP_


#include "NOsTypes.hpp"
#include "ILockable.hpp"

class MutexLock : public ILockable
{
public:
	MutexLock();
	virtual ~MutexLock();

   virtual void lock();
   virtual void unlock();
private:
   NOsTypes::DBUSIPC_tMutexHnd   mMutex;
};

#endif /* Guard for MUTEXLOCK_HPP_ */
