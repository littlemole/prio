#ifdef PROMISE_USE_BOOST_ASIO

#include "priocpp/api.h"
#include "priocpp/impl/asio.h"
#include "priocpp/ssl_connection.h"

#include <iostream>

#ifdef _WIN32
#include <inttypes.h>
typedef int ssize_t;
#define close_socket ::closesocket
#define SHUT_WR SD_SEND 
#else
#define close_socket ::close
#endif

using namespace repro;

namespace prio      {


extern unsigned char next_proto_list[256];
extern size_t next_proto_list_len;	

SslConnectionImpl::SslConnectionImpl(SslCtx& ssl)
	: socket(asioLoop().io(), ssl.ctx->ssl), 
	  resolver(asioLoop().io()),
	  ctx(ssl.ctx->ssl)
{
}

bool SslConnectionImpl::isHttp2Requested()
{
	const unsigned char *alpn = NULL;
	unsigned int alpnlen = 0;

	SSL* ssl = socket.native_handle();
	
	SSL_get0_next_proto_negotiated(ssl, &alpn, &alpnlen);

#if OPENSSL_VERSION_NUMBER >= 0x10002000L

	if (alpn == NULL) 
	{
		SSL_get0_alpn_selected(ssl, &alpn, &alpnlen);
	}

#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
	
	if (alpn == NULL || alpnlen != 2 || memcmp("h2", alpn, 2) != 0) 
	{
		return false;
	}
	return true;
}


SslConnection::SslConnection(SslConnectionImpl* impl)
	: timeouts_(connection_timeouts()),
	  impl_(impl)
{
	REPRO_MONITOR_INCR(SslConnection);
}


SslConnection::~SslConnection()
{
	cancel();
	close();
	REPRO_MONITOR_DECR(SslConnection);	
}

connection_timeout_t& SslConnection::timeouts()
{
	return timeouts_;
}

Future<Connection::Ptr> SslConnection::connect(const std::string& host, int port,  SslCtx& ctx )
{
	auto p = promise<Connection::Ptr>();

	auto impl = new SslConnectionImpl(ctx);
	auto c = new SslConnection(impl);		

	boost::asio::ip::tcp::resolver::query query(host, "");
	impl->resolver.async_resolve(
		query,
		[impl,p,port,c](const boost::system::error_code& error,boost::asio::ip::tcp::resolver::iterator iterator)
		{
			while(iterator != boost::asio::ip::tcp::resolver::iterator())
			{
				boost::asio::ip::tcp::endpoint end = *iterator;
				if (end.protocol() != boost::asio::ip::tcp::v4())
				{
					iterator++;
					continue;
				}

				boost::asio::ip::tcp::endpoint endpoint(
					end.address(), 
					port
				);

				impl->socket
				.lowest_layer()
				.async_connect( 
					endpoint, 
					[impl,p,c](const boost::system::error_code& error)
					{
						if(error)
						{
							delete c;
							p.reject(Ex("connection failed"));
						}
						else
						{
							impl->socket.async_handshake(
								boost::asio::ssl::stream_base::client,
								[impl,p,c](const boost::system::error_code& error)
								{
									if(error)
									{
										delete c;
										p.reject(Ex("ssl handshake client connect failed"));
										return;		
									}
#if BOOST_VERSION < 106599
									boost::asio::ip::tcp::socket::non_blocking_io non_blocking_io(true);
									impl->socket.lowest_layer().io_control(non_blocking_io);
#else
									impl->socket.lowest_layer().non_blocking(true);
#endif									
									p.resolve(Connection::Ptr( c ));
								}
							);
						}
					}
				);
				return;
			}
			
			delete c;
			p.reject(Ex("connect dns lookup failed"));
		}
	);

	return p.future();
}


Future<Connection::Ptr, std::string> SslConnection::read()
{
	auto p = promise<Connection::Ptr,std::string>();
	auto ptr = shared_from_this();

	if (timeouts_.rw_timeout_s != 0)
	{
		impl_->timer.after(timeouts_.rw_timeout_s).then(cancellation(p, impl_->socket.lowest_layer()));
	}

	impl_->socket.async_read_some(
		boost::asio::buffer(impl_->data,impl_->max_length),
		[this,ptr,p](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();

			if(error)
			{
				if (is_io_cancelled(error)) return;

				p.reject(Ex(std::string("read failed ")+error.message()));
			}
			else
			{
				p.resolve(ptr,std::string(impl_->data,bytes_transferred));
			}
		}
	);

	return p.future();
}

Future<Connection::Ptr, std::string> SslConnection::read(size_t s)
{
	auto p = promise<Connection::Ptr,std::string>();
	auto ptr = shared_from_this();
	
	if (timeouts_.rw_timeout_s != 0)
	{
		impl_->timer.after(timeouts_.rw_timeout_s).then(cancellation(p, impl_->socket.lowest_layer()));
	}

	std::shared_ptr<std::vector<char>> buffer = std::make_shared<std::vector<char>>(s,0);

	async_read(
		impl_->socket,
		boost::asio::buffer(&(buffer->at(0)),s),
		[this,ptr,p,buffer](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();

			if(error)
			{
				if (is_io_cancelled(error)) return;
				
				p.reject(Ex(std::string("read() failed")+error.message()));
			}
			else
			{
				p.resolve( ptr, std::string( &(buffer->at(0)), bytes_transferred) );
			}
		}
	);

	return p.future();
}


Future<Connection::Ptr> SslConnection::write( const std::string& data)
{
	auto p = promise<Connection::Ptr>();
	auto ptr = shared_from_this();

	if (timeouts_.rw_timeout_s != 0)
	{
		impl_->timer.after(timeouts_.rw_timeout_s).then(cancellation(p, impl_->socket.lowest_layer()));
	}

	std::shared_ptr<std::string> buffer = std::make_shared<std::string>(data);

	async_write(
		impl_->socket,
		boost::asio::buffer(buffer->data(),buffer->size()),
		[this,p,ptr,buffer](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();

			if(error)
			{
				if (is_io_cancelled(error)) return;
				
				p.reject(Ex(std::string("write failed")+error.message()));
			}
			else
			{
				p.resolve( ptr );
			}
		}
	);

	return p.future();
}


void SslConnection::close()
{
	if(impl_->socket.lowest_layer().is_open())
	{
		impl_->socket.lowest_layer().close();
	}	
}

Future<> SslConnection::shutdown()
{
	
	if(!impl_->socket.lowest_layer().is_open())
		return resolved<>();

	auto p = promise<>();

	impl_->socket.shutdown();

	read()
	.then([this,p](Connection::Ptr,std::string data)
	{
		close();
		p.resolve();
	})
	.otherwise([this,p](const std::exception& ex)
	{
		close();
		p.resolve();
	});

	return p.future();
}

void SslConnection::cancel()
{
	try
	{
		impl_->timer.cancel();

		if(impl_->socket.lowest_layer().is_open())
		{
			impl_->socket.lowest_layer().cancel();
		}
	}
	catch(const std::exception& ex)
	{
		std::cout << "ex in cancel(): " << ex.what() << std::endl;
	}
	catch(...)
	{
		std::cout << "some ex in cancel(): ?"  << std::endl;
	}
}

bool SslConnection::isHttp2Requested()
{
	return impl_->isHttp2Requested();
}


SslCtxImpl::SslCtxImpl()
	  : ssl( boost::asio::ssl::context::tlsv12 ) 
{}

SslCtx::SslCtx()
	: ctx(new SslCtxImpl)
{
	SSL_CTX* sslCtx = ctx->ssl.native_handle();
	SSL_CTX_set_options(
		sslCtx,
		SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
	  	SSL_OP_NO_COMPRESSION |
	  	SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
	);
}

SslCtx::~SslCtx()
{}

void SslCtx::load_cert_pem(const std::string& file)
{
	std::cout << "load cer " << file << std::endl;
	ctx->ssl.use_certificate_chain_file(file);
	ctx->ssl.use_private_key_file(file, boost::asio::ssl::context::pem);

	SSL_CTX* sslCtx = ctx->ssl.native_handle();

	EC_KEY *ecdh = 0;
	ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	if (!ecdh) 
	{
		throw(Ex("EC_KEY curvename failed "));
	}
	SSL_CTX_set_tmp_ecdh(sslCtx, ecdh);
	EC_KEY_free(ecdh);	

	int r = SSL_CTX_set_cipher_list(sslCtx,"ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK");

	std::cout << "set cipher: " << r << std::endl;

}

} // close namespaces

#endif


