

#include "PipeWatch.hpp"
#include "Dispatcher.hpp"

PipeWatch::PipeWatch
   (
   int32_t     fd,
   uint32_t    flags,
   bool        enabled,
   tHandleFunc handler,
   void*       data,
   Dispatcher* disp
   )
   : Watch(flags, enabled)
   , mHandler(handler)
   , mUserData(data)
   , mDispatcher(disp)
{
   descriptor(fd);
   mDispatcher->addWatch(this);
}

   
PipeWatch::~PipeWatch()
{
   mDispatcher->removeWatch(this);
}


bool PipeWatch::handle
   (
   uint32_t flags
   )
{
   bool result = false;
   if ( 0 != mHandler )
   {
      result = mHandler(flags, mUserData);
   }
   
   return result;
}
