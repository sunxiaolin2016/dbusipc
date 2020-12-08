#ifndef SERVICEREGISTRATION_HPP_
#define SERVICEREGISTRATION_HPP_

#include <string>
#include "dbusipc/dbusipc.h"

//
// Forward Declaration
//
class Connection;
struct DBusMessage;

class ServiceRegistration
{
public:
   ServiceRegistration(Connection* conn,
            const std::string& busName,
            const std::string& objPath,
            uint32_t flag,
            DBUSIPC_tRequestCallback onRequest,
            DBUSIPC_tUserToken token);
	
	virtual ~ServiceRegistration();
	
	const char* getBusName() const;
	const char* getObjectPath() const;
	Connection* getConnection();
	uint32_t getFlags() const;

	void dispatch(DBusMessage* reqMsg, DBUSIPC_tConstStr method,
                 DBUSIPC_tConstStr parms,
                 uint64_t timeout = DBUSIPC_MAX_UINT64);
	void introspect(std::string& xml);

private:
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   ServiceRegistration(const ServiceRegistration& other);
   ServiceRegistration& operator=(const ServiceRegistration& rhs);
   
   Connection*             mConn;
   std::string             mBusName;
   std::string             mObjectPath;
   uint32_t                mFlags;
   DBUSIPC_tRequestCallback mOnRequest;
   DBUSIPC_tUserToken       mUserToken;
};

inline const char* ServiceRegistration::getBusName() const
{
   return mBusName.c_str();
}


inline const char* ServiceRegistration::getObjectPath() const
{
   return mObjectPath.c_str();
}


inline uint32_t ServiceRegistration::getFlags() const
{
   return mFlags;
}

inline Connection* ServiceRegistration::getConnection()
{
   return mConn;
}

#endif /* Guard for SERVICEREGISTRATION_HPP_ */
