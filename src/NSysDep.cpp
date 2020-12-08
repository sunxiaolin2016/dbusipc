

#include "NSysDep.hpp"

#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if OS_QNX
#include <sys/slog.h>
#include <sys/slogcodes.h>
#endif
#include <cstdlib>
#include "dbus/dbus.h"

namespace NSysDep
{

bool DBUSIPC_startupNetworkStack()
{
   return true;
}


bool DBUSIPC_shutdownNetworkStack()
{
   return true;
}


int32_t DBUSIPC_poll
   (
   DBUSIPC_tPollFdContainer&   fds,
   int32_t                    msecTimeout
   )
{
   std::vector<struct pollfd> pollFdList;
   std::vector<struct pollfd>::iterator pollIt;
   DBUSIPC_tPollFdContainer::iterator dbusPollIt;
   struct pollfd pollFd;
   DBUSIPC_tPollFd dbusFd;
   int32_t nSelected(0);
   int32_t flags(0);

   for ( dbusPollIt = fds.begin(); dbusPollIt != fds.end(); ++dbusPollIt )
   {
      flags = (*dbusPollIt).events;
      pollFd.events = 0;
      pollFd.revents = 0;
      pollFd.fd = (*dbusPollIt).fd;
      if ( flags & DBUS_WATCH_READABLE ) pollFd.events |= POLLIN;
      if ( flags & DBUS_WATCH_WRITABLE ) pollFd.events |= POLLOUT;
      if ( flags & DBUS_WATCH_HANGUP ) pollFd.events |= POLLHUP;
      if ( flags & DBUS_WATCH_ERROR ) pollFd.events  |= POLLERR;
      pollFdList.push_back(pollFd);
   }

   nSelected = poll(&pollFdList[0], pollFdList.size(), msecTimeout);

   if ( 0 < nSelected )
   {
      fds.resize(0);
      for ( pollIt = pollFdList.begin(); pollIt != pollFdList.end(); pollIt++ )
      {
         dbusFd.fd = (*pollIt).fd;
         dbusFd.events = (*pollIt).events;
         dbusFd.revents = 0;
         flags = (*pollIt).revents;
         if ( flags & POLLIN ) dbusFd.revents |= DBUS_WATCH_READABLE;
         if ( flags & POLLOUT ) dbusFd.revents |= DBUS_WATCH_WRITABLE;
         if ( flags & POLLHUP ) dbusFd.revents |= DBUS_WATCH_HANGUP;
         if ( flags & POLLERR ) dbusFd.revents |= DBUS_WATCH_ERROR;
         fds.push_back(dbusFd);
      }
   }

   return nSelected;
}


uint64_t DBUSIPC_getSystemTime()
{
   struct timespec now;
   uint64_t msecTime(0U);

   if ( -1 != clock_gettime(CLOCK_MONOTONIC, &now) )
   {
      msecTime = (now.tv_sec * 1000U) + (now.tv_nsec / 1000000U);
   }

   return msecTime;
}


void DBUSIPC_sleep
   (
   uint32_t msecTimeout
   )
{
   static_cast<void>(usleep(static_cast<useconds_t>(msecTimeout * 1000U)));
   return;
}


std::string DBUSIPC_getenv
   (
   const char* varName
   )
{
   std::string value;

   if ( 0 != varName )
   {
      const char* v = std::getenv(varName);
      if ( 0 != v )
      {
         value.append(v);
      }
   }
   return value;
}


void DBUSIPC_setEnvironmentVariable()
{
   // A ridiculously large bus address;
   static const uint32_t MAX_DBUS_BUS_ADDRESS_SIZE = 1024;
   char busAddress[MAX_DBUS_BUS_ADDRESS_SIZE+1] = {0};
   char scanString[256] = {0};

   // Check if DBUS environment variable has already been set
   if (0 == getenv("DBUS_SESSION_BUS_ADDRESS"))
   {
      char *environmentShellName = getenv("DBUS_SCRIPT_FILE_NAME");
      if (0 != environmentShellName)
      {
         FILE *fd = fopen(environmentShellName, "r");
         if (fd)
         {
            snprintf(scanString, sizeof(scanString), "DBUS_SESSION_BUS_ADDRESS='%%%ds';", MAX_DBUS_BUS_ADDRESS_SIZE);
            fscanf(fd, scanString, busAddress);
      
            if ((strlen(busAddress) < MAX_DBUS_BUS_ADDRESS_SIZE) && (strlen(busAddress) > 2))
            {
               if (strcmp(&busAddress[strlen(busAddress)-2], "';") == 0)
               {
                  busAddress[strlen(busAddress)-2]= '\0';
               }

               setenv("DBUS_SESSION_BUS_ADDRESS", busAddress, 0);
            }
            
            fclose(fd);
         }
      }
   }
}


NOsTypes::DBUSIPC_tProcId DBUSIPC_getProcId()
{
   return getpid();
}


#if OS_QNX
int32_t DBUSIPC_slog
   (
   DBUSIPC_tSlogSeverity sev,
   const char*          fmt,
   ...
   )
{
   va_list arg;
   va_start(arg, fmt);

   int32_t result = vslogf(_SLOG_SETCODE(_SLOGC_TEST, 0),
                           static_cast<int32_t>(sev), fmt, arg);
   va_end(arg);

   return result;
}

#elif OS_LINUX

int32_t DBUSIPC_slog
   (
   DBUSIPC_tSlogSeverity sev,
   const char*          fmt,
   ...
   )
{

   return 0;
}

#endif

}  // End of namespace: NSysDep
