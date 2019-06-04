#ifdef PROMISE_USE_LIBUV

#include "priocpp/api.h"
#include "priocpp/impl/uv.h"

using namespace repro;

namespace prio      {


std::thread::id null_thread;


Loop& theLoop()
{
	return uvLoop();
}



UVLoop& uvLoop()
{
	static UVLoop loop;
	return loop;
}


UVLoop::UVLoop()
{
	eventThreadId_ = std::this_thread::get_id();
    io_ = std::shared_ptr<uv_loop_t>( new uv_loop_t);

    uv_loop_init(io_.get());
}

UVLoop::~UVLoop()  noexcept
{
}

void UVLoop::run()  noexcept
{
	thePool().start();
    uv_run(io_.get(), UV_RUN_DEFAULT);
    uv_loop_close(io_.get());
	signals_.clear();
}

void UVLoop::exit()  noexcept
{
	for( std::shared_ptr<Signal> s : signals_)
	{
		s->cancel();
	}
	nextTick( [this] {
		thePool().stop();
        uv_stop(io_.get());
	});
}


void UVLoop::onThreadStart(std::function<void()> f) noexcept
{
	thePool().onThreadStart = f;
}

void UVLoop::onThreadShutdown(std::function<void()> f) noexcept
{
	thePool().onThreadShutdown = f;
}

bool UVLoop::isEventThread() const noexcept
{
	return eventThreadId_ == std::this_thread::get_id();
}

Future<int> UVLoop::signal(int s) noexcept
{
	auto p = promise<int>();

	auto signal = std::make_shared<Signal>();
	signals_.push_back(signal);

	signal->when(s)
	.then( [p]( int signal_number)
	{
		p.resolve(signal_number);
	});		

	return p.future();
}

//////////////////////////////////////////////////////////////

} // close namespaces

#endif
