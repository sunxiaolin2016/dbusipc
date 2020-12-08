
#include <stdlib.h>
#include "trace.h"

void DBUSIPC_trace
   (
   FILE*             fp,
   DBUSIPC_tConstStr  name,
   DBUSIPC_tConstStr  fmt,
   va_list*          list
   )
{
   fprintf(fp, "%s: ", name);
   vfprintf(fp, fmt, *list);
   fprintf(fp, "\n");
   fflush(fp);
}

void DBUSIPC_traceInfo
   (
   DBUSIPC_tConstStr fmt,
   ...
   )
{
   va_list argList;
   va_start(argList, fmt);
   DBUSIPC_trace(stdout, "INFO", fmt, &argList);
   va_end(argList);   
}


void DBUSIPC_traceWarn
   (
   DBUSIPC_tConstStr fmt,
   ...
   )
{
   va_list argList;
   va_start(argList, fmt);
   DBUSIPC_trace(stderr, "WARN", fmt, &argList);
   va_end(argList);   
}


void DBUSIPC_traceError
   (
   DBUSIPC_tConstStr  fmt,
   ...
   )
{
   va_list argList;
   va_start(argList, fmt);
   DBUSIPC_trace(stderr, "ERR", fmt, &argList);
   va_end(argList);   
}

