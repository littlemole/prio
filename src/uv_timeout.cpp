#ifdef PROMISE_USE_LIBUV

#include "priocpp/api.h"
#include "priocpp/impl/uv.h"

using namespace repro;

namespace prio      {


struct TimeoutImpl
{
	TimeoutImpl()
	{
        uv_timer_init(uvLoop().io().get(), &timer);
        uv_handle_set_data( &timer, this);        
    }

    static void callback(uv_timer_t *handle)
    {
        TimeoutImpl* t = (TimeoutImpl*)(uv_handle_get_data(handle));

        if( t->f_)
        {
            t->f_();
        }
        else
        {
            t->p_.resolve();
        }
    }


	uv_timer_t  timer;

    Promise<> p_;

    std::function<void()> f_;
};

Timeout::Timeout()
: impl_(nullptr)
{
	LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(prio::timeouts);
}

Timeout::~Timeout()
{
	cancel();
	LITTLE_MOLE_RELEASE_DEBUG_REF_CNT(timeouts);	
}

Future<> Timeout::after(int ms)
{
    impl_.reset(new TimeoutImpl);
	impl_->p_ = promise<>();
	cancel();

    uv_timer_start(&(impl_->timer), TimeoutImpl::callback, ms, 0);

	return p.future();
}

void Timeout::after( const std::function<void()>& f, int ms)
{
    impl_.reset(new TimeoutImpl);
    impl_->f_ = f;
	cancel();

    uv_timer_start(&(impl_->timer), TimeoutImpl::callback, ms, 0);

}

void Timeout::after( std::function<void()>&& f, int ms)
{
    impl_.reset(new TimeoutImpl);
    impl_->f_ = f;
	cancel();

    uv_timer_start(&(impl_->timer), TimeoutImpl::callback, ms, 0);
}

void Timeout::cancel()
{
	if(impl_ )
	{
        uv_timer_stop(&(impl_->timer);        
	}
    impl_.reset();
}



//////////////////////////////////////////////////////////////

} // close namespaces

#endif
