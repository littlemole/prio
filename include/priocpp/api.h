#ifndef _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_API_HANDLER_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_API_HANDLER_DEF_GUARD_


#include "priocpp/common.h"
#include "priocpp/loop.h"
#include "priocpp/timeout.h"
#include "priocpp/signal.h"
#include "priocpp/url.h"
#include <tuple>


/**
 * \file api.h
 */

namespace prio      {



/**
*  \brief call std::function<void()> f on the eventloop asynchronously.
*  \param f completion handler
*
*  This function will invoke the callback asybchronously on the main event loop.
*/
void nextTick(const std::function<void()> f) noexcept;
//void nextTick(std::function<void()>&& f) noexcept;


class EventLoop 
{
public:
	EventLoop();
};

/**
* \class Libraries
* \brief initialize requires libraries
* \param Args... Library RAII loader classes to instantiate
* 
* Takes a variadic list of Library-Initialization types.
* Will initialize in a RAII style. to be used from main.
*
*/

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

/**
* \brief return a resolved promise
* \param variadic list of parameters to be passed to Promise::resolve()
*
* return a future that will be resolved with given args
* on next tick event loop
*/

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
//////////////////////////////////////////////////////////////

/**
* \brief return a rejected future
* \param promise to be rejected
*
* reject the passed promise P using Exception E
* on next tick event loop
*
*/

template<class P, class E>
auto rejected(P p, E ex) noexcept
{
	nextTick( [p,ex]{
		p.reject(ex);
	});
	return p.future();
}

/**
* \brief return a lambda then can be passed to Promise::otherwise()
*
* will reject the passed promise with the exception that gets passed to the lambda
* when otherwise() will be called
*/

/*
template<class P>
auto reject(P p)
{
	return [p](const std::exception& ex)
	{
		p.reject(ex);
	};
}
*/


/**
* \class connection_timeout_t
* \brief timeout defaults
*
*
*/

struct connection_timeout_t
{
	//! connection timeout
	int connect_timeout_s;
	//! read-write timeout
	int rw_timeout_s;
};

/**
* \class Connection
* \brief abstract base of all Connections
*
* base class for TCPConnection and SSLConnection
*/

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	typedef std::shared_ptr<Connection> Ptr;

	virtual ~Connection() {}
   
    //! asynchronously read some available char sequence from socket connection
	virtual repro::Future<Connection::Ptr, std::string> read() = 0;
    //! asynchronously read up to n available chars from socket connection
	virtual repro::Future<Connection::Ptr, std::string> read(size_t n) = 0;
	//! asynchronously write data to socket connection
	virtual repro::Future<Connection::Ptr> write(const std::string& data) = 0;

	//! check if Http2 was requested. Always false if not over SSL.
	virtual bool isHttp2Requested() { return false; }	

	//! close the connection
	virtual void close() = 0;
	//! asynchronous graceful shutdown
	virtual repro::Future<> shutdown() = 0;
	//! cancel current pending IO
	virtual void cancel() = 0;

	//! return connection timeouts for this connection
	virtual connection_timeout_t& timeouts() = 0;

	//! connect a tcp socket to hostname and given port, using HTTP
	static repro::Future<Connection::Ptr> connect(const std::string& host, int port);
	//! connect a tcp socker to hostname and given port, using HTTPS
	static repro::Future<Connection::Ptr> connect(const std::string& host, int port, SslCtx& ctx);
};

/**
* \fn connection_timeouts()
* \brief return the connection timeout defaults
*
*
*/

connection_timeout_t& connection_timeouts();

//////////////////////////////////////////////////////////////
// Ssl Context when using openssl
//////////////////////////////////////////////////////////////

/**
* \class SslCtx
* \brief Openssl context to be used for SSL/TLS
*
* to be used in main()
*/

class SslCtx
{
public:
	SslCtx();
	virtual ~SslCtx();

	//! load TLS certificates from PEM file.
	void load_cert_pem(const std::string& file);
	
	std::unique_ptr<SslCtxImpl> ctx;
};

/**
* \fn theSslCtx()
* \brief a default SslCtx()
*
* to be used if you need only one.
*/
SslCtx& theSslCtx();

//////////////////////////////////////////////////////////////
// simple socket listener
// opens and binds a server socket (tcp or ssl)
//////////////////////////////////////////////////////////////

/**
* \class Listener
* \brief  simple socket listener
*
* opens and binds a server socket (tcp or ssl)
*/

class Listener 
{
public:

	Listener();
	Listener(SslCtx& ctx);
	~Listener();

	//! stop listening
	void cancel();

	//! \brief open server socket
	//! bind a tcp/tls socket to port and start accepting incoming connections
	Listener& bind(int port);
	Listener& onAccept(std::function<void(Connection::Ptr)>);

	template<class E>
	Listener& onError(E& handler)
	{
		auto& chain = this->getErrorChain();
		repro::otherwise_chain(chain,handler);		
		return *this;
	}


private:

	std::function<bool(const std::exception_ptr&)>& getErrorChain();

  std::unique_ptr<ListenerImpl> impl_;
};

#ifndef _WIN32


/**
* \class IO 
* \brief Wait on file descriptor IO helper
*
* to integrate with 3dparty libs
*/

class IO
{
public:

	IO();
	~IO();

	//! wait for read on given socket
	repro::Future<> onRead(socket_t fd);
	//! wait for write on given socket
	repro::Future<> onWrite(socket_t fd);

	//! cancel any current waits
	void cancel();

private:

    std::unique_ptr<IOImpl> impl_;
};

#endif


/**
* \brief make an async promise based call for each element of range
* \param Iterator begin
* \param Iterator end
* \param callable F
*
* will resolve its promise once handler has been called for each element.
* or reject on the first exception thrown.
*/

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

/**
* \brief make an async promise based call for each element of range
* \param Container c
* \param callable F
*
* will resolve its promise once handler has been called for each element.
* or reject on the first exception thrown.
*/

template<class C,class F>
repro::Future<> forEach(C& c, F f )
{
	auto container = std::make_shared<C>(c);

	return forEach( container->begin(), container->end(), [container,f](typename C::value_type i)
	{
		return f(i);
	});
}

/**
* \brief ECMA6 style promise
* \param lambda with two auto params (resolve and reject)
*
* usage: 
* \code{.cpp}
*    future<int>( [](auto resolve, auto reject)
*    {
*        timeout( [resolve] () 
*        {
*            resolve(42);
*        },1);
*    })
*    .then( [](int i) 
*    {
*        // std::cout << i << std::endl;		
*    })
* \endcode
*/
/*
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
*/

} // close namespaces

#endif

