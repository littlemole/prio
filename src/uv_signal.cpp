#ifdef PROMISE_USE_LIBUV

#include "priocpp/api.h"
#include "priocpp/impl/uv.h"

using namespace repro;

namespace prio      {

struct SignalImpl
{
	SignalImpl()
	{
        uv_signal_init(uvLoop().io().get(), &st_);        
        uv_handle_set_data( &st_, this);
    }

    static void signal_handler(uv_signal_t *handle, int signum)
    {
        SignalImpl* sig = (SignalImpl*)(uv_handle_get_data(handle));

        sig->p_.resolve(signum);
        
        uv_signal_stop(handle);
    }

    Promise<int> p_;

    uv_signal_t st_;
};

Signal::Signal()
	: impl_(new SignalImpl)
{}

Signal::~Signal()
{}


Future<int> Signal::when(int s)
{
	impl_->p_ = promise<int>();

    uv_signal_start( &(impl_->st_), SignalImpl::signal_handler, s);

	return impl_->p_.future();
}


void Signal::wait(Promise<int> p, int s)
{
    //NIMPL
}


void Signal::cancel()
{
    uv_signal_stop(&(impl_->st_));
}

//////////////////////////////////////////////////////////////

} // close namespaces

#endif
