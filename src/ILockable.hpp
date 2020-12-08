#ifndef ILOCKABLE_HPP_
#define ILOCKABLE_HPP_


class ILockable
{
public:
	ILockable() {};
	virtual ~ILockable() {};
	
	virtual void lock() = 0;
	virtual void unlock() = 0;
};

#endif /* Guard for ILOCKABLE_HPP_ */
