#ifndef SINGLETONHOLDER_HPP_
#define SINGLETONHOLDER_HPP_

#include "MutexLock.hpp"
#include "ScopedLock.hpp"

template <class T>
class SingletonHolder
{
public:
   SingletonHolder()
      : lock()
      , instance(0)
   {
   }
   
   ~SingletonHolder()
   {
      delete instance;
   }
   
   T* get()
   {
      ScopedLock slock(lock);
      if (0 == instance ) instance = new T;
      return instance;
   }
   
   void destroy()
   {
      ScopedLock slock(lock);
      delete instance;
      instance = 0;
   }

private:
   MutexLock   lock;
   T*          instance;
};

#endif /* Guard for SINGLETONHOLDER_HPP_ */
