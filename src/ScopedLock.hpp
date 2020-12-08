#ifndef SCOPEDLOCK_HPP_
#define SCOPEDLOCK_HPP_

#include "ILockable.hpp"

class ScopedLock
{
public:
	ScopedLock(ILockable& lock);
	~ScopedLock();
private:
   ILockable& scopedLock;
};

#endif /* Guard for SCOPEDLOCK_HPP_ */
