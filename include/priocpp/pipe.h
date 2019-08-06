#ifndef MOL_PROMISE_LIBEVENT_PIPE_DEF_GUARD_DEFINE_
#define MOL_PROMISE_LIBEVENT_PIPE_DEF_GUARD_DEFINE_

/**
 * \file pipe.h
 */

#ifndef _WIN32

#include "reprocpp/promise.h"
#include "priocpp/api.h"

namespace prio      {


template<class ... Args>
class Arguments
{
public:
	Arguments(Args ... args)
		: args_{args...,(const char*)NULL}
	{
	}

	std::vector<const char*> get() const
	{
		return args_;
	}

private:
	std::vector<const char*> args_;
};


template<class ... Args>
auto arguments(Args ... args)
{
	return Arguments<Args...>(args...);
}

template<class ... Args>
auto environment(Args ... args)
{
	return Arguments<Args...>(args...);
}

/**
 * \brief unix pipe implementation 
 *
 * open a process and control its input and output asynchronously.
 **/
class Pipe : public std::enable_shared_from_this<Pipe>
{
public:

	//! a Pipe::Ptr is a std::shared_ptr<Pipe>
	typedef std::shared_ptr<Pipe> Ptr;

	Pipe();
	~Pipe();

	//! create a Pipe as a shared ptr
	static Ptr create();

	//! specify stdin for piped process
	Ptr stdin(const std::string& s);

	//! specify path for subprocess to execute
	//! once the piped process has finished, the future will be resolved
	repro::Future<Pipe::Ptr> pipe(const std::string& path )
	{
		return pipe_impl( path );
	}


	//! specify path for subprocess to execute and add arguments
	//! once the piped process has finished, the future will be resolved
	template<class A>
	repro::Future<Pipe::Ptr> pipe(const std::string& path, A&& a )
	{
		args_ = a.get();
		return pipe_impl( path, ( char* const*) &(args_[0]) );
	}

	//! specify path for subprocess to execute, add arguments and specify env** vector
	//! once the piped process has finished, the future will be resolved
	template<class A>
	repro::Future<Pipe::Ptr> pipe(const std::string& path, A&& a, char ** env )
	{
		args_ = a.get();
		return pipe_impl( path, ( char* const*) &(args_[0]), env );
	}

	//! specify path for subprocess to execute, add arguments and specify environment as std::vector
	//! once the piped process has finished, the future will be resolved
	template<class A, class E>
	repro::Future<Pipe::Ptr> pipe(const std::string& path, A&& args, E&& env )
	{
		args_ = args.get();
		env_ = env.get();
		return pipe_impl( path,  (char* const*) &(args_[0]),  (char* const*) &(env_[0]) );
	}

	//! get the piped processes stdout
	std::string stdout();
	//! get the piped processes stderr
	std::string stderr();

	//! exit code from piped process
	int result();

private:

	repro::Future<Pipe::Ptr> pipe_impl(const std::string& path,   char* const* args = NULL,  char* const* env = NULL);

	void run_child(const std::string& path,  char* const* args,  char* const* env);

	void run_parent();

	repro::Future<> read(repro::Promise<>);
	repro::Future<> write(repro::Promise<>);


	int result_;
	pid_t pid_;
	int filedes_[2];
	std::ostringstream stdout_oss_;
	std::ostringstream stderr_oss_;
	std::string stdin_;
	size_t written_;

	std::vector<const char*> args_;
	std::vector<const char*> env_;

	IO io_;
	std::shared_ptr<Pipe> self_;
	repro::Promise<Pipe::Ptr> promise_;
};


}

#endif
#endif
