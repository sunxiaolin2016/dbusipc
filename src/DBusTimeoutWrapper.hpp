#ifndef DBUSTIMEOUTWRAPPER_HPP_
#define DBUSTIMEOUTWRAPPER_HPP_

#include "dbus/dbus.h"
#include "Timeout.hpp"

//
// Forward Declarations
//
class Dispatcher;

class DBusTimeoutWrapper : public Timeout
{
public:
	DBusTimeoutWrapper(DBusTimeout* internal, Dispatcher* disp);
	virtual ~DBusTimeoutWrapper();
	
	void toggle();
	virtual bool handle();

private:
   // Private copy constructor and assignment operator to prevent misuse
   DBusTimeoutWrapper(const DBusTimeoutWrapper& rhs);
   DBusTimeoutWrapper& operator=(const DBusTimeoutWrapper& rhs);
   
   DBusTimeout*   mDBusTimeout;
   Dispatcher*    mDispatcher;
};

#endif /* Guard for DBUSTIMEOUTWRAPPER_HPP_ */
