#ifndef _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_CONNECT_SSL_HANDLER_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_CONNECT_SSL_HANDLER_DEF_GUARD_

#include "priocpp/api.h"



namespace prio      {


LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(ssl_connections);

class SslConnectionImpl;

class SslConnection : public Connection
{
public:

	typedef std::shared_ptr<Connection> Ptr;

	SslConnection(SslConnectionImpl* impl);
	virtual ~SslConnection();

	static Future<Connection::Ptr> connect(const std::string& host, int port, SslCtx& ctx);

	virtual Future<Connection::Ptr, std::string> read();
	virtual Future<Connection::Ptr, std::string> read(size_t n);
	virtual Future<Connection::Ptr> write(const std::string& data);

	virtual void close();
	virtual Future<> shutdown();
	virtual void cancel();

	virtual connection_timeout_t& timeouts();

	virtual bool isHttp2Requested();
	
protected:

	void ssl_do_connect(repro::Promise<Connection::Ptr> p);
	void do_ssl_read(repro::Promise<Connection::Ptr,std::string> p, short what);
	void do_ssl_read(repro::Promise<Connection::Ptr,std::string> p, short what, std::shared_ptr<std::string> buffer, std::shared_ptr<size_t> want);
	void do_ssl_write( repro::Promise<Connection::Ptr> p, std::string data, std::shared_ptr<size_t> written, short what);

	connection_timeout_t timeouts_;

	std::unique_ptr<SslConnectionImpl> impl_;

};


} // close namespaces

#endif
