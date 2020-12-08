
#include <time.h>
#include "Timeout.hpp"
#include "NSysDep.hpp"

Timeout::Timeout
   (
   int32_t  msecInterval,
   bool     repeat,
   bool     enabled
   )
   : mInterval(msecInterval)
   , mExpiry(0)
   , mRepeat(repeat)
   , mEnabled(enabled)
{
   resetExpiry();
}

Timeout::Timeout
   (
   const Timeout& rhs
   )
{
   operator=(rhs);
}

Timeout::~Timeout()
{
}


void Timeout::interval
   (
   int32_t  msec
   )
{
   mInterval = msec;
   if ( mEnabled )
   {
      resetExpiry();
   }
}


void Timeout::repeat
   (
   bool  option
   )
{
   mRepeat = option;
}


void Timeout::enabled
   (
   bool  option
   )
{
   mEnabled = option;
   if ( mEnabled )
   {
      resetExpiry();
   }
}


void Timeout::resetExpiry()
{
   mExpiry = NSysDep::DBUSIPC_getSystemTime() + static_cast<uint64_t>(mInterval);
}

Timeout& Timeout::operator=
   (
   const Timeout& rhs
   )
{
   if ( &rhs != this )
   {
      mInterval = rhs.mInterval;
      mExpiry = rhs.mExpiry;
      mRepeat = rhs.mRepeat;
      mEnabled = rhs.mEnabled;
   }

   return *this;
}
