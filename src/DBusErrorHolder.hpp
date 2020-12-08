#ifndef DBUSERRORHOLDER_HPP_
#define DBUSERRORHOLDER_HPP_

#include "dbus/dbus.h"
#include "dbusipc/dbusipc.h"

class DBusErrorHolder
{
public:
	DBusErrorHolder();
	~DBusErrorHolder();
	
	bool hasName(DBUSIPC_tConstStr name);
	bool isSet();
	DBUSIPC_tConstStr getName() const;
	DBUSIPC_tConstStr getMessage() const;

	DBusError* getInst();
	
private:
   // Provide no implementation to prevent mis-use
   DBusErrorHolder(const DBusErrorHolder& other);
   DBusErrorHolder& operator=(const DBusErrorHolder& rhs);
   
   DBusError   mError;
};

inline DBusErrorHolder::DBusErrorHolder()
{
   dbus_error_init(&mError);
}


inline DBusErrorHolder::~DBusErrorHolder()
{
   dbus_error_free(&mError);
}

inline bool DBusErrorHolder::hasName
   (
   DBUSIPC_tConstStr  name
   )
{
   return dbus_error_has_name(&mError, name) ? true : false;
}

inline bool DBusErrorHolder::isSet()
{
   return dbus_error_is_set(&mError) ? true : false;
}

inline DBUSIPC_tConstStr DBusErrorHolder::getName() const
{
   return mError.name ? mError.name : "";
}

inline DBUSIPC_tConstStr DBusErrorHolder::getMessage() const
{
   return mError.message ? mError.message : "";
}

inline DBusError* DBusErrorHolder::getInst()
{
   return &mError;
}


#endif /* Guard for DBUSERRORHOLDER_HPP_ */
