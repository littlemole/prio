#ifndef _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_SIGNAL_HANDLER_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_SIGNAL_HANDLER_DEF_GUARD_


#include "priocpp/common.h"


namespace prio      {


//////////////////////////////////////////////////////////////
// create signal handler
//////////////////////////////////////////////////////////////

Future<int> signal(int s) noexcept;


class Signal
{
public:

	Signal();
	~Signal();

	Future<int> when(int s);

    void cancel();

private:

	void wait(repro::Promise<int> p, int s);

	std::unique_ptr<SignalImpl> impl_;
};



} // close namespaces

#endif

