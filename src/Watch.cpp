

#include "Watch.hpp"

Watch::Watch
   (
   uint32_t flags,
   bool     enabled
   )
   : mFd(-1)
   , mFlags(flags)
   , mEnabled(enabled)
{
}


Watch::Watch
   (
   const Watch&   rhs
   )
   : mFd(rhs.mFd)
   , mFlags(rhs.mFlags)
   , mEnabled(rhs.mEnabled)
{
}


Watch::~Watch()
{
}


Watch& Watch::operator=
   (
   const Watch&   rhs
   )
{
   if ( &rhs != this )
   {
      mFd = rhs.mFd;
      mFlags = rhs.mFlags;
      mEnabled = rhs.mEnabled;
   }
   
   return *this;
}

bool Watch::enabled()
{
   return mEnabled;
}


uint32_t Watch::flags()
{
   return mFlags;
}

