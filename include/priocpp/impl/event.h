#ifndef DEFINE_MOL_HTTP_SERVER_EVENTLOOP_ASIOIMPL_DEF_GUARD_DEFINE_
#define DEFINE_MOL_HTTP_SERVER_EVENTLOOP_ASIOIMPL_DEF_GUARD_DEFINE_

#ifdef PROMISE_USE_LIBEVENT

#include <fcntl.h>
#include <event2/event.h>
#include <event2/dns.h>

#include "reprocpp/debug.h"
#include "priocpp/api.h"

namespace prio  		{


LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(events);
LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(sockets);
LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(timeouts);


bool would_block();

//////////////////////////////////////////////////////////////

}

typedef struct bio_st BIO;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;

namespace prio      {


LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(events);


class Event : public std::enable_shared_from_this<Event>
{
public:

	typedef std::shared_ptr<Event> Ptr;
	typedef std::function<void(socket_t fd,short what)> callback_t;

	Event() noexcept;
	~Event();

	static Event::Ptr create(::event_base* loop, socket_t fd = -1, short what=0) noexcept;

	Event::Ptr callback( const callback_t& f);
	Event::Ptr callback( callback_t&& f);
	Event::Ptr add(int secs, int ms) noexcept;
	Event::Ptr add() noexcept;

	void activate(int flags = 0);

	void cancel() noexcept;
	void dispose() noexcept;

	::event* handle();

private:

	callback_t cb_;

	Event(const Event& rhs) = delete;
	Event(Event&& rhs) = delete;

	Event& operator=(const Event& rhs) = delete;
	Event& operator=(Event&& rhs) = delete;

	static void event_handler(socket_t fd, short what, void* arg);

	event* e;
};

Event::Ptr onEvent(socket_t fd, short what);

class LibEventLoop : public Loop
{
public:

	LibEventLoop();
	virtual ~LibEventLoop() noexcept;

	virtual void run() noexcept;
	virtual void exit()  noexcept;

	virtual void onThreadStart(std::function<void()> f) noexcept;
	virtual void onThreadShutdown(std::function<void()> f) noexcept;

	virtual bool isEventThread() const noexcept;

	virtual Future<int> signal(int s) noexcept;

	Event::Ptr on(socket_t fd, short what) const noexcept;

	event_base* base() { return base_; }

private:

	LibEventLoop(const LibEventLoop& rhs) = delete;
	LibEventLoop(LibEventLoop&& rhs) = delete;
	LibEventLoop& operator=(const LibEventLoop& rhs) = delete;
	LibEventLoop& operator=(LibEventLoop&& rhs) = delete;

	std::thread::id eventThreadId_;
	std::vector<std::shared_ptr<Signal>> signals_;

	event_base* base_;
};


LibEventLoop& eventLoop();


class TcpConnectionImpl
{
public:

	TcpConnectionImpl();

	socket_t fd;

	std::shared_ptr<Event> e_read;
	std::shared_ptr<Event> e_write;
};

class SslConnectionImpl
{
public:

	SslConnectionImpl();
	~SslConnectionImpl();

	socket_t fd;
	SSL* ssl;

	std::shared_ptr<Event> e_;

	bool isHttp2Requested();	
};


struct SslCtxImpl
{
	SslCtxImpl();
	~SslCtxImpl();
	
	void loadKeys( const std::string& keyfile );
	void enableHttp2();
	void enableHttp2Client();
	
	SSL_CTX* ctx;
};



struct ListenerImpl
{	
	ListenerImpl();
	virtual ~ListenerImpl();

    virtual void accept_handler(repro::Promise<Connection::Ptr> p) = 0;

	Future<ConnectionPtr> bind( int port );
	void cancel();

	socket_t fd;
	Event::Ptr e;
	repro::Promise<Connection::Ptr> promise_; 
};


struct IOImpl
{
	IOImpl();
	~IOImpl();

	Future<> onRead(socket_t fd);
	Future<> onWrite(socket_t fd);

	void cancel();

	Event::Ptr e;
};



struct TcpListenerImpl : public ListenerImpl
{
	TcpListenerImpl();	
	~TcpListenerImpl();

    void accept_handler(repro::Promise<Connection::Ptr> p);
};



struct SslListenerImpl : public ListenerImpl
{
	SslListenerImpl(SslCtx& ssl);	
	~SslListenerImpl();

    void accept_handler(repro::Promise<Connection::Ptr> p);

	SslCtx& ctx;
};


class Resolver
{
public:

	Resolver();
	~Resolver();

	Future<int> connect(const std::string& host, int port);

private:

	struct ResolveData
	{
		repro::Promise<sockaddr_in*> p;
	};

	Future<sockaddr_in*> resolve(const std::string host);

	static void callback(int errcode, struct evutil_addrinfo *addr, void *ptr);

	evdns_base *dnsbase_;
};

Resolver& dnsResolver();


} // close namespaces

#endif 
#endif



