#ifdef PROMISE_USE_BOOST_ASIO

#include <fcntl.h>

#include "priocpp/api.h"
#include "priocpp/connection.h"
#include "priocpp/impl/asio.h"

using namespace repro;

namespace prio      {


TcpConnectionImpl::TcpConnectionImpl()
	: socket(asioLoop().io()), 
	  resolver(asioLoop().io())
{}

TcpConnection::TcpConnection(TcpConnectionImpl* impl)
	: impl_(impl), timeouts_(connection_timeouts())
{
	LITTLE_MOLE_ADDREF_DEBUG_REF_CNT(tcp_connections);
}

TcpConnection::~TcpConnection()
{
	LITTLE_MOLE_RELEASE_DEBUG_REF_CNT(tcp_connections);
	close();
}

connection_timeout_t& TcpConnection::timeouts()
{
	return timeouts_;
}

Future<Connection::Ptr> TcpConnection::connect(const std::string& host, int port)
{
	auto p = repro::promise<Connection::Ptr>();

	auto impl = new TcpConnectionImpl;
	Connection::Ptr ptr( new TcpConnection(impl) );		

	boost::asio::ip::tcp::resolver::query query(host, "");
	impl->resolver
	.async_resolve(
		query,
		[impl,p,ptr,port](const boost::system::error_code& error,boost::asio::ip::tcp::resolver::iterator iterator)
		{
			while(iterator != boost::asio::ip::tcp::resolver::iterator())
			{
				boost::asio::ip::tcp::endpoint end = *iterator;
				//std::string s = end.address().to_string();
				if (end.protocol() != boost::asio::ip::tcp::v4())
				{
					iterator++;
					continue;
				}

				boost::asio::ip::tcp::endpoint endpoint(
					end.address(), 
					port
				);

				impl->timer.after(ptr->timeouts().connect_timeout_s)
				.then([impl,p]()
				{
					impl->socket.cancel();
					p.reject(IoTimeout("connect cancelled due to timeout"));
				});

				impl->socket
				.async_connect( 
					endpoint, 
					[impl,p,ptr](const boost::system::error_code& error)
					{
						impl->timer.cancel();

						if(error)
						{
							p.reject(repro::Ex("connection failed"));
						}
						else
						{
							boost::asio::ip::tcp::socket::non_blocking_io non_blocking_io(true);
							impl->socket.io_control(non_blocking_io);
							p.resolve(ptr);
						}
					}
				);
				return;
			}
			p.reject(repro::Ex("connect dns lookup failed"));
		}
	);

	return p.future();
}



Future<Connection::Ptr, std::string> TcpConnection::read()
{
	auto p = repro::promise<Connection::Ptr,std::string>();

	auto ptr = shared_from_this();

	impl_->timer.after(timeouts_.rw_timeout_s)
	.then( [this,p]()
	{
		impl_->socket.cancel();
		p.reject(IoTimeout("read cancelled due to timeout"));
	});	

	impl_->socket
	.async_read_some(
		boost::asio::buffer(impl_->data,impl_->max_length),
		[this,ptr,p](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();

			if(impl_->closed)
			{
				return;
			}
			
			if(error)
			{
				if(error.value() == boost::system::errc::operation_canceled)
				{
					return;
				}
				std::cout << "read failed: " << error.value() << " == " << boost::system::errc::operation_canceled << std::endl;
				p.reject(repro::Ex(std::string("read failed: ") + error.message()));
			}
			else
			{
				p.resolve(ptr,std::string(impl_->data,bytes_transferred));
			}
		}
	);

	return p.future();
}

Future<Connection::Ptr, std::string> TcpConnection::read(size_t s)
{
	auto p = repro::promise<Connection::Ptr,std::string>();

	auto ptr = shared_from_this();

	impl_->timer.after(timeouts_.rw_timeout_s)
	.then( [this,p]()
	{
		impl_->socket.cancel();
		p.reject(IoTimeout("read(n) cancelled due to timeout"));
	});	

	std::shared_ptr<std::vector<char>> buffer = std::make_shared<std::vector<char>>(s,0);

	async_read(
		impl_->socket,
		boost::asio::buffer(&(buffer->at(0)),s),
		[this,ptr,p,buffer](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();

			if(impl_->closed)
			{
				return;
			}			

			if(error)
			{
				if(error.value() == boost::system::errc::operation_canceled)
				{
					return;
				}

				p.reject(repro::Ex(std::string("read failed: ") + error.message()) );
			}
			else
			{
				p.resolve( ptr, std::string( &(buffer->at(0)), bytes_transferred) );
			}
		}
	);

	return p.future();
}

Future<Connection::Ptr> TcpConnection::write(const std::string& data)
{
	auto p = repro::promise<Connection::Ptr>();

	auto ptr = shared_from_this();

	impl_->timer.after(timeouts_.rw_timeout_s)
	.then( [this,p]()
	{
		impl_->socket.cancel();
		p.reject(IoTimeout("write cancelled due to timeout"));
	});	

	std::shared_ptr<std::string> buffer = std::make_shared<std::string>(data);

	async_write(
		impl_->socket,
		boost::asio::buffer(buffer->data(),buffer->size()),
		[this,p,ptr,buffer](const boost::system::error_code& error,std::size_t bytes_transferred)
		{
			impl_->timer.cancel();
			
			if(impl_->closed)
			{
				return;
			}
			
			if(error)
			{
				if(error.value() == boost::system::errc::operation_canceled)
				{
					return;
				}

				p.reject(repro::Ex(std::string("write failed: ") + error.message()));
			}
			else
			{
				p.resolve( ptr );
			}
		}
	);

	return p.future();
}

void TcpConnection::close()
{
	impl_->closed = true;	
	impl_->socket.close();	
}




Future<> TcpConnection::shutdown()
{
	if(!impl_->socket.is_open())
		return resolved<>();

	auto p = repro::promise<>();

	impl_->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_type::shutdown_send);

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

void TcpConnection::cancel()
{
	impl_->timer.cancel();
	impl_->socket.cancel();
}


} // close namespaces


#endif

