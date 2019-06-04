#ifndef _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_API_HANDLER_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_API_HANDLER_DEF_GUARD_


#include "priocpp/common.h"
#include "priocpp/loop.h"
#include "priocpp/timeout.h"
#include "priocpp/signal.h"
#include "priocpp/url.h"
#include <tuple>

namespace prio      {



/**
*  \fn nextTick(f)
*  \brief call std::function<void()> f on the eventloop asynchronously.
*  \param f completion handler
*
*  This function will invoke the callback asybchronously on the main event loop.
*/
void nextTick(const std::function<void()> f) noexcept;


// init io loop libraries, ie make libevent threadsafe.
class EventLoop 
{
public:
	EventLoop();
};


template<class T,class ... Args>
class Libraries : public Libraries<Args...>
{
private:
	T lib_;
};

template<class T>
class Libraries<T> 
{
private:
	T lib_;
};

//////////////////////////////////////////////////////////////
// return a promise that will be resolved with given args
// on next tick event loop
//////////////////////////////////////////////////////////////

template<class ...VArgs>
auto resolved(VArgs ... vargs) noexcept
{
	auto p = repro::promise<VArgs...>();
	nextTick( [p,vargs...]{
		p.resolve(vargs...);
	});
	return p.future();
}

//////////////////////////////////////////////////////////////
// reject the passed promise P using Exception E
// on next tick event loop
//////////////////////////////////////////////////////////////
template<class P, class E>
auto rejected(P p, E ex) noexcept
{
	nextTick( [p,ex]{
		p.reject(ex);
	});
	return p.future();
}


template<class P>
auto reject(P p)
{
	return [p](const std::exception& ex)
	{
		p.reject(ex);
	};
}


//////////////////////////////////////////////////////////////
// abstract Connection interface.
// known subclasses are TcpConnection and SslConnection
//////////////////////////////////////////////////////////////



struct connection_timeout_t
{
	int connect_timeout_s;
	int rw_timeout_s;
};


class Connection : public std::enable_shared_from_this<Connection>
{
public:
	typedef std::shared_ptr<Connection> Ptr;

	virtual ~Connection() {}
   
	virtual repro::Future<Connection::Ptr, std::string> read() = 0;
	virtual repro::Future<Connection::Ptr, std::string> read(size_t n) = 0;
	virtual repro::Future<Connection::Ptr> write(const std::string& data) = 0;

	virtual bool isHttp2Requested() { return false; }	

	virtual void close() = 0;
	virtual repro::Future<> shutdown() = 0;
	virtual void cancel() = 0;

	virtual connection_timeout_t& timeouts() = 0;


	static repro::Future<Connection::Ptr> connect(const std::string& host, int port);
	static repro::Future<Connection::Ptr> connect(const std::string& host, int port, SslCtx& ctx);
};


connection_timeout_t& connection_timeouts();

//////////////////////////////////////////////////////////////
// Ssl Context when using openssl
//////////////////////////////////////////////////////////////

class SslCtx
{
public:
	SslCtx();
	virtual ~SslCtx();

	void load_cert_pem(const std::string& file);
	
	std::unique_ptr<SslCtxImpl> ctx;
};

SslCtx& theSslCtx();

//////////////////////////////////////////////////////////////
// simple socket listener
// opens and binds a server socket (tcp or ssl)
//////////////////////////////////////////////////////////////

class Listener 
{
public:

	Listener();
	Listener(SslCtx& ctx);
	~Listener();

	void cancel();

	repro::Future<ConnectionPtr> bind(int port);

private:
  std::unique_ptr<ListenerImpl> impl_;
};

#ifndef _WIN32
//////////////////////////////////////////////////////////////
// Wait on file descriptor IO helper
// to integrate with 3dparty libs
//////////////////////////////////////////////////////////////

class IO
{
public:

	IO();
	~IO();

	repro::Future<> onRead(socket_t fd);
	repro::Future<> onWrite(socket_t fd);

	void cancel();

private:

    std::unique_ptr<IOImpl> impl_;
};

#endif



template<class I,class F>
repro::Future<> forEach( I begin, I end, F f )
{
	auto p = repro::promise<>();

	if ( begin == end)
	{
		prio::nextTick([p]()
		{
			p.resolve();
		});
		
		return p.future();
	}

	I step = begin;
	step++;

	f(*begin)
	.then([step,end,f]()
	{
		return forEach(step,end,f);
	})
	.then([p]()
	{
		p.resolve();
	})
	.otherwise(reject(p));

	return p.future();
}

template<class C,class F>
repro::Future<> forEach(C& c, F f )
{
	auto container = std::make_shared<C>(c);

	return forEach( container->begin(), container->end(), [container,f](typename C::value_type i)
	{
		return f(i);
	});
}

/// ES6 Ecma Script promise style (syntactic sugar)
template<class ...Args,class T>
repro::Future<Args...> future( T cb )    
{
    auto p = repro::promise<Args...>();

		nextTick( [p,cb]()
		{
				try
				{
					auto resolve = [p]( Args... args) 
					{
						p.resolve(args...);
					};

					auto reject = [p]( const std::exception& ex) 
					{
						p.reject(ex);
					};

						cb(resolve,reject);
				}
				catch(...)
				{
					auto ex = std::current_exception();
					p.reject(ex);
				}
		});

    return p.future();
}

} // close namespaces

#endif

