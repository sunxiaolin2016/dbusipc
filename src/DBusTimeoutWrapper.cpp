
#include "DBusTimeoutWrapper.hpp"
#include "Dispatcher.hpp"

DBusTimeoutWrapper::DBusTimeoutWrapper
   (
   DBusTimeout*   internal,
   Dispatcher*    disp
   )
   : Timeout(0, true, true)
   , mDBusTimeout(internal)
   , mDispatcher(disp)
{
   dbus_timeout_set_data(mDBusTimeout, this, 0);
   interval(dbus_timeout_get_interval(mDBusTimeout));
   enabled(dbus_timeout_get_enabled(mDBusTimeout));
   mDispatcher->addTimeout(this);
}


DBusTimeoutWrapper::~DBusTimeoutWrapper()
{
   mDispatcher->removeTimeout(this);
   // Reset the data to indicate we're no longer associated with
   // this DBus timeout.
   dbus_timeout_set_data(mDBusTimeout, 0, 0);
}


void DBusTimeoutWrapper::toggle()
{
   enabled(dbus_timeout_get_enabled(mDBusTimeout));
   // According to the D-Bus documentation, the interval
   // may change between toggles. As a result, we have to
   // update the interval to catch any changes.
   interval(dbus_timeout_get_interval(mDBusTimeout));
}


bool DBusTimeoutWrapper::handle()
{
   return dbus_timeout_handle(mDBusTimeout) ? true : false;
}
