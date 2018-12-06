#ifdef PROMISE_USE_LIBEVENT

#include <priocpp/api.h>
#include <priocpp/task.h>
#include <priocpp/impl/event.h>
#include "priocpp/ssl_connection.h"

#include <fcntl.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>


#include <iostream>

#ifdef _WIN32
#include <inttypes.h>
typedef int ssize_t;
//#include <nghttp2/nghttp2.h>
#define close_socket ::closesocket
#define SHUT_WR SD_SEND 
#else
//#include <nghttp2/nghttp2.h>
#define close_socket ::close
#endif

using namespace repro;

namespace prio      {


extern unsigned char next_proto_list[256];
extern size_t next_proto_list_len;
	
// forward
int check_err_code(SSL* ssl, int len, int want);


SslConnectionImpl::SslConnectionImpl()
	: fd(-1),
	  ssl(0)
{
}

SslConnectionImpl::~SslConnectionImpl()
{
}


SslConnection::SslConnection(SslConnectionImpl* impl)
	: impl_(impl)
{
	//std::cout << "SslConnection()" << std::endl;	
	LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(ssl_connections);

	timeouts_ = connection_timeouts();
}


SslConnection::~SslConnection()
{
	//std::cout << "~SslConnection()" << std::endl;
	close();

	LITTLE_MOLE_RELEASE_DEBUG_REF_CNT(ssl_connections);
}

connection_timeout_t& SslConnection::timeouts()
{
	return timeouts_;
}


void SslConnection::ssl_do_connect(Promise<Connection::Ptr> p)
{
	int r = SSL_connect(impl_->ssl);
	int s = check_err_code(impl_->ssl,r,EV_READ);
	if ( s == 0)
	{
		p.resolve(Ptr(this));
		return;
	}
	if ( s < 0 )
	{
		p.reject(Ex());
		delete this;
		return;
	}

	impl_->e_ = onEvent( impl_->fd, s)
	->callback( [this,p](socket_t fd, short w)
	{
		if(w == EV_TIMEOUT)
		{
			p.reject(IoTimeout("IO timeout in SslConnection::do_connect"));
			delete this;
			return;
		}
		ssl_do_connect(p);
	})
	->add(timeouts().connect_timeout_s,0);
}

Future<Connection::Ptr> SslConnection::connect(const std::string& host, int port,  SslCtx& ctx )
{
	auto p = promise<Connection::Ptr>();

	auto impl = new SslConnectionImpl;
	auto ptr = new SslConnection(impl);		

	std::string h = host;

	dnsResolver()
	.connect(h,port)
	.then( [p,impl,ptr,&ctx](socket_t fd)
	{
		impl->fd = fd;

		SSL* ssl = SSL_new(ctx.ctx->ctx);
		impl->ssl = ssl;

		SSL_set_fd(ssl,(int)fd);
		SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

		ptr->ssl_do_connect(p);
	})
	.otherwise([p](const std::exception& ex)
	{
		p.reject(ex);
	});

	return p.future();	
}


void SslConnection::do_ssl_read(Promise<Connection::Ptr,std::string> p, short what)
{
	impl_->e_ = onEvent(impl_->fd,what)
	->callback( [this,p,what](socket_t fd, short w)
	{
		if( what & EV_TIMEOUT)
		{
			p.reject( IoTimeout("timeout in do_ssl_read") );
			return;
		}

		char c[1024];
		int n = 1024;
 
		int len = SSL_read( impl_->ssl, c, n );
		if ( len > 0 )
		{
			p.resolve(shared_from_this(),std::string(c,len));
			return;
		}

		int s = check_err_code(impl_->ssl,len,EV_READ);
		if ( s <= 0 )
		{
//			auto e = impl_->e_;
			p.reject(Ex("ssl io EOF"));
//			e->dispose();
			return;
		}
		do_ssl_read(p,s);
	});

	if (timeouts().rw_timeout_s != 0)
	{
		impl_->e_->add(timeouts().rw_timeout_s, 0);
	}
	else
	{
		impl_->e_->add();
	}
}


Future<Connection::Ptr, std::string> SslConnection::read()
{
	auto p = promise<Connection::Ptr,std::string>();

	char c[1024];
	int n = 1024;

	int len = SSL_read( impl_->ssl, c, n );
	if ( len > 0 )
	{
		std::string data(c,len);

		auto ptr = shared_from_this();
		nextTick( [p,data,ptr]()
		{
			p.resolve(ptr,data);
		});
		return p.future();
	}

	int s = check_err_code(impl_->ssl,len,EV_READ);
	if ( s <= 0 )
	{
		nextTick( [p]()
		{
			p.reject(Ex("ssl io EOF read"));
		});
		return p.future();
	}

	do_ssl_read(p,s);

	return p.future();
}


void SslConnection::do_ssl_read( Promise<Connection::Ptr,std::string> p, short what, std::shared_ptr<std::string> buffer, std::shared_ptr<size_t> want)
{
	impl_->e_ = onEvent(impl_->fd,what)
	->callback( [this,p,want,buffer](socket_t fd, short w)
	{
		while(true)
		{
			char c[1024];
			int n = *want;

			int len = SSL_read( impl_->ssl, c, n );
			if ( len > 0 )
			{
				buffer->append(std::string(c,len));

				if ( *want - len <= 0)
				{
					p.resolve(shared_from_this(),*buffer);
					return;
				}
				*want -= len;
				continue;
			}

			int s = check_err_code(impl_->ssl,len,EV_READ);
			if ( s <= 0 )
			{
				p.reject(Ex("ssl io EOF"));
				return;
			}

			do_ssl_read(p,s,buffer,want);
			return;
		}
	});

	if (timeouts().rw_timeout_s != 0)
	{
		impl_->e_->add(timeouts().rw_timeout_s, 0);
	}
	else
	{
		impl_->e_->add();
	}
}

Future<Connection::Ptr, std::string> SslConnection::read(size_t s)
{
	auto p = promise<Connection::Ptr,std::string>();

	std::shared_ptr<size_t> want = std::make_shared<size_t>(s);
	std::shared_ptr<std::string> buffer = std::make_shared<std::string>();

//	char c[s];
	std::vector<char> c(s,0);
	int n = s;

	while(true)
	{
		int len = SSL_read( impl_->ssl, &c[0], n );
		if ( len > 0 )
		{
			std::string tmp(&c[0],len);
			buffer->append(tmp);

			if ( *want -len <= 0 )
			{
				auto ptr = shared_from_this();
				nextTick( [p,buffer,ptr]()
				{
					p.resolve(ptr,*buffer);
				});
				return p.future();
			}
			*want -= len;
			continue;
		}

		int s = check_err_code(impl_->ssl,len,EV_READ);
		if ( s <= 0 )
		{
			nextTick( [p]()
			{
				p.reject(Ex("ssl io EOF"));
			});
			return p.future();
		}

		do_ssl_read(p,s,buffer,want);
		return p.future();
	}

	return p.future();
}

void SslConnection::do_ssl_write(Promise<Connection::Ptr> p, std::string data, std::shared_ptr<size_t> written, short what)
{
	impl_->e_ = onEvent(impl_->fd,what)
	->callback([this,p,data,written](socket_t fd, short w)
	{
		while(true)
		{
			int len = SSL_write( impl_->ssl, data.c_str() + *written, data.size() - *written );
			if ( len > 0 )
			{
				*written = *written + len;
				if (*written >= data.size())
				{
					p.resolve(shared_from_this());
					return;
				}
				continue;
			}

			int s = check_err_code(impl_->ssl,len,EV_WRITE);
			if ( s <= 0 )
			{
				p.reject(Ex("ZERO RETURN"));
				return;
			}

			do_ssl_write(p,data,written,s);
			return;
		}
	});

	if (timeouts().rw_timeout_s != 0)
	{
		impl_->e_->add(timeouts().rw_timeout_s, 0);
	}
	else
	{
		impl_->e_->add();
	}
}

Future<Connection::Ptr> SslConnection::write( const std::string& data)
{
	auto p = promise<Connection::Ptr>();

	auto written = std::make_shared<size_t>(0);

	while(true)
	{
		int len = SSL_write( impl_->ssl, data.c_str() + *written, data.size() - *written );
		if ( len > 0 )
		{
			*written = *written + len;
			if (*written >= data.size())
			{
				auto ptr = shared_from_this();
				nextTick( [p,ptr]()
				{
					p.resolve(ptr);
				});
				return p.future();
			}
			continue;
		}

		int s = check_err_code(impl_->ssl,len,EV_WRITE);
		if ( s <= 0 )
		{
			nextTick( [p]()
			{
				p.reject(Ex("ssl io ex"));
			});
			return p.future();
		}

		do_ssl_write(p,data,written,s);
		return p.future();
	}

	return p.future();
}


void SslConnection::close()
{
	cancel();
	if(impl_->fd != -1)
	{
		close_socket(impl_->fd);
		impl_->fd = -1;
		LITTLE_MOLE_RELEASE_DEBUG_REF_CNT(sockets);
	}
	if(impl_->ssl)
	{
		SSL_free(impl_->ssl);
		impl_->ssl = 0;
	}
}



Future<> SslConnection::shutdown()
{
	auto p = promise<>();

	if(impl_->fd == -1)
		return resolved();

	int s = SSL_get_shutdown(impl_->ssl);

	int r = 0;
    if ( s == SSL_RECEIVED_SHUTDOWN )
	{
		r = SSL_shutdown(impl_->ssl);
		close();
		return resolved();
	}
	else
	{
		r = SSL_shutdown(impl_->ssl);
		if ( r == 0 )
		{
			::shutdown(impl_->fd,SHUT_WR);
			r = SSL_shutdown(impl_->ssl) == 1;
			close();
		}
		return resolved();
	}

	return p.future();
}

void SslConnection::cancel()
{
	if ( impl_ && impl_->e_ )
	{
		impl_->e_->cancel();
	}
}

bool SslConnection::isHttp2Requested()
{
	return impl_->isHttp2Requested();
}

bool SslConnectionImpl::isHttp2Requested()
{
	const unsigned char *alpn = NULL;
    unsigned int alpnlen = 0;
	
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

int check_err_code(SSL* ssl, int len, int want)
{
	int r = SSL_get_error(ssl,len);
	switch(r)
	{
		case SSL_ERROR_SYSCALL :
		{
			if( would_block())
			{
				return want;
			}

			return -1;
		}
		case SSL_ERROR_ZERO_RETURN :
		{
			return -1;
		}
		case SSL_ERROR_NONE:
		{
			return 0;
		}
		case SSL_ERROR_WANT_READ:
		{
			return EV_READ;
		}
		case SSL_ERROR_WANT_WRITE:
		{
			return EV_WRITE;
		}
		default :
		{
			std::cout << "SSL SOME ERR" << r << std::endl;
		}
	}
	return -1;
}




SslCtxImpl::SslCtxImpl()
{
	ctx = SSL_CTX_new(TLSv1_2_method()); //SSLv23_method());

	EC_KEY *ecdh;
  
//	ssl_ctx = SSL_CTX_new(SSLv23_server_method());
	if (!ctx) 
	{
	  throw(Ex("Could not create SSL/TLS context"));
	}

	SSL_CTX_set_options(
		ctx,
		SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
	  	SSL_OP_NO_COMPRESSION |
	  	SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
	);
  
	ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	if (!ecdh) 
	{
		throw(Ex("EC_KEY curvename failed "));
	}
	SSL_CTX_set_tmp_ecdh(ctx, ecdh);
	EC_KEY_free(ecdh);	

	//SSL_CTX_set_cipher_list(ctx,"ECDH+AESGCM:ECDH+CHACHA20:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:RSA+AESGCM:RSA+AES:!aNULL:!MD5:!DSS");

}

SslCtxImpl::~SslCtxImpl()
{
	SSL_CTX_free(ctx);
}


int next_proto_cb(SSL *ssl, const unsigned char **data,unsigned int *len, void *arg);

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
int alpn_select_proto_cb(SSL *ssl, const unsigned char **out,
          unsigned char *outlen, const unsigned char *in,
		  unsigned int inlen, void *arg);
#endif

void SslCtxImpl::loadKeys( const std::string& keyfile )
{
	if(!(SSL_CTX_use_certificate_chain_file(ctx,	keyfile.c_str())))
		throw Ex("Can't read certificate file");

	if(!(SSL_CTX_use_PrivateKey_file(ctx,keyfile.c_str(),SSL_FILETYPE_PEM)))
		throw Ex("Can't read key file");
	  
}

/*

void SslCtxImpl::enableHttp2(  )
{
	next_proto_list[0] = NGHTTP2_PROTO_VERSION_ID_LEN;
	memcpy(&next_proto_list[1], NGHTTP2_PROTO_VERSION_ID,
	NGHTTP2_PROTO_VERSION_ID_LEN);
	next_proto_list_len = 1 + NGHTTP2_PROTO_VERSION_ID_LEN;
	
	SSL_CTX_set_next_protos_advertised_cb(ctx, next_proto_cb, NULL);
	
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	SSL_CTX_set_alpn_select_cb(ctx, alpn_select_proto_cb, NULL);
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
	  
}

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

void SslCtxImpl::enableHttp2Client(  )
{
	next_proto_list[0] = NGHTTP2_PROTO_VERSION_ID_LEN;
	memcpy(&next_proto_list[1], NGHTTP2_PROTO_VERSION_ID,
	NGHTTP2_PROTO_VERSION_ID_LEN);
	next_proto_list_len = 1 + NGHTTP2_PROTO_VERSION_ID_LEN;

	SSL_CTX_set_next_proto_select_cb(ctx, select_next_proto_cb, NULL);
	
#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	  SSL_CTX_set_alpn_protos(ctx, (const unsigned char *)"\x02h2", 3);
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L	  
}

*/

SslCtx::SslCtx()
	: ctx(new SslCtxImpl)
{

}

SslCtx::~SslCtx()
{
}


void SslCtx::load_cert_pem(const std::string& file)
{
	ctx->loadKeys(file);
}

/*
void SslCtx::enableHttp2()
{
	ctx->enableHttp2();
}


void SslCtx::enableHttp2Client()
{
	ctx->enableHttp2Client();
}
*/

} // close namespaces



#endif
