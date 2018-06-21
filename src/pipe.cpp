#include <functional>

#include "priocpp/pipe.h"
#include "priocpp/loop.h"

#ifdef _WIN32_xx_DISABLED

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

namespace prio      {



Pipe::Pipe()
	: result_(0), pid_(0),written_(0)
{}

Pipe::~Pipe()
{}


Pipe::Ptr Pipe::create()
{
	auto ptr = std::make_shared<Pipe>();
	return ptr;
}

Pipe::Ptr Pipe::stdin(const std::string& s)
{
	stdin_ = s;
	return shared_from_this();
}


Future<Pipe::Ptr> Pipe::pipe_impl(const std::string& path, char* const* args, char* const* env)
{
	auto p = promise<Pipe::Ptr>();

	if (pipe2(filedes_, O_NONBLOCK|O_CLOEXEC) == -1)
	{
		throw Ex("create pipe failed");
	}
	pid_ = fork();
	if (pid_ == -1)
	{
		throw Ex("fork failed");
	}
	else if (pid_ == 0)
	{
		run_child(path,args,env);
	}
	else
	{
		run_parent(p);
	}

	return p.future();
}

std::string Pipe::stdout()
{
	return stdout_oss_.str();
}

std::string Pipe::stderr()
{
	return stderr_oss_.str();
}

int Pipe::result()
{
	return result_;
}


void Pipe::run_child(const std::string& path, char* const* args, char* const* env)
{
	  while ((dup2(filedes_[0], STDIN_FILENO)  == -1) && (errno == EINTR)) {}
	  while ((dup2(filedes_[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
	  close(filedes_[0]);
	  close(filedes_[1]);
	  close(STDIN_FILENO);
	  close(STDERR_FILENO);
	  execve(path.c_str(), args, env);

	  throw Ex("execl child failed");
}

void Pipe::run_parent(Promise<Pipe::Ptr> p)
{
	write()
	.then( [] (Pipe::Ptr pipe)
	{
		return pipe->read();
	})
	.then( [p] (Pipe::Ptr pipe)
	{
		p.resolve(pipe);
	})
	.otherwise([p](const std::exception& ex)
	{
		p.reject(ex);
	});
}

Future<Pipe::Ptr> Pipe::read()
{
	auto p = promise<Pipe::Ptr>();

	auto ptr = shared_from_this();

	close(filedes_[1]);

	e_ = onEvent((socket_t)(filedes_[0]),(short)EV_READ|EV_PERSIST)
	->callback( [p,ptr] (int fd, short what)
	{
		if(what == EV_TIMEOUT)
		{
			p.reject(IoTimeout("IO timeout in Pipe::read"));
			return;
		}

		while(true)
		{
			char buf[1024];
			auto len = Socket(fd).read(	buf , 1024);

			if(len > 0)
			{
				ptr->stdout_oss_.write(buf,len);
				continue;
			}
			else if(len<=0)
			{
				if( (len == -1) && (errno == EWOULDBLOCK) )
				{
					// do nothing
				}
				else
				{
					waitpid(ptr->pid_, NULL, 0);

					close(ptr->filedes_[0]);

					p.resolve(ptr);

					ptr->e_->dispose();
				}
			}
			break;
		}
	})
	->add();
	return p->future();
}

Future<Pipe::Ptr>::Ptr Pipe::write()
{
	auto ptr = shared_from_this();

	if(stdin_.empty())
	{
		return resolved<Pipe::Ptr>(ptr);
	}

	auto p = promise<Pipe::Ptr>();

	written_ = 0;

	e_ = onEvent((socket_t)(filedes_[1]),(short)EV_WRITE|EV_PERSIST)
	->callback( [p,ptr] (int fd, short what)
	{
		if(what == EV_TIMEOUT)
		{
			p.reject(IoTimeout("IO timeout in Pipe::write"));
			return;
		}

		while(true)
		{
			auto len = Socket(fd).write(ptr->stdin_.c_str(), ptr->stdin_.size()-ptr->written_);

			if(len > 0)
			{
				ptr->written_ += len;
				if ( ptr->written_ >= ptr->stdin_.size() )
				{
					auto tmp = ptr->e_;
					p.resolve(ptr);
					break;
				}
				continue;
			}
			else if(len<=0)
			{
				if( (len == -1) && (errno == EWOULDBLOCK) )
				{
					// do nothing
				}
				else
				{
					waitpid(ptr->pid_, NULL, 0);

					close(ptr->filedes_[0]);
					close(ptr->filedes_[1]);

					p.reject(Ex("IoEx in Pipe::write"));

					ptr->e_->dispose();
				}
			}
			break;
		}
	})
	->add();
	return p.future();
}


}

#endif
