#ifndef _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_API_HANDLER_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_API_HANDLER_DEF_GUARD_


#include "priocpp/common.h"
#include "priocpp/loop.h"
#include "priocpp/timeout.h"
#include "priocpp/signal.h"
#include "priocpp/url.h"


namespace prio      {



/**
*  \fn nextTick(f)
*  \brief call std::function<void()> f on the eventloop asynchronously.
*  \param f completion handler
*
*  This function will invoke the callback asybchronously on the main event loop.
*/
void nextTick(const std::function<void()>& f) noexcept;
void nextTick(std::function<void()>&& f) noexcept;


// init io loop libraries, ie make libevent threadsafe.
void init();


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
   
	virtual Future<Connection::Ptr, std::string> read() = 0;
	virtual Future<Connection::Ptr, std::string> read(size_t n) = 0;
	virtual Future<Connection::Ptr> write(const std::string& data) = 0;

	virtual bool isHttp2Requested() { return false; }	

	virtual void close() = 0;
	virtual Future<> shutdown() = 0;
	virtual void cancel() = 0;

	virtual connection_timeout_t& timeouts() = 0;


	static Future<Connection::Ptr> connect(const std::string& host, int port);
	static Future<Connection::Ptr> connect(const std::string& host, int port, SslCtx& ctx);
};


connection_timeout_t& connection_timeouts();

//////////////////////////////////////////////////////////////
// Ssl Context when using openssl
//////////////////////////////////////////////////////////////

class SslCtx
{
public:
	SslCtx();
	~SslCtx();

	void load_cert_pem(const std::string& file);
	void enableHttp2();
	void enableHttp2Client();
	
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

	Future<ConnectionPtr> bind(int port);

private:
  std::unique_ptr<ListenerImpl> impl_;
};

//////////////////////////////////////////////////////////////
// Wait on file descriptor IO helper
// to integrate with 3dparty libs
//////////////////////////////////////////////////////////////

class IO
{
public:

	IO();
	~IO();

	Future<> onRead(socket_t fd);
	Future<> onWrite(socket_t fd);

	void cancel();

private:

    std::unique_ptr<IOImpl> impl_;
};


} // close namespaces

#endif

