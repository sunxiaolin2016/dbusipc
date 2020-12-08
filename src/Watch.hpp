#ifndef WATCH_HPP_
#define WATCH_HPP_

#include "dbusipc/dbusipc.h"

class Watch
{
public:
	Watch(uint32_t flags, bool enabled = false);
	Watch(const Watch& rhs);
	
	virtual ~Watch();

   virtual bool enabled();
   void enabled(bool option);

   int32_t descriptor() const;

   virtual uint32_t flags();
   void flags(uint32_t f);

   virtual bool handle(uint32_t flags) = 0;

   Watch& operator=(const Watch& rhs);
      
protected:
   void descriptor(int32_t fd);
   
private:
   int32_t  mFd;
   uint32_t mFlags;
   bool     mEnabled;
};

inline void Watch::enabled
   (
   bool option
   )
{
   mEnabled = option;
}

inline int32_t Watch::descriptor() const
{
   return mFd;
}

inline void Watch::descriptor
   (
   int32_t  fd
   )
{
   mFd = fd;
}


inline void Watch::flags
   (
   uint32_t f
   )
{
   mFlags = f;
}


#endif /* Guard for WATCH_HPP_ */
