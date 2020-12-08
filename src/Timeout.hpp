#ifndef TIMEOUT_HPP_
#define TIMEOUT_HPP_

#include "dbusipc/dbusipc.h"

class Timeout
{
public:
	Timeout(int32_t msecInterval, bool repeat = true,
	        bool enabled = false);
	Timeout(const Timeout& rhs);
	
	virtual ~Timeout();
	
	int32_t interval() const;
	void interval(int32_t msec);
	
	bool repeat() const;
	void repeat(bool option);
	
	bool enabled() const;
	void enabled(bool option);
	
	uint64_t expiry() const;
	void resetExpiry();
	
	virtual bool handle() = 0;
	Timeout& operator=(const Timeout& rhs);
	
private:
   int32_t        mInterval;
   uint64_t       mExpiry;
   bool           mRepeat;
   bool           mEnabled;
};

inline int32_t Timeout::interval() const
{
   return mInterval;
}

inline bool Timeout::repeat() const
{
   return mRepeat;  
}

inline bool Timeout::enabled() const
{
   return mEnabled;
}

inline uint64_t Timeout::expiry() const
{
   return mExpiry;
}

#endif /* Guard for TIMEOUT_HPP_ */
