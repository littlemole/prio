#ifdef PROMISE_USE_LIBEVENT

#include <fcntl.h>
#include "reprocpp/debug.h"
#include "priocpp/impl/event.h"
#include "priocpp/connection.h"
#include "priocpp/ssl_connection.h"
#include "priocpp/task.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/dns.h>
#include <event2/util.h>


#ifdef _WIN32
#include "Ws2tcpip.h"
#define close_socket ::closesocket
#define SHUT_RDWR SD_SEND 
#else
#define close_socket ::close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using namespace repro;

namespace prio      {


int check_err_code(SSL* ssl, int len, int want);

EventLoop::EventLoop()
{
#ifdef _WIN32
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
		wVersionRequested = MAKEWORD(2, 2);

		err = WSAStartup(wVersionRequested, &wsaData);
		evthread_use_windows_threads();
#else 
		evthread_use_pthreads();
#endif
}

bool would_block()
{
#ifndef _WIN32
	return errno == EINPROGRESS;
#else
	return ::WSAGetLastError() == WSAEWOULDBLOCK;
#endif
}


void set_non_blocking(socket_t fd)
{
#ifndef _WIN32
	// set non blocking
	int flags;
	if ( -1 == (flags = fcntl(fd, F_GETFL, 0)) )
	{
		flags = 0;        
	}
	flags = flags | O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);  
#else
	u_long iMode = 1;
	ioctlsocket(fd, FIONBIO, &iMode);
#endif
}

Event::Ptr onEvent(socket_t fd, short what)
{
	return eventLoop().on(fd,what);
}


void Event::event_handler(socket_t fd, short what, void* arg)
{
	Event* e = (Event*)arg;
	e->cb_(fd,what);
}

Event::Ptr Event::create(::event_base* loop, socket_t fd, short what) noexcept
{
	Ptr that;
	try
	{
		that = std::make_shared<Event>();
		auto e = ::event_new(
			loop,
			fd,
			what,
			&event_handler,
			(void*)(that.get())
		);
		that->e = reinterpret_cast<event*>(e);
	}
	catch(...)
	{
		std::terminate();
	}
	return that;
}

Event::Event() noexcept
{
	LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(events);

	e = nullptr;
	cb_ = [](socket_t fd, short what){};
}

Event::~Event()
{
	LITTLE_MOLE_RELEASE_DEBUG_REF_CNT(events);
	cancel();
}

void Event::cancel() noexcept
{
	if(e)
	{
		::event_del(e);
		::event_free(e);
		e = nullptr;
	}
	cb_ = [](socket_t fd, short what){};
}

Event::Ptr Event::add(int secs, int ms) noexcept
{
	timeval tv = {secs,ms};
	Ptr ptr = shared_from_this();
	event_add((::event*)e,&tv);
	return ptr;
}

Event::Ptr Event::add() noexcept
{
	Ptr ptr = shared_from_this();
	event_add(e,(const timeval*)0);
	return ptr;
}


void Event::activate(int flags)
{
	::event_active(e,flags,0);
}

void Event::dispose() noexcept
{
	cancel();
}

Event::Ptr Event::callback( const callback_t& f)
{
	cb_ = std::move(f);
	return shared_from_this();
}


Event::Ptr Event::callback(callback_t&& f)
{
	cb_ = std::move(f);
	return shared_from_this();
}

event* Event::handle()
{
	return e;
}


void nextTick( const std::function<void()> f) noexcept
{
	auto e = Event::create(eventLoop().base());
	e->callback( [e,f] (socket_t fd, short what) 
	{
		auto tmp = std::move(f);
		e->dispose();
		tmp();
	})
	->add()
	->activate();
};

/*
void nextTick(std::function<void()>&& f) noexcept
{
	auto e = Event::create(eventLoop().base());
	e->callback([e, f](socket_t fd, short what)
	{
		auto tmp = std::move(f);
		e->dispose();
		tmp();
	})
	->add(0,1);
};
*/
Future<> nextTick() noexcept
{
	auto p = promise();

	auto e = Event::create(eventLoop().base());
	e->callback( [e,p] (socket_t fd, short what) 
	{
		auto tmp = std::move(p);
		e->dispose();
		tmp.resolve();
	})
	->add(0,1);

	return p.future();
};


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
	socket_t fd;

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	int yes = 1;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		throw Ex("create socket for bind failed");;
	}


	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes,
		sizeof(int)) == -1)
	{
		throw Ex("setsockopt failed");
	}

	if( ::bind(fd, (sockaddr *)&sin, sizeof(sin)) )
	{
		close_socket(fd);
		throw Ex("bind failed");;
	}

	set_non_blocking(fd);

	LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(sockets);

	impl_->fd = fd;

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
	: promise_(promise<Connection::Ptr>())
{}

ListenerImpl::~ListenerImpl()
{}


Future<ConnectionPtr> ListenerImpl::bind( int port )
{
	if (::listen(fd, SOMAXCONN ) == -1) 
	{
		throw Ex("server: failed to listen");
	}   

	accept_handler(promise_);

	return promise_.future();
}

void ListenerImpl::cancel()
{
	if(e)
	{
		e->cancel();
	}
}



TcpListenerImpl::TcpListenerImpl()
{}

TcpListenerImpl::~TcpListenerImpl()
{
	close_socket(fd);
	LITTLE_MOLE_RELEASE_DEBUG_REF_CNT(sockets);
	if(e)
		e->cancel();
}

void TcpListenerImpl::accept_handler(Promise<Connection::Ptr> p)
{
	e = onEvent(fd, EV_READ|EV_PERSIST);

	e->callback( [p](socket_t fd, short what )
	{
		try
		{
			auto impl = new TcpConnectionImpl;
			Connection::Ptr ptr( new TcpConnection(impl) );		

			sockaddr_storage their_addr; 
			socklen_t sin_size = sizeof(their_addr);

			socket_t client = ::accept(fd, (struct sockaddr *)&their_addr, &sin_size);

			if (client == -1) {
				throw Ex("server: failed to accept socket");
			}
			
			LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(sockets);

			set_non_blocking(client);

			impl->fd = client;

			p.resolve(ptr);
		}
		catch(...)
		{
			p.reject(std::current_exception());
		}
	});
	e->add();

}

SslListenerImpl::SslListenerImpl(SslCtx& ssl)
	: ctx(ssl)
{}

SslListenerImpl::~SslListenerImpl()
{
	close_socket(fd);
	LITTLE_MOLE_RELEASE_DEBUG_REF_CNT(sockets);
	if(e)
		e->cancel();
}

void do_ssl_accept(Promise<Connection::Ptr> p, Connection::Ptr ptr, socket_t sock_fd, SSL* ssl, short what, SSL_CTX* ctx)
{
	Event::Ptr e  = onEvent(sock_fd,what);
	e->callback( [e,p,ptr,sock_fd,ssl,what,ctx](socket_t fd, short w)
	{
		int r = ::SSL_accept(ssl);
		int s = check_err_code(ssl,r,what);

		if ( s == SSL_ERROR_NONE )
		{
			p.resolve(ptr);
			e->dispose();
			return;
		}

		if ( s < 0 )
		{
			p.reject(Ex("SSL accept failed"));
			e->dispose();
			return;
		}

		do_ssl_accept(p,ptr,sock_fd,ssl,s,ctx);
		e->dispose();
	})
	->add();
}

void SslListenerImpl::accept_handler(Promise<Connection::Ptr> p)
{
	e = onEvent(fd, EV_READ|EV_PERSIST);

	e->callback( [this,p](socket_t fd, short what )
	{
		auto impl = new SslConnectionImpl;
		Connection::Ptr ptr( new SslConnection(impl) );		

		try
		{
			sockaddr_storage their_addr; 
			socklen_t sin_size = sizeof(their_addr);

			socket_t client = ::accept(fd, (struct sockaddr *)&their_addr, &sin_size);

			if (client == -1) {
				throw Ex("server: failed to accept socket");
			}
			
			LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(sockets);

			set_non_blocking(client);

			SSL* ssl = SSL_new(ctx.ctx->ctx);
			SSL_set_fd(ssl,(int)client);
			SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

			impl->fd = client;
			impl->ssl = ssl;

			int r = ::SSL_accept(ssl);
			int s = check_err_code(ssl,r,what);
			if ( s == SSL_ERROR_NONE)
			{
				p.resolve(ptr);
				return;
			}
			if ( s < 0)
			{
				p.reject(Ex("SSL accept failed"));
				return;
			}		
			do_ssl_accept(p,ptr,client,ssl,s,ctx.ctx->ctx);
		}
		catch(...)
		{
			p.reject(std::current_exception());
		}
	});
	e->add();
}

IOImpl::IOImpl()
{}

IOImpl::~IOImpl()
{
	cancel();
}

Future<> IOImpl::onRead(socket_t fd)
{
	auto p = promise();

	e = onEvent(fd, EV_READ);
	e->callback( [p](socket_t fd, short what )
	{
		p.resolve();
	})
	->add();

	return p.future();
}


Future<> IOImpl::onWrite(socket_t fd)
{
	auto p = promise();

	e = onEvent(fd, EV_WRITE);
	e->callback( [p](socket_t fd, short what )
	{		
		p.resolve();
	})
	->add();

	return p.future();
}

void IOImpl::cancel()
{
	if(e)
	{
		e->cancel();
		e.reset();
	}
}


IO::IO()
	: impl_(new IOImpl)
{}

IO::~IO()
{
	cancel();
}

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


Resolver& dnsResolver()
{
	static Resolver r;
	return r;
}

Resolver::Resolver()
{}

Resolver::~Resolver()
{}


Future<socket_t> Resolver::connect(const std::string& host, int port)
{
	auto p = promise<socket_t>();

	prio::task( [host,port]() -> socket_t
	{

		struct addrinfo hints;
		struct addrinfo *result, *rp;
		int s;

		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;    /* Allow IPv4 */
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_CANONNAME;
		hints.ai_protocol = 0;          /* Any protocol */

		s = getaddrinfo(host.c_str(), NULL, &hints, &result);
		if (s != 0) 
		{
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
			return -1;
		}

		for (rp = result; rp != NULL; rp = rp->ai_next) 
		{
			struct sockaddr_in *sin = (struct sockaddr_in *)(rp->ai_addr);

// to show DNS resolves:
//			char * c = inet_ntoa(sin->sin_addr);
//			std::cout << "DNS: " << c << std::endl;

			sin->sin_port = htons(port);	

			socket_t fd;
			// create the socket
			fd = ::socket( AF_INET,SOCK_STREAM, IPPROTO_TCP);
			LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(sockets);

			set_non_blocking(fd);

			// connect
			int r = ::connect( fd, (const sockaddr*)sin, sizeof(sockaddr_in));
			freeaddrinfo(result);

			if(r< 0 && !would_block())
			{
				return -1;
			}
			
			return fd;
		}
		return -1;
	})
	.then([p](socket_t fd)
	{
		if(fd == -1)
		{
			p.reject(Ex("host not found"));
			return;
		}

		IO* io = new IO;

		io
		->onWrite(fd)
		.then([p, io, fd]()
		{
			p.resolve(fd);
			delete io;
		});
	})
	.otherwise([p](const std::exception& ex)
	{
		p.reject(ex);
	});

	return p.future();
}



} // close namespaces

#endif



