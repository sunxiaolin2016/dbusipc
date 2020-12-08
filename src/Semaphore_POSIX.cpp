


#include "Semaphore_POSIX.h"

#include <time.h>
#include <sys/time.h>




SemaphoreImpl::SemaphoreImpl(int n, int max): _n(n), _max(max)
{
	//poco_assert (n >= 0 && max > 0 && n <= max);
	//printf("-chb-SemaphoreImpl construct %d %d\n",_n,_max);
	if(_n==0)
	{
		_max=1;
	}
	if (pthread_mutex_init(&_mutex, NULL))
	{
		//throw SystemException("cannot create semaphore (mutex)");
		printf("cannot create semaphore (mutex)\n");
		return;
	}
	
	pthread_condattr_t attr;
	if (pthread_condattr_init(&attr))
	{
		pthread_mutex_destroy(&_mutex);
		//throw SystemException("cannot create semaphore (condition attribute)");
		printf("cannot create semaphore (condition attribute)\n");
		return;
	}
	if (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC))
    {
		pthread_condattr_destroy(&attr);
		pthread_mutex_destroy(&_mutex);
		//throw SystemException("cannot create semaphore (condition attribute clock)");
		printf("cannot create semaphore (condition attribute clock)\n");
		return;
    }
	if (pthread_cond_init(&_cond, &attr))
	{
		pthread_condattr_destroy(&attr);
		pthread_mutex_destroy(&_mutex);
		//throw SystemException("cannot create semaphore (condition)");
		printf("cannot create semaphore (condition attribute clock)\n");
		return;
	}
	pthread_condattr_destroy(&attr);

}


SemaphoreImpl::~SemaphoreImpl()
{
	pthread_cond_destroy(&_cond);
	pthread_mutex_destroy(&_mutex);
}


void SemaphoreImpl::waitImpl()
{
	if (pthread_mutex_lock(&_mutex))
	{
		//throw SystemException("wait for semaphore failed (lock)"); 
		printf("wait for semaphore failed (lock)\n");
		return;
	}
		
	while (_n  < 1) 
	{
		//printf("-chb- will wait condition\n");
		pthread_cond_wait(&_cond, &_mutex);
		//if (pthread_cond_wait(&_cond, &_mutex))
		//{
		//	pthread_mutex_unlock(&_mutex);
			//throw SystemException("wait for semaphore failed");
		//	printf("wait for semaphore failed\n");
			//return;
		//}
	}
	--_n;
	pthread_mutex_unlock(&_mutex);
}


bool SemaphoreImpl::waitImpl(long milliseconds)
{
	int rc = 0;
	struct timespec abstime;

	clock_gettime(CLOCK_MONOTONIC, &abstime);
	abstime.tv_sec  += milliseconds / 1000;
	abstime.tv_nsec += (milliseconds % 1000)*1000000;
	if (abstime.tv_nsec >= 1000000000)
	{
		abstime.tv_nsec -= 1000000000;
		abstime.tv_sec++;
	}


	if (pthread_mutex_lock(&_mutex) != 0)
	{
		//throw SystemException("wait for semaphore failed (lock)"); 
		printf("wait for semaphore failed (lock)\n");
		return false;
	}
		
	while (_n < 1) 
	{
		if ((rc = pthread_cond_timedwait(&_cond, &_mutex, &abstime)))
		{
			if (rc == ETIMEDOUT) break;
			pthread_mutex_unlock(&_mutex);
			//throw SystemException("cannot wait for semaphore");
			printf("cannot wait for semaphore\n");
			return false;
		}
	}
	if (rc == 0) --_n;
	pthread_mutex_unlock(&_mutex);
	return rc == 0;
}


