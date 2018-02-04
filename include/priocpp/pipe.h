#ifndef MOL_PROMISE_LIBEVENT_PIPE_DEF_GUARD_DEFINE_
#define MOL_PROMISE_LIBEVENT_PIPE_DEF_GUARD_DEFINE_

#ifndef _WIN32

#include "reprocpp/promise.h"
#include "priocpp/api.h"

namespace prio      {


template<class ... Args>
class Arguments
{
public:
	Arguments(Args&& ... args)
		: args_{args...,(const char*)NULL}
	{
	}

	std::vector<const char*> get()
	{
		return args_;
	}

private:
	std::vector<const char*> args_;
};


template<class ... Args>
auto arguments(Args&& ... args)
{
	return Arguments<Args...>(args...);
}

template<class ... Args>
auto environment(Args&& ... args)
{
	return Arguments<Args...>(args...);
}

class Pipe : public std::enable_shared_from_this<Pipe>
{
public:

	typedef std::shared_ptr<Pipe> Ptr;

	Pipe();
	~Pipe();

	static Ptr create();

	Ptr stdin(const std::string& s);

	Future<Pipe::Ptr> pipe(const std::string& path )
	{
		return pipe_impl( path );
	}

	template<class A>
	Future<Pipe::Ptr> pipe(const std::string& path, A&& a )
	{
		args_ = a.get();
		return pipe_impl( path, ( char* const*) &(args_[0]) );
	}

	template<class A, class E>
	Future<Pipe::Ptr> pipe(const std::string& path, A&& args, E&& env )
	{
		args_ = args.get();
		env_ = env.get();
		return pipe_impl( path,  (char* const*) &(args_[0]),  (char* const*) &(env_[0]) );
	}

	std::string stdout();
	std::string stderr();
	int result();

private:

	Future<Pipe::Ptr> pipe_impl(const std::string& path,   char* const* args = NULL,  char* const* env = NULL);

	void run_child(const std::string& path,  char* const* args,  char* const* env);

	void run_parent(repro::Promise<Pipe::Ptr> p);

	Future<Pipe::Ptr> read();
	Future<Pipe::Ptr> write();


	int result_;
	pid_t pid_;
	int filedes_[2];
	std::ostringstream stdout_oss_;
	std::ostringstream stderr_oss_;
	std::string stdin_;
	size_t written_;

	std::vector<const char*> args_;
	std::vector<const char*> env_;
};


}

#endif
#endif
