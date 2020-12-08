
#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "Semaphore_POSIX.h"

class  Semaphore: private SemaphoreImpl
	/// A Semaphore is a synchronization object with the following 
	/// characteristics:
	/// A semaphore has a value that is constrained to be a non-negative
	/// integer and two atomic operations. The allowable operations are V 
	/// (here called set()) and P (here called wait()). A V (set()) operation 
	/// increases the value of the semaphore by one. 
	/// A P (wait()) operation decreases the value of the semaphore by one, 
	/// provided that can be done without violating the constraint that the 
	/// value be non-negative. A P (wait()) operation that is initiated when 
	/// the value of the semaphore is 0 suspends the calling thread. 
	/// The calling thread may continue when the value becomes positive again.
{
public:
	Semaphore(int n);
	Semaphore(int n, int max);
		/// Creates the semaphore. The current value
		/// of the semaphore is given in n. The
		/// maximum value of the semaphore is given
		/// in max.
		/// If only n is given, it must be greater than
		/// zero.
		/// If both n and max are given, max must be
		/// greater than zero, n must be greater than
		/// or equal to zero and less than or equal
		/// to max.
		
	~Semaphore();
		/// Destroys the semaphore.

	void post();
		/// Increments the semaphore's value by one and
		/// thus signals the semaphore. Another thread
		/// waiting for the semaphore will be able
		/// to continue.

	void wait();
		/// Waits for the semaphore to become signalled.
		/// To become signalled, a semaphore's value must
		/// be greater than zero. 
		/// Decrements the semaphore's value by one.

	void wait(long milliseconds);
		/// Waits for the semaphore to become signalled.
		/// To become signalled, a semaphore's value must
		/// be greater than zero.
		/// Throws a TimeoutException if the semaphore
		/// does not become signalled within the specified
		/// time interval.
		/// Decrements the semaphore's value by one
		/// if successful.

	bool tryWait(long milliseconds);
		/// Waits for the semaphore to become signalled.
		/// To become signalled, a semaphore's value must
		/// be greater than zero.
		/// Returns true if the semaphore
		/// became signalled within the specified
		/// time interval, false otherwise.
		/// Decrements the semaphore's value by one
		/// if successful.
	
private:
	Semaphore();
	Semaphore(const Semaphore&);
	Semaphore& operator = (const Semaphore&);
};


//
// inlines
//
inline void Semaphore::post()
{
	setImpl();
}


inline void Semaphore::wait()
{
	waitImpl();
}


inline void Semaphore::wait(long milliseconds)
{
	if (!waitImpl(milliseconds))
	{
		//throw TimeoutException();
		printf("semaphore wait timeout\n");
		return;
	}
		
}


inline bool Semaphore::tryWait(long milliseconds)
{
	return waitImpl(milliseconds);
}




#endif // _SEMAPHORE_H_
