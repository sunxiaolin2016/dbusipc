#ifndef PIPEWATCH_HPP_
#define PIPEWATCH_HPP_

#include "Watch.hpp"

//
// Forward Declarations
//
class Dispatcher;

class PipeWatch : public Watch
{
public:
   typedef bool (*tHandleFunc)(uint32_t, void*);
   
	PipeWatch(int32_t fd, uint32_t flags, bool enabled,
	         tHandleFunc handler, void* data, Dispatcher* disp);
	virtual ~PipeWatch();

private:
   // Private copy constructor and assignment operator to prevent misuse
   PipeWatch(const PipeWatch& rhs);
   PipeWatch& operator=(const PipeWatch& rhs);
   
   virtual bool handle(uint32_t flags);
   tHandleFunc mHandler;
   void*       mUserData;
   Dispatcher* mDispatcher;
};

#endif /* Guard for PIPEWATCH_HPP_ */
