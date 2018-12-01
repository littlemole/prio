#include "gtest/gtest.h"
#include <functional>

#include "reprocpp/after.h"
#include "test.h"

#include "priocpp/api.h"
#include "priocpp/task.h"
#include "priocpp/connection.h"
#include "priocpp/ssl_connection.h"
#include "priocpp/pipe.h"
#include "cryptoneat/cryptoneat.h"
#include <signal.h>

using namespace prio;
using namespace repro;


class BasicTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
	  MOL_TEST_PRINT_CNTS();
  }

  virtual void TearDown() {
	  MOL_TEST_PRINT_CNTS();
  }
}; // end test setup

#ifdef _RESUMABLE_FUNCTIONS_SUPPORTED

Future<> coroReturnNoAsyncVoid(std::string& e)
{
	e = "test";
	co_return;
}

TEST_F(BasicTest, coroReturnNoAsyncVoid) {

	std::string e;

	nextTick( [&e](){

		coroReturnNoAsyncVoid(e);
	
	});

	theLoop().run();

	EXPECT_EQ("test", e);
	MOL_TEST_ASSERT_CNTS(0, 0);
}

Future<std::string> coroReturnNoAsyncString()
{
	co_return "test";
}

Future<> coroReturnNoAsyncStringTrampoline(std::string& e)
{
	e = co_await coroReturnNoAsyncString();
}

TEST_F(BasicTest, coroReturnNoAsyncString) {

	std::string e;

	nextTick( [&e](){

		coroReturnNoAsyncStringTrampoline(e);
	});

	theLoop().run();

	EXPECT_EQ("test", e);
	MOL_TEST_ASSERT_CNTS(0, 0);
}


Future<std::string> coroReturnNoAsyncStringThrow()
{
	throw std::exception();
	co_return "test";
}

Future<> coroReturnNoAsyncStringThrowTrampoline(std::string& e)
{
	try
	{
		e = co_await coroReturnNoAsyncStringThrow();
	}
	catch(const std::exception& ex)
	{
		e = "ex";
	}
}

TEST_F(BasicTest, coroReturnNoAsyncStringThrow) {

	std::string e;

	nextTick( [&e](){

		coroReturnNoAsyncStringThrowTrampoline(e);
	});

	theLoop().run();

	EXPECT_EQ("ex", e);
	MOL_TEST_ASSERT_CNTS(0, 0);
}


Future<std::string> coroReturnNoAsyncStringThrowLate()
{
	co_await nextTick();
	throw std::exception();
	co_return "test";
}

Future<> coroReturnNoAsyncStringThrowLateTrampoline(std::string& e)
{
	try
	{
		e = co_await coroReturnNoAsyncStringThrowLate();
	}
	catch(const std::exception& ex)
	{
		e = "ex";
		theLoop().exit();
	}
	co_return;
}

TEST_F(BasicTest, coroReturnNoAsyncStringThrowLate) {

	std::string e;
/*
	signal(SIGINT)
		.then([](int s) {
		theLoop().exit();
	});
	
*/
	/*
	prio::timeout([]() 
	{ 
		theLoop().exit(); 
	},5,0);
	*/
	nextTick( [&e](){

		coroReturnNoAsyncStringThrowLateTrampoline(e);
	});

	theLoop().run();

	EXPECT_EQ("ex", e);
	MOL_TEST_ASSERT_CNTS(0, 0);
}

Future<std::string> coroReturnNoAsyncStringLate()
{
	co_await nextTick();
	co_return "test";
}

Future<> coroReturnNoAsyncStringLateTrampoline(std::string& e)
{
	try
	{
		e = co_await coroReturnNoAsyncStringLate();
	}
	catch(const std::exception& ex)
	{
		e = "ex";
	}
}

TEST_F(BasicTest, coroReturnNoAsyncStringLate) {

	std::string e;

	nextTick( [&e](){

		coroReturnNoAsyncStringLateTrampoline(e);
	});

	theLoop().run();

	EXPECT_EQ("test", e);
	MOL_TEST_ASSERT_CNTS(0, 0);
}

#endif

TEST_F(BasicTest, trimTest) {

	EXPECT_STREQ("",trim("").c_str());

	EXPECT_STREQ("",trim("\r\n").c_str());

	EXPECT_STREQ("abc",trim(" abc ").c_str());
	EXPECT_STREQ("abc",trim("abc ").c_str());
	EXPECT_STREQ("abc",trim(" abc").c_str());

	EXPECT_STREQ("a b c",trim(" a b c ").c_str());
}

TEST_F(BasicTest, Thenable) {

	int c = 0;

	nextTick( [&c]()
	{
		MOL_TEST_PRINT_CNTS();
		EXPECT_EQ(0,c);
		c++;
	});

	MOL_TEST_PRINT_CNTS();
	theLoop().run();

    EXPECT_EQ(1,c);
    MOL_TEST_ASSERT_CNTS(0,0);
}




TEST_F(BasicTest, AsyncTask) {

	int c = 0;

	{
		signal(SIGINT)
		.then([](int s){

		});

		task([&c](){
			EXPECT_EQ(0,c);
			c++;
		})
		.then([&c](){
			EXPECT_EQ(1,c);
			c++;
			theLoop().exit();
		});

		theLoop().run();
		std::cout << c << std::endl;
	}
    EXPECT_EQ(2,c);
    MOL_TEST_ASSERT_CNTS(0,0);
}


TEST_F(BasicTest, TcpServer) {

	std::string result;

	{
		signal(SIGINT)
		.then([](int s){

		});

		Connection::Ptr c;

		Listener listener;
		listener.bind(9876)
		.then([&c](Connection::Ptr client)
		{
			c = client;
			client->read()
			.then( [](Connection::Ptr client,std::string data)
			{
				return client->write(data);
			})
			.then([](Connection::Ptr client)
			{
			});
		});
		
		Connection::Ptr ptr;
		timeout([&result,&ptr,&listener]()
		{
			TcpConnection::connect("localhost", 9876)
			.then([&ptr](Connection::Ptr client)
			{
				ptr = client;
				return client->write("HELO WORLD");
			})
			.then([](Connection::Ptr client)
			{
				return client->read();
			})
			.then([&result,&listener](Connection::Ptr client, std::string data)
			{
				result = data;
				listener.cancel();
				theLoop().exit();
			});
		}, 0, 1250);
	
		
		theLoop().run();
	}

	EXPECT_EQ(result,std::string("HELO WORLD"));
	MOL_TEST_ASSERT_CNTS(0,0);
}


TEST_F(BasicTest, TcpServerReadN) {

	std::string result;
	{
		Connection::Ptr c;
		Connection::Ptr ptr(nullptr);

		Listener listener;
		listener.bind(9876)
		.then([&c](Connection::Ptr client)
		{
			c = client;
			client->read()
			.then( [](Connection::Ptr client,std::string data)
			{
				return client->write(data);
			})
			.then([](Connection::Ptr client)
			{
			});
		});


		timeout([&result,&ptr,&listener]()
		{
			TcpConnection::connect("localhost", 9876)
			.then([&ptr](Connection::Ptr client)
			{
				ptr = client;
				return client->write("HELO WORLD");
			})
			.then([](Connection::Ptr client)
			{
				return client->read(4);
			})
			.then([&result,&listener](Connection::Ptr client, std::string data)
			{
				result = data;
				listener.cancel();
				theLoop().exit();
				
			});
		}, 0, 50);
		theLoop().run();
	}

	EXPECT_EQ(result,std::string("HELO"));
	MOL_TEST_ASSERT_CNTS(0,0);
}

/* TODO fix after.h
TEST_F(BasicTest, TcpServerBoth) {

	std::string result;
	{

		std::vector<Connection::Ptr> clients;
		Connection::Ptr client1;
		Connection::Ptr client2;

		Listener listener;
		listener.bind(9876)
		.then([&clients](Connection::Ptr client)
		{
			clients.push_back(client);
			client->read()
			.then( [](Connection::Ptr client,std::string data)
			{
				std::cout << "got data: " << data << std::endl;
				return client->write(data);
			})
			.then([](Connection::Ptr client)
			{
				std::cout << "server done " << std::endl;
				client->close();
			});
		});

		after(
			[&client1](){
				auto p = promise<std::string>();


				TcpConnection::connect("localhost",9876)
				->then([&client1](Connection::Ptr client)
				{
					client1 = client;
					return client->write("HELO WORLD");
				})
				->then( [](Connection::Ptr client)
				{
					std::cout << "client 1 start read " << std::endl;
					return client->read(4);
				})
				->then( [p](Connection::Ptr client,std::string data)
				{
					std::cout << "client1 got data: " << data << std::endl;
					p->resolve(data);
					//client->close();
				});

				return p->future();
			}(),

			[&client2](){
				auto p = promise<std::string>();

				TcpConnection::connect("localhost",9876)
				->then([&client2](Connection::Ptr client)
				{
					client2 = client;
					return client->write("HELO WORLD");
				})
				->then( [](Connection::Ptr client)
				{
					std::cout << "client 2 start read " << std::endl;
					return client->read(5);
				})
				->then([p](Connection::Ptr client,std::string data)
				{
					std::cout << "client2 got data: " << data << std::endl;
					p->resolve(data);
				});

				return p->future();
			}()
		)
		->then([&result,&listener](std::string s1,std::string s2){

			std::cout << "cboth " << std::endl;

			EXPECT_EQ(s1,std::string("HELO"));
			EXPECT_EQ(s2,std::string("HELO "));

			std::ostringstream oss;
			oss << s2 << s1;
			result = oss.str();

			listener.cancel();
			theLoop().exit();
		});


		theLoop().run();
	}

	EXPECT_EQ(result,std::string("HELO HELO"));
	MOL_TEST_ASSERT_CNTS(0,0);
}
*/

#ifdef _WIN32_xx_DISABLED

TEST_F(BasicTest, SimplePipeClass)
{
	{
		timeout( []()
		{
			auto args = arguments("ls","-lah");

			Pipe::create()
			->pipe("/bin/ls",args)
			->then([](Pipe::Ptr pipe)
			{
				std::cout << pipe->stdout() << std::endl;
			})
			->otherwise([](const std::exception& ex)
			{
				std::cout << ex.what() << std::endl;
			});

		},1,0);
		theLoop().run();
	}
	MOL_TEST_ASSERT_CNTS(0,0);

}

TEST_F(BasicTest, SimplePipeClassCat)
{
	{
		timeout( []()
		{
			Pipe::create()
			->stdin("HELLO CAT")
			->pipe("/bin/cat") //,arguments("cat"))
			->then([](Pipe::Ptr pipe)
			{
				std::cout << pipe->stdout() << std::endl;
			})
			->otherwise([](const std::exception& ex)
			{
				std::cout << ex.what() << std::endl;
			});
		},1,0);
		theLoop().run();
	}
	MOL_TEST_ASSERT_CNTS(0,0);

}
#endif


int main(int argc, char **argv) 
{
	prio::Libraries<EventLoop,cryptoneat::SSLUser> init;
//	prio::init();
//	cryptoneat::SSLUser useSSL;

	::testing::InitGoogleTest(&argc, argv);
    int r = RUN_ALL_TESTS();

    return r;
}
