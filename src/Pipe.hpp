

#ifndef PIPE_HPP_
#define PIPE_HPP_

#include "dbusipc/dbusipc.h"
#include <unistd.h>

class Pipe
{
public:
   // Invalid Pipe file descriptor
   static const int32_t INVALID_FD = -1;

   Pipe();
   ~Pipe();

   bool open(bool blockOnRead = true, bool blockOnWrite = false);
   bool close();

   int32_t getReadFd() const;
   int32_t getWriteFd() const;

   int32_t read(void* buf, uint32_t nBytes);
   int32_t write(const void* buf, uint32_t nBytes);

private:
   int32_t  mReadFd;
   int32_t  mWriteFd;
};
#endif /* Guard for PIPE_HPP_ */
