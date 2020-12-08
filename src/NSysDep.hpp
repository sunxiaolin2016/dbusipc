
#ifndef NSYSDEP_HPP_
#define NSYSDEP_HPP_

#include <vector>
#include <string>
#include "dbusipc/dbusipc.h"
#include "NOsTypes.hpp"
#include <unistd.h>

namespace NSysDep {

typedef struct DBUSIPC_tPollFd
{
   int32_t  fd;
   int16_t  events;  // Events to poll for
   int16_t  revents; // Events that occurred
} DBUSIPC_tPollFd;

typedef std::vector<DBUSIPC_tPollFd> DBUSIPC_tPollFdContainer;

bool DBUSIPC_startupNetworkStack();
bool DBUSIPC_shutdownNetworkStack();
int32_t DBUSIPC_poll(DBUSIPC_tPollFdContainer& fds, int32_t msecTimeout);
void DBUSIPC_sleep(uint32_t msecTimeout);
std::string DBUSIPC_getenv(const char* varName);
NOsTypes::DBUSIPC_tProcId DBUSIPC_getProcId();

// Returns time in msec since system started.
uint64_t DBUSIPC_getSystemTime();

void DBUSIPC_setEnvironmentVariable();

typedef enum {
   SLOG_SEV_SHUTDOWN    = 0,
   SLOG_SEV_CRITICAL    = 1,
   SLOG_SEV_ERROR       = 2,
   SLOG_SEV_WARNING     = 3,
   SLOG_SEV_NOTICE      = 4,
   SLOG_SEV_INFO        = 5,
   SLOG_SEV_DEBUG1      = 6,
   SLOG_SEV_DEBUG2      = 7
} DBUSIPC_tSlogSeverity;

int32_t DBUSIPC_slog(DBUSIPC_tSlogSeverity sev, const char* fmt, ...);

}  // End namespace: NSysDep

#endif /* Guard for NSYSDEP_HPP_ */
