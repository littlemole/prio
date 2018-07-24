#ifndef DEFINE_MOL_HTTP_SERVER_EVENTLOOP_DEF_GUARD_DEFINE_
#define DEFINE_MOL_HTTP_SERVER_EVENTLOOP_DEF_GUARD_DEFINE_

#include "priocpp/threadpool.h"

namespace prio  		{

//////////////////////////////////////////////////////////////

class Loop
{
public:

	virtual ~Loop() noexcept {};

	virtual void run() noexcept = 0;
	virtual void exit()  noexcept = 0;

	virtual void onThreadStart(std::function<void()> f) noexcept = 0;
	virtual void onThreadShutdown(std::function<void()> f) noexcept = 0;

	virtual bool isEventThread() const noexcept = 0;

	virtual repro::Future<int> signal(int s) noexcept = 0;
};


Loop& theLoop();



} // close namespaces


#endif



