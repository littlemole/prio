#ifdef PROMISE_USE_LIBUV

#include <fcntl.h>

#include "priocpp/impl/uv.h"
#include "priocpp/connection.h"
#include "priocpp/ssl_connection.h"

using namespace repro;

namespace prio      {

struct async_handle
{
    async_handle()
    {
        uv_async_init( uvLoop().io().get(), &at_, async_handle::callback);
        uv_handle_set_data( &at_, this);        
    }

    uv_async_t at_;
    std::function<void()> f_;
    Promise<> p_;

    static void callback(uv_async_t* handle)
    {
        async_handle* t = (async_handle*)(uv_handle_get_data(handle));

        if( t->f_)
        {
            t->f_();
        }
        else
        {
            t->p_.resolve();
        }
        delete t;
    }
};

void nextTick(const std::function<void()> f) noexcept
{
    async_handle* a = new async_handle();
    a->f_ = f;

    uv_async_send(&(a->at_));
};

Future<> nextTick() noexcept
{
    auto p = promise();

    async_handle* a = new async_handle();
    a->p_ = p;

    uv_async_send(&(a->at_));

    return p.future();
};



EventLoop::EventLoop()
{
#ifdef _WIN32
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD(2, 2);

		err = WSAStartup(wVersionRequested, &wsaData);
#endif
}


Listener::Listener()
  : impl_(new TcpListenerImpl) 
{
}

Listener::Listener(SslCtx& ctx)
  : impl_(new SslListenerImpl(ctx)) 
{
}

Listener::~Listener()
{}


Future<ConnectionPtr> Listener::bind( int port )
{
	return impl_->bind(port);
}


void Listener::cancel()
{
	return impl_->cancel();
}




SslCtx& theSslCtx()
{
	static SslCtx ctx;
	return ctx;
}



ListenerImpl::ListenerImpl()
{
    uv_tcp_init(uvLoop().io().get(), &acceptor);    
    uv_handle_set_data(&acceptor,this);

    p = promise<ConnectionPtr>();
}

ListenerImpl::~ListenerImpl()
{
}

void ListenerImpl::on_new_connection(uv_stream_t *server, int status) 
{
    ListenerImpl* impl = (ListenerImpl*)(uv_handle_get_data(server));

    uv_tcp_t* client = new uv_tcp_t;

    uv_tcp_init(uvLoop().io().get(), client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) 
    {
        impl->accept_handler(client);
    }
}



Future<ConnectionPtr> ListenerImpl::bind( int port )
{
    struct sockaddr_in addr;

    uv_ip4_addr("0.0.0.0", port, &addr);

    uv_tcp_bind(&acceptor, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*) &acceptor, DEFAULT_BACKLOG, &ListenerImpl::on_new_connection);
    if (r) 
    {
        throw repro::Ex("failed to bind");
    }

	return p.future();
}

void ListenerImpl::cancel()
{
    uv_read_stop(&acceptor);
}

void ListenerImpl::on_close(uv_handle_t* handle)
{

}

TcpListenerImpl::TcpListenerImpl()

TcpListenerImpl::~TcpListenerImpl()
{
    uv_close(&acceptor, &ListenerImpl::on_close);
}

void TcpListenerImpl::accept_handler(uv_tcp_t* c)
{
	auto impl = new TcpConnectionImpl;
    impl->con.reset(c);

	Connection::Ptr ptr( new TcpConnection(impl) );		

    p.resolve(ptr);
}

SslListenerImpl::SslListenerImpl(SslCtx& ssl)
	: ctx(ssl)
{}

SslListenerImpl::~SslListenerImpl()
{
    uv_close(&acceptor, &ListenerImpl::on_close);
}

void SslListenerImpl::accept_handler(Promise<Connection::Ptr> p)
{
	auto impl = new SslConnectionImpl(ctx);
    impl->con.reset(c);

	Connection::Ptr ptr( new SslConnection(impl) );		

    // TODO SSL handshake!

    p.resolve(ptr);
}




#ifndef _WIN32

IOImpl::IOImpl()
	: sd_(asioLoop().io())
{
}

IOImpl::~IOImpl()
{
	cancel();
}

void IOImpl::on_io_callback(uv_poll_t* handle, int status, int events)
{
    IOImpl* io = (IOImpl*)(uv_handle_get_data(handle));

    if(status < 0)
    {
        io->p_.reject();
        return;
    }
    io->p_.resolve();
}


Future<> IOImpl::onRead(socket_t fd)
{
	p_ = promise();

    uv_poll_init_socket(uvLoop().io().get(), &poll_, fd );
    uv_handle_set_data( &poll_, this);

	cancel();

    uv_poll_start(&poll_, UV_READABLE, &IOImpl::on_io_callback);

	return p.future();
}


Future<> IOImpl::onWrite(socket_t fd)
{
	p_ = promise();

    uv_poll_init_socket(uvLoop().io().get(), &poll_, fd );
    uv_handle_set_data( &poll_, this);

	cancel();

    uv_poll_start(&poll_, UV_WRITABLE , &IOImpl::on_io_callback);

	return p.future();
}


void IOImpl::cancel()
{
    uv_poll_stop(&poll_);
}


IO::IO()
	: impl_(new IOImpl)
{}

IO::~IO()
{}

Future<> IO::onRead(socket_t fd)
{
	return impl_->onRead(fd);
}

Future<> IO::onWrite(socket_t fd)
{
	return impl_->onWrite(fd);
}

void IO::cancel()
{
	if(impl_)
		impl_->cancel();
}

#endif



} // close namespaces

#endif



