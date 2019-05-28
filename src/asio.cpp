#ifdef PROMISE_USE_BOOST_ASIO

#include <fcntl.h>

#include "priocpp/impl/asio.h"
#include "priocpp/connection.h"
#include "priocpp/ssl_connection.h"
#include <boost/version.hpp>

using namespace repro;

namespace prio      {



void nextTick(const std::function<void()> f) noexcept
{
	asioLoop().io().post(std::move(f));
/*	
	timeout([f]()
	{
		f();

	}, 0, 1);
*/
};

/*
void nextTick(std::function<void()>&& f) noexcept
{
//	asioLoop().io().post(std::move(f));

	timeout([f]()
	{
		f();

	}, 0, 1);
};
*/
Future<> nextTick() noexcept
{
	auto p = promise();
/*
	timeout([p]() 
	{
		p.resolve();

	}, 0, 1);
*/
	asioLoop().io().post( [p]()
	{
		p.resolve();
	});
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
	: acceptor(asioLoop().io())
{}

ListenerImpl::~ListenerImpl()
{}


Future<ConnectionPtr> ListenerImpl::bind( int port )
{
	auto p = promise<Connection::Ptr>();

	acceptor = boost::asio::ip::tcp::acceptor(
		asioLoop().io() //, 
//		boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)
	);

//	boost::asio::ip::tcp::acceptor acceptor(io_service);
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	acceptor.open(endpoint.protocol());
	acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen();

	accept_handler(p);

	return p.future();
}

void ListenerImpl::cancel()
{
	acceptor.cancel();
}



TcpListenerImpl::TcpListenerImpl()
	: socket( asioLoop().io()){}

TcpListenerImpl::~TcpListenerImpl()
{
	socket.close();
}

void TcpListenerImpl::accept_handler(Promise<Connection::Ptr> p)
{
	auto impl = new TcpConnectionImpl;
	Connection::Ptr ptr( new TcpConnection(impl) );		

	acceptor.async_accept(
		impl->socket,
		[this,impl,p,ptr](const boost::system::error_code& error)
		{
			if(is_io_cancelled(error))
			{	
				return;
			}

#if BOOST_VERSION < 106599
			boost::asio::ip::tcp::socket::non_blocking_io non_blocking_io(true);
			impl->socket.io_control(non_blocking_io);
#else
			impl->socket.non_blocking(true);
#endif
			p.resolve(ptr);
			accept_handler(p);
		}
	);
}

SslListenerImpl::SslListenerImpl(SslCtx& ssl)
	: socket( asioLoop().io(), ssl.ctx->ssl ),
	ctx(ssl)
{}

SslListenerImpl::~SslListenerImpl()
{
	if(socket.lowest_layer().is_open())
		socket.lowest_layer().close();
}

void SslListenerImpl::accept_handler(Promise<Connection::Ptr> p)
{
	auto impl = new SslConnectionImpl(ctx);
	Connection::Ptr ptr( new SslConnection(impl) );		

	acceptor.async_accept(
		impl->socket.lowest_layer(),
		[this,impl,ptr,p](const boost::system::error_code& error)
		{
			if(is_io_cancelled(error))
			{	
				return;
			}

			if(error)
			{
				SslListenerImpl::accept_handler(p);
				return;
			}
#if BOOST_VERSION < 106599
			boost::asio::ip::tcp::socket::non_blocking_io non_blocking_io(true);
			impl->socket.lowest_layer().io_control(non_blocking_io);
#else
			impl->socket.lowest_layer().non_blocking(true);
#endif


			impl->socket.async_handshake(
				boost::asio::ssl::stream_base::server,
				[this,p,ptr](const boost::system::error_code& error)
				{
					if ( error  )
					{	
						p.reject(Ex("SSL HANDSHAKE FAILED"));
						return;
					}

					p.resolve(ptr);
					SslListenerImpl::accept_handler(p);
				}
			);
		}
	);
}

#ifndef _WIN32

IOImpl::IOImpl()
	: sd_(asioLoop().io())
{}

IOImpl::~IOImpl()
{
	cancel();
}

Future<> IOImpl::onRead(socket_t fd)
{
	auto p = promise();

	cancel();
	sd_.assign(fd);

	sd_.async_read_some(
		boost::asio::null_buffers(), 
		[this,p] ( boost::system::error_code ec, std::size_t bytes_transferred  )
		{
			handle_callback(p,ec);
		}
	);	

	return p.future();
}


Future<> IOImpl::onWrite(socket_t fd)
{
	auto p = promise();

	cancel();
	sd_.assign(fd);

	sd_.async_write_some(
		boost::asio::null_buffers(), 
		[this,p] ( boost::system::error_code ec, std::size_t bytes_transferred  )
		{
			handle_callback(p,ec);
		}
	);	

	return p.future();
}

void IOImpl::handle_callback( const Promise<>& p, boost::system::error_code error )
{
	if(error)
	{
		if(is_io_cancelled(error))
		{
			return;
		}

		p.reject(Ex(std::string("wait failed: ") + error.message()));
	}
	else
	{
		p.resolve( );
	}
}

void IOImpl::cancel()
{
	if(sd_.is_open())
	{
		sd_.cancel();
	}
	sd_.release();
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

bool is_io_cancelled(const boost::system::error_code& error)
{
#ifdef _WIN32

#define E_ERROR_OPERATION_ABORTED          995L

	if (error.value() == E_ERROR_OPERATION_ABORTED)
	{
		return true;
	}
#endif

	if (error.value() == boost::system::errc::operation_canceled)
	{
		return true;
	}

	return false;
}

} // close namespaces

#endif



