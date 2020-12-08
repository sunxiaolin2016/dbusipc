#ifndef DBUSIPC_EXPORT_H_
#define DBUSIPC_EXPORT_H_


#ifdef WIN32
	#ifdef DBUSIPC_EXPORTS
	#define DBUSIPC_API __declspec(dllexport)
	#else
	#define DBUSIPC_API __declspec(dllimport)
	#endif
#else
	#define DBUSIPC_API
#endif


#endif //DBUSIPC_EXPORT_H_
