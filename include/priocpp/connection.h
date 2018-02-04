#ifndef _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_CONNECT_HANDLER_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_CONNECT_HANDLER_DEF_GUARD_

#include "priocpp/api.h"
 

namespace prio      {


LITTLE_MOLE_DECLARE_DEBUG_REF_CNT(tcp_connections);


class TcpConnectionImpl;

class TcpConnection : public Connection
{
public:

	typedef std::shared_ptr<Connection> Ptr;

	TcpConnection(TcpConnectionImpl*);
	virtual ~TcpConnection();

	static Future<Connection::Ptr> connect(const std::string& host, int port);

	virtual Future<Connection::Ptr, std::string> read();
	virtual Future<Connection::Ptr, std::string> read(size_t n);
	virtual Future<Connection::Ptr> write(const std::string& data);

	virtual void close();
	virtual Future<> shutdown();
	virtual void cancel();

	virtual connection_timeout_t& timeouts();

protected:

	std::unique_ptr<TcpConnectionImpl> impl_;
	connection_timeout_t timeouts_;
};


} // close namespaces

#endif

