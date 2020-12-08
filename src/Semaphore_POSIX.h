#ifndef _SEMAPHORE_POSIX_H_
#define _SEMAPHORE_POSIX_H_


#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#if OS_LINUX
#define EOK 0
#endif




class  SemaphoreImpl
{
protected:
	SemaphoreImpl(int n, int max);		
	~SemaphoreImpl();
	void setImpl();
	void waitImpl();
	bool waitImpl(long milliseconds);
	
private:
	volatile int    _n;
	int             _max;
	pthread_mutex_t _mutex;
	pthread_cond_t  _cond;
};


//
// inlines
//
inline void SemaphoreImpl::setImpl()
{
	if (pthread_mutex_lock(&_mutex) != EOK)
	{
		//throw SystemException("cannot signal semaphore (lock)");
		printf("semaphore post can't lock %d %s\n",errno, strerror(errno));
		return;
	}		
		
	if (_n < _max)
	{
		++_n;
	}
	else
	{
		pthread_mutex_unlock(&_mutex);
		//throw SystemException("cannot signal semaphore: count would exceed maximum");
		printf("cannot signal semaphore: count would exceed maximum %d %d\n",_n,_max);
		return;
	}	
	
	if (pthread_cond_signal(&_cond) != EOK)
	{
		//pthread_mutex_unlock(&_mutex);
		printf("cannot signal semaphore %d %s\n",errno, strerror(errno));
	}
	pthread_mutex_unlock(&_mutex);
}




#endif //_SEMAPHORE_POSIX_H_
