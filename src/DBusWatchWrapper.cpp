

#include "DBusWatchWrapper.hpp"
#include "Dispatcher.hpp"

DBusWatchWrapper::DBusWatchWrapper
   (
   DBusWatch*     internal,
   Dispatcher*    disp
   )
   : Watch(0U, false)
   , mDBusWatch(internal)
   , mDispatcher(disp)
{
   dbus_watch_set_data(mDBusWatch, this, 0);
   descriptor(dbus_watch_get_unix_fd(mDBusWatch));
   Watch::flags(dbus_watch_get_flags(mDBusWatch));
   Watch::enabled(dbus_watch_get_enabled(mDBusWatch));
   mDispatcher->addWatch(this);
}

   
DBusWatchWrapper::~DBusWatchWrapper()
{
   mDispatcher->removeWatch(this);
   // Reset the data to indicate we're no longer associated with
   // this DBus watch.
   dbus_watch_set_data(mDBusWatch, 0, 0);
}


void DBusWatchWrapper::toggle()
{
   Watch::enabled(dbus_watch_get_enabled(mDBusWatch));
}


bool DBusWatchWrapper::handle
   (
   uint32_t flags
   )
{
   return dbus_watch_handle(mDBusWatch, flags) ? true : false;
}


bool DBusWatchWrapper::enabled()
{
   Watch::enabled(dbus_watch_get_enabled(mDBusWatch));
   return Watch::enabled();
}


uint32_t DBusWatchWrapper::flags()
{
   Watch::flags(dbus_watch_get_flags(mDBusWatch));
   return Watch::flags();
}


