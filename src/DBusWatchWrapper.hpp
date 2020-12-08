#ifndef DBUSWATCHWRAPPER_HPP_
#define DBUSWATCHWRAPPER_HPP_

#include "dbus/dbus.h"
#include "Watch.hpp"

//
// Forward Declarations
//
class Dispatcher;

class DBusWatchWrapper : public Watch
{
public:
   DBusWatchWrapper(DBusWatch* internal, Dispatcher* disp);
	virtual ~DBusWatchWrapper();
	
	virtual bool enabled();
	virtual uint32_t flags();
	
   void toggle();
   
   virtual bool handle(uint32_t flags);
	   
private:
   
   // Private copy constructor and assignment operator to prevent misuse
   DBusWatchWrapper(const DBusWatchWrapper& rhs);
   DBusWatchWrapper& operator=(const DBusWatchWrapper& rhs);
      
   DBusWatch*     mDBusWatch;
   Dispatcher*    mDispatcher;
};

#endif /* Guard for DBUSWATCHWRAPPER_HPP_ */
