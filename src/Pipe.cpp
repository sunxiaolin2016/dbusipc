

#include "Pipe.hpp"

#include <fcntl.h>


Pipe::Pipe()
   : mReadFd(INVALID_FD)
   , mWriteFd(INVALID_FD)
{
}


Pipe::~Pipe()
{
   (void)close();
}


bool Pipe::open
   (
   bool  blockOnRead,
   bool  blockOnWrite
   )
{
   int32_t fds[2] = {INVALID_FD, INVALID_FD};
   bool status(false);

   int32_t rc = pipe(fds);
   if ( 0 == rc )
   {
      // If reads are non-blocking then ...
      if ( !blockOnRead )
      {
         rc = fcntl(fds[0], F_SETFD, O_NONBLOCK);
         if ( 0 > rc )
         {
            (void)::close(fds[0]);
            fds[0] = INVALID_FD;
         }
      }

      // if writes are non-blocking then ...
      if ( !blockOnWrite )
      {
         rc = fcntl(fds[1], F_SETFD, O_NONBLOCK);
         if ( 0 > rc )
         {
            (void)::close(fds[1]);
            fds[1] = INVALID_FD;
         }
      }

      if ( (INVALID_FD != fds[0]) && (INVALID_FD != fds[1]) )
      {
         status = true;
         mReadFd = fds[0];
         mWriteFd = fds[1];
      }
   }

   // Do necessary clean-up if there was a failure opening the pipe.
   if ( !status )
   {
      if ( INVALID_FD != fds[0] )
      {
         (void)::close(fds[0]);
      }

      if ( INVALID_FD != fds[1] )
      {
         (void)::close(fds[1]);
      }
   }

   return status;
}


bool Pipe::close()
{
   bool closed(true);

   if ( INVALID_FD != mReadFd )
   {
      if ( 0 != ::close(mReadFd) )
      {
         closed = false;
      }
      mReadFd = INVALID_FD;
   }

   if ( INVALID_FD != mWriteFd )
   {
      if ( 0 != ::close(mWriteFd) )
      {
         closed = false;
      }
      mWriteFd = INVALID_FD;
   }

   return closed;
}


int32_t Pipe::getReadFd() const
{
   return mReadFd;
}


int32_t Pipe::getWriteFd() const
{
   return mWriteFd;
}


int32_t Pipe::read
   (
   void*    buf,
   uint32_t nBytes
   )
{
   return ::read(mReadFd, buf, nBytes);
}


int32_t Pipe::write
   (
   const void* buf,
   uint32_t    nBytes
   )
{
   return ::write(mWriteFd, buf, nBytes);
}

