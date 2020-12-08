
#include "Semaphore.hpp"
#include "Semaphore_POSIX.h"


Semaphore::Semaphore(int n): SemaphoreImpl(n, n)
{
}


Semaphore::Semaphore(int n, int max): SemaphoreImpl(n, max)
{
}


Semaphore::~Semaphore()
{
}

