#include "gtest/gtest.h"

#include "reprocpp/after.h"
#include "test.h"

#include "priocpp/api.h"
#include "priocpp/task.h"
#include "priocpp/connection.h"
#include "priocpp/ssl_connection.h"
#include "priocpp/pipe.h"
#include "cryptoneat/cryptoneat.h"
#include <signal.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

using namespace repro;
using namespace prio;

class BasicTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
	  MOL_TEST_PRINT_CNTS();
  }

  virtual void TearDown() {
	  MOL_TEST_PRINT_CNTS();
  }
}; // end test setup


TEST_F(BasicTest, HttpClient2) {

	std::string result;
	{
#ifndef _WIN32
		signal(SIGPIPE).then([](int s){ std::cout << "SIGPIPE" << std::endl; });
#endif
		Connection::Ptr client;

		TcpConnection::connect("amazon.de",80)
		.then( [&client](Connection::Ptr con)
		{
			client = con;
			return con->write("GET / HTTP/1.0\r\n\r\n");
		})
		.then( [](Connection::Ptr con)
		{
			return con->read();
		})
		.then( [&result](Connection::Ptr con, std::string data)
		{
			result = data.substr(0,15);
			con->close();
			theLoop().exit();
		})
		.otherwise([](const std::exception& ex)
		{
			std::cout << ex.what() << std::endl;
			theLoop().exit();
		});
		theLoop().run();
	}

	EXPECT_EQ("HTTP/1.1 301 Mo",result);
	MOL_TEST_ASSERT_CNTS(0,0);

}




TEST_F(BasicTest, SSlClient2) {

	SslCtx ssl;

	std::string result;
	{
#ifndef _WIN32
		signal(SIGPIPE).then([](int s){ std::cout << "SIGPIPE" << std::endl; });
#endif
		Connection::Ptr client;

		SslConnection::connect("amazon.de",443,ssl)
		.then( [&client](Connection::Ptr con)
		{
			std::cout << "connected" << std::endl;
			client = con;
			return con->write("GET / HTTP/1.0\r\nHost:amazon.de\r\n\r\n");
		})
		.then( [](Connection::Ptr con)
		{
			std::cout << "written" << std::endl;			
			return con->read();
		})
		.then( [&result](Connection::Ptr con, std::string data)
		{
			result = data.substr(0,15);
			std::cout << "read " << result << std::endl;			
			con->close();
			theLoop().exit();
		})
		.otherwise([](const std::exception& ex)
		{
			std::cout << "ex!" << std::endl;						
			std::cout << ex.what() << std::endl;
			theLoop().exit();
		});
		theLoop().run();
	}

	EXPECT_EQ("HTTP/1.1 301 Mo",result);
	MOL_TEST_ASSERT_CNTS(0,0);

}




TEST_F(BasicTest, SSlClient3) 
{
	SslCtx ssl;
	ssl.load_cert_pem("pem/server.pem");

	std::string result;
	{
		signal(SIGINT).then([](int s){ theLoop().exit(); });

		Connection::Ptr c;

		Listener listener(ssl);
		listener.bind(8765)
		.then( [&c](Connection::Ptr con)
		{ 
			std::cout << "server connected ex!" << std::endl;
			c =con;
			return con->read();
		})
		.then( [](Connection::Ptr con, std::string data)
		{
			std::cout << "server has read" << std::endl;
			return con->write(data);
		})
		.then( [](Connection::Ptr con)
		{
			std::cout << "server has written" << std::endl;
			return con->read();
		})
		.otherwise([&listener](const std::exception& ex)
		{
			std::cout << "ex!" << ex.what() << std::endl;
			listener.cancel();

			timeout( []() {
				theLoop().exit();			
			},1,100);
		});

		SslCtx ctx;
		SslConnection::Ptr con;
		nextTick([&con,&ctx,&result,&listener]()
		{
			SslConnection::connect("localhost",8765,ctx)
			.then( [&con](Connection::Ptr c)
			{
				std::cout << "client connected" << std::endl;
				con = c;
				return con->write("HELO");
			})
			.then( [](Connection::Ptr con)
			{
				std::cout << "client written" << std::endl;
				return con->read();
			})
			.then( [&result](Connection::Ptr con, std::string data)
			{
				std::cout << "client received " << data << std::endl;
				result = data;
				con->close();
			})
			.otherwise([&listener](const std::exception& ex)
			{
				std::cout << ex.what() << std::endl;
				listener.cancel();
				theLoop().exit();
			});
		});

		theLoop().run();
	}

	MOL_TEST_ASSERT_CNTS(0,0);
	EXPECT_EQ("HELO",result);

}

#ifdef _RESUMABLE_FUNCTIONS_SUPPORTED

repro::Future<> coroutine_example(std::string& result,Connection::Ptr& client);


TEST_F(BasicTest, Coroutine) {

	std::string result;
	{
		signal(SIGINT).then([](int s) {theLoop().exit(); });

		Connection::Ptr client;
		coroutine_example(result,client).then([](){});

		theLoop().run();
	}

	EXPECT_EQ("HTTP/1.1 301 Mo", result);
	MOL_TEST_ASSERT_CNTS(0, 0);
}


repro::Future<> coroutine_example(std::string& result,Connection::Ptr& client)
{
	try
	{
		client = co_await TcpConnection::connect("amazon.de",80);
		co_await client->write("GET / HTTP/1.0\r\n\r\n");

		client->read()
		.then( [&result](Connection::Ptr con, std::string data)
		{
			result = data.substr(0,15);
			con->close();
			theLoop().exit();
		})
		.otherwise([](const std::exception& ex)
		{
			std::cout << ex.what() << std::endl;
			theLoop().exit();
		});

		//theLoop().exit();
	}
	catch (const std::exception& ex)
	{
		std::cout << "ex:t " << ex.what() << std::endl;
		theLoop().exit();
	}
	co_return;
}

#endif


int main(int argc, char **argv) 
{
	prio::init();
	cryptoneat::SSLUser useSSL;

	::testing::InitGoogleTest(&argc, argv);
    int r = RUN_ALL_TESTS();

    return r;
}