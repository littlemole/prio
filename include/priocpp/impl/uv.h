#ifndef DEFINE_MOL_HTTP_SERVER_EVENTLOOP_UVIMPL_DEF_GUARD_DEFINE_
#define DEFINE_MOL_HTTP_SERVER_EVENTLOOP_UVIMPL_DEF_GUARD_DEFINE_

#ifdef PROMISE_USE_LIBUV

#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

namespace prio	 	{

LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(events);
LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(sockets);
LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(timeouts);

//////////////////////////////////////////////////////////////



class UVLoop : public Loop
{
public:

	UVLoop();
	virtual ~UVLoop() noexcept;

	virtual void run() noexcept;
	virtual void exit()  noexcept;

	virtual void onThreadStart(std::function<void()> f) noexcept;
	virtual void onThreadShutdown(std::function<void()> f) noexcept;

	virtual bool isEventThread() const noexcept;

	virtual repro::Future<int> signal(int s) noexcept;

	std::shared_ptr<uv_loop_t> io()
	{
		return io_;
	}

private:

	UVLoop(const UVLoop& rhs) = delete;
	UVLoop(UVLoop&& rhs) = delete;
	UVLoop& operator=(const UVLoop& rhs) = delete;
	UVLoop& operator=(UVLoop&& rhs) = delete;

	std::thread::id eventThreadId_;
	std::vector<std::shared_ptr<Signal>> signals_;

    std::shared_ptr<uv_loop_t> io_;
};


UVLoop& uvLoop();


class TcpConnectionImpl
{
public:

	TcpConnectionImpl();

    std::shared_ptr<uv_tcp_t> con;

	enum { max_length = 1024 };
	char data[max_length];

	Timeout timer;

	bool closed = false;
};

class SslConnectionImpl
{
public:

	SslConnectionImpl(SslCtx&);

	bool isHttp2Requested();	

    std::shared_ptr<uv_tcp_t> con;
    SSL* ssl;

	enum { max_length = 1024 };
	char data[max_length];

	Timeout timer;

	bool closed = false;
};



struct SslCtxImpl
{
	SslCtxImpl();

    SSL_CTX* ctx;
};

struct ListenerImpl
{	
	ListenerImpl();
	virtual ~ListenerImpl();

    virtual void accept_handler(uv_tcp_t* c) = 0;

	repro::Future<ConnectionPtr> bind( int port );
	void cancel();

    uv_tcp_t acceptor;
    Promise<ConnectionPtr> p;

    static void on_new_connection(uv_stream_t *server, int status) ; 
    static void on_close(uv_handle_t* handle);
};



#ifndef _WIN32

struct IOImpl
{
	IOImpl();
	~IOImpl();

	repro::Future<> onRead(socket_t fd);
	repro::Future<> onWrite(socket_t fd);

	void cancel();

private:

    static void on_io_callback(uv_poll_t* handle, int status, int events);

	uv_poll_t poll_;
    Promise<> p_;
};

#endif

struct TcpListenerImpl : public ListenerImpl
{
	TcpListenerImpl();	
	~TcpListenerImpl();

    void accept_handler(uv_tcp_t* c);

};



struct SslListenerImpl : public ListenerImpl
{
	SslListenerImpl(SslCtx& ssl);	
	~SslListenerImpl();

    void accept_handler(uv_tcp_t* c);

	SslCtx& ctx;
};


} // close namespaces

#endif
#endif



