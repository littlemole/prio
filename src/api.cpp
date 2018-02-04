#include <fcntl.h>

#include "priocpp/api.h"
#include "priocpp/loop.h"
#include "priocpp/connection.h"
#include "priocpp/ssl_connection.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

#ifdef _WIN32
#include <inttypes.h>
typedef int ssize_t;
#endif
#include <nghttp2/nghttp2.h>


namespace prio      {



Future<> timeout( int secs, int ms) noexcept
{
  auto p = repro::promise<>();
  
  auto t = new Timeout();

  ms = ms + 1000*secs;
  
  t->after(ms)
  .then( [p,t]() 
  {
    p.resolve();
    delete t;
  })
  .otherwise( [p,t]( const std::exception& ex) 
  {
    p.reject(ex);
    delete t;
  });

  return p.future();
}

void timeout(const std::function<void()>& f, int secs, int ms) noexcept
{
  auto t = new Timeout();

  ms = ms + 1000*secs;
  
  t->after( [t,f]()
  {
	 f();
	 delete t;
  }, ms);

}


void timeout(const std::function<void()>& f, int secs) noexcept
{
	timeout(f, secs,0);
}

Future<> timeout( int secs) noexcept
{
	return timeout(secs,0);
}




unsigned char next_proto_list[256];
size_t next_proto_list_len;

Future<int> signal(int s) noexcept
{
	return theLoop().signal(s);
}



connection_timeout_t& connection_timeouts()
{
	static 	connection_timeout_t ctt { 10000, 10000 };
	return ctt;
}

Future<Connection::Ptr> Connection::connect(const std::string& host, int port)
{
	return TcpConnection::connect(host,port);
}


Future<Connection::Ptr> Connection::connect(const std::string& host, int port, SslCtx& ctx)
{
	return SslConnection::connect(host,port,ctx);
}

int next_proto_cb(SSL *ssl, const unsigned char **data,unsigned int *len, void *arg) 
{
  *data = next_proto_list;
  *len = (unsigned int)next_proto_list_len;
  return SSL_TLSEXT_ERR_OK;
}

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
int alpn_select_proto_cb(SSL *ssl, const unsigned char **out,
          unsigned char *outlen, const unsigned char *in,
		  unsigned int inlen, void *arg) 
{
  int rv = nghttp2_select_next_protocol((unsigned char **)out, outlen, in, inlen);

  if (rv != 1) {
    return SSL_TLSEXT_ERR_NOACK;
  }

  return SSL_TLSEXT_ERR_OK;
}
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L


} // close namespaces



