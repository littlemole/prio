#ifdef PROMISE_USE_BOOST_ASIO

#include "priocpp/api.h"
#include "priocpp/impl/asio.h"
#include "priocpp/ssl_connection.h"

#include <iostream>

#ifdef _WIN32
#include <inttypes.h>
typedef int ssize_t;
#include <nghttp2/nghttp2.h>
#define close_socket ::closesocket
#define SHUT_WR SD_SEND 
#else
#include <nghttp2/nghttp2.h>
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
{}

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
	LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(ssl_connections);
}


SslConnection::~SslConnection()
{
	close();

	LITTLE_MOLE_RELEASE_DEBUG_REF_CNT(ssl_connections);
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

									boost::asio::ip::tcp::socket::non_blocking_io non_blocking_io(true);
									impl->socket.lowest_layer().io_control(non_blocking_io);

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

	//cancel();
/*
	impl_->timer.after(timeouts_.rw_timeout_s)
	.then( [this,p]()
	{
		impl_->socket.lowest_layer().cancel();
		p.reject(IoTimeout("ssl read cancelled due to timeout"));
	});	
*/
	impl_->socket.async_read_some(
		boost::asio::buffer(impl_->data,impl_->max_length),
		[this,ptr,p](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();

			if(error)
			{
				if(error.value() == boost::system::errc::operation_canceled)
				{
					std::cout << "cancel ssl::read()" << std::endl;
					return;
				}
/*
				if ((error.category() == boost::asio::error::get_ssl_category())
					&& (ERR_GET_REASON(error.value()) == SSL_R_SHORT_READ))
			    {
					p.resolve(ptr,"");
					return;
				}	
				*/				

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

	//cancel();
/*
	impl_->timer.after(timeouts_.rw_timeout_s)
	.then( [this,p]()
	{
		impl_->socket.lowest_layer().cancel();
		p.reject(IoTimeout("ssl read(n) cancelled due to timeout"));
	});	
*/
	std::shared_ptr<std::vector<char>> buffer = std::make_shared<std::vector<char>>(s,0);

	async_read(
		impl_->socket,
		boost::asio::buffer(&(buffer->at(0)),s),
		[this,ptr,p,buffer](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();

			if(error)
			{
				if(error.value() == boost::system::errc::operation_canceled)
				{
					std::cout << "cancel ssl::read(n)" << std::endl;
					return;
				}
				
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

	//cancel();

	/*
	impl_->timer.after(timeouts_.rw_timeout_s)
	.then( [this,p]()
	{
		impl_->socket.lowest_layer().cancel();
		p.reject(IoTimeout("ssl write cancelled due to timeout"));
	});	
*/

	std::shared_ptr<std::string> buffer = std::make_shared<std::string>(data);

	async_write(
		impl_->socket,
		boost::asio::buffer(buffer->data(),buffer->size()),
		[this,p,ptr,buffer](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();
			
			if(error)
			{
				if(error.value() == boost::system::errc::operation_canceled)
				{
					std::cout << "cancel ssl::write()" << std::endl;
					return;
				}
				
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
	std::cout << "SslCOnnection::close()" << std::endl;
	//cancel();
	if(impl_->socket.lowest_layer().is_open())
	{
		impl_->socket.lowest_layer().close();
	}	
}

Future<> SslConnection::shutdown()
{
	std::cout << "SslCOnnection::shutdown()" << std::endl;
	
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
	std::cout << "SslCOnnection::cancel()" << std::endl;
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
	  : ssl(asioLoop().io(), boost::asio::ssl::context::sslv23)
{}

SslCtx::SslCtx()
	: ctx(new SslCtxImpl)
{
	/*
	ctx->ssl.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
	);
	*/
	
	EC_KEY *ecdh = 0;
	SSL_CTX* sslCtx = ctx->ssl.native_handle();
	SSL_CTX_set_options(
		sslCtx,
		SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
	  	SSL_OP_NO_COMPRESSION |
	  	SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
	);
  
	ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	if (!ecdh) 
	{
		throw(Ex("EC_KEY curvename failed "));
	}
	SSL_CTX_set_tmp_ecdh(sslCtx, ecdh);
	EC_KEY_free(ecdh);	

}


SslCtx::~SslCtx()
{
}


void SslCtx::load_cert_pem(const std::string& file)
{
	ctx->ssl.use_certificate_chain_file(file);
	ctx->ssl.use_private_key_file(file, boost::asio::ssl::context::pem);
}

int next_proto_cb(SSL *ssl, const unsigned char **data,unsigned int *len, void *arg);

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
int alpn_select_proto_cb(SSL *ssl, const unsigned char **out,
          unsigned char *outlen, const unsigned char *in,
		  unsigned int inlen, void *arg);
#endif


static int select_next_proto_cb(SSL *ssl, unsigned char **out,
	unsigned char *outlen, const unsigned char *in,
	unsigned int inlen, void *arg) 
{
	if (nghttp2_select_next_protocol(out, outlen, in, inlen) <= 0) 
	{
		std::cout << "err select_next_proto_cb ACK failed -no http2" << std::endl;
		return SSL_TLSEXT_ERR_NOACK;
	}
	return SSL_TLSEXT_ERR_OK;
}

void SslCtx::enableHttp2()
{
	SSL_CTX* sslCtx = ctx->ssl.native_handle();

	next_proto_list[0] = NGHTTP2_PROTO_VERSION_ID_LEN;
	memcpy(&next_proto_list[1], NGHTTP2_PROTO_VERSION_ID,
	NGHTTP2_PROTO_VERSION_ID_LEN);
	next_proto_list_len = 1 + NGHTTP2_PROTO_VERSION_ID_LEN;
	
	SSL_CTX_set_next_protos_advertised_cb(sslCtx, next_proto_cb, NULL);
	
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	SSL_CTX_set_alpn_select_cb(sslCtx, alpn_select_proto_cb, NULL);
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
}

void SslCtx::enableHttp2Client(  )
{
	ctx->enableHttp2Client();
}

void SslCtxImpl::enableHttp2Client(  )
{
	SSL_CTX* sslCtx = ssl.native_handle();
	next_proto_list[0] = NGHTTP2_PROTO_VERSION_ID_LEN;
	memcpy(&next_proto_list[1], NGHTTP2_PROTO_VERSION_ID,
	NGHTTP2_PROTO_VERSION_ID_LEN);
	next_proto_list_len = 1 + NGHTTP2_PROTO_VERSION_ID_LEN;

	SSL_CTX_set_next_proto_select_cb(sslCtx, select_next_proto_cb, NULL);
	
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	  SSL_CTX_set_alpn_protos(sslCtx, (const unsigned char *)"\x02h2", 3);
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L	  
}


} // close namespaces

#endif


