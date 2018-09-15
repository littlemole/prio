#ifndef _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_API_TIMEOUT_HANDLER_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_API_TIMEOUT_HANDLER_DEF_GUARD_


#include "priocpp/common.h"


namespace prio      {


//////////////////////////////////////////////////////////////
// do something next on EventLoop
//////////////////////////////////////////////////////////////

repro::Future<> timeout(int secs, int ms) noexcept;
repro::Future<> timeout(int secs) noexcept;

repro::Future<> nextTick() noexcept;

//////////////////////////////////////////////////////////////
// timeout events.
// threadsafe versions, callable from worker thread
//////////////////////////////////////////////////////////////

void timeout( const std::function<void()>& f, int secs, int ms) noexcept;
void timeout( const  std::function<void()>& f, int secs) noexcept;
    

//////////////////////////////////////////////////////////////
// Timeout, cancel-able
//////////////////////////////////////////////////////////////

class Timeout
{
public:

   Timeout();
   ~Timeout();
   
   repro::Future<> after(int ms);
   void after(const std::function<void()>& f,int ms);
   void after(std::function<void()>&& f, int ms);

   void cancel();
   
private:
   std::unique_ptr<TimeoutImpl> impl_;
};


class TimeoutEx : public repro::ReproEx<TimeoutEx> {};


} // close namespaces

#endif

