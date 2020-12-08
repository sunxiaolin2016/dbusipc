#ifndef REQUESTCONTEXT_HPP_
#define REQUESTCONTEXT_HPP_

#include "dbusipc/dbusipc.h"
//
// Forward Declarations
//
struct DBusMessage;
class Connection;

class RequestContext
{
public:
	RequestContext(Connection* conn, DBusMessage* request);
	~RequestContext();

	DBUSIPC_tError sendReply(DBUSIPC_tConstStr result);
	DBUSIPC_tError sendError(DBUSIPC_tConstStr errName, DBUSIPC_tConstStr errMsg);
	
private:
   // (Unimplemented) private copy constructor and assignment operator
   // to prevent misuse
   RequestContext(const RequestContext& other);
   RequestContext& operator=(const RequestContext& rhs);
   
   Connection*    mConn;
   DBusMessage*   mReqMsg;
};

#endif /* Guard for REQUESTCONTEXT_HPP_ */
