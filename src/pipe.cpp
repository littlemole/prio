#include <functional>

#include "priocpp/pipe.h"
#include "priocpp/loop.h"

#ifndef _WIN32

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

using namespace repro;

namespace prio      {



Pipe::Pipe()
	: result_(0), pid_(0),written_(0), promise_(promise<Pipe::Ptr>())
{}

Pipe::~Pipe()
{}


Pipe::Ptr Pipe::create()
{
	auto ptr = std::make_shared<Pipe>();
	ptr->self_ = ptr;
	return ptr;
}

Pipe::Ptr Pipe::stdin(const std::string& s)
{
	stdin_ = s;
	return self_;
}


Future<Pipe::Ptr> Pipe::pipe_impl(const std::string& path, char* const* args, char* const* env)
{
	if (pipe2(filedes_, O_NONBLOCK|O_CLOEXEC) == -1)
	{
		self_.reset();
		throw Ex("create pipe failed");
	}
	pid_ = fork();
	if (pid_ == -1)
	{
		self_.reset();
		throw Ex("fork failed");
	}
	else if (pid_ == 0)
	{
		run_child(path,args,env);
	}
	else
	{
		run_parent();
	}

	return promise_.future();
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

	  std::cout << "exec chld " << path << std::endl;
	  execve(path.c_str(), args, env);

	  throw Ex("execl child failed");
}

void Pipe::run_parent()
{
	written_ = 0;

	auto pw = promise<>();
	auto pr = promise<>();

	write(pw)
	.then( [this,pr] ()
	{
		close(filedes_[1]);
		return read(pr);
	})
	.then( [this] ()
	{
		promise_.resolve(self_);
		self_.reset();
	})
	.otherwise([this](const std::exception& ex)
	{
		promise_.reject(ex);
		self_.reset();
	});
}

Future<> Pipe::read(repro::Promise<> p)
{
	io_.onRead((socket_t)(filedes_[0]))
	.then([this,p]()
	{
		while(true)
		{
			std::cout << "READ " << std::endl;

			char buf[1024];
			auto len = ::read(filedes_[0],buf,1024);

			if(len > 0)
			{
				std::cout << "READ " << len << std::endl;
				stdout_oss_.write(buf,len);
				continue;
			}
			else if(len<=0)
			{
				if( (len == -1) && (errno == EWOULDBLOCK) )
				{
					std::cout << "EWOULDBLOCK " << std::endl;
					this->read(p);
				}
				else
				{
					std::cout << "DOME READ " << len << " " << errno << std::endl;

					waitpid(this->pid_, NULL, 0);

					std::cout << "WAITPID READ " << std::endl;

					close(this->filedes_[0]);

					p.resolve();

					//ptr.reset();
				}
			}
			break;
		}

	});

	return p.future();
}

Future<> Pipe::write(repro::Promise<> p)
{
	if(stdin_.empty())
	{
		return resolved<>();
	}

	io_.onWrite(filedes_[1])
	.then([this,p]()
	{
		while(true)
		{
			std::cout << "writing " << std::endl;
			auto len = ::write(filedes_[1],this->stdin_.c_str()+this->written_, this->stdin_.size()-this->written_);

			if(len > 0)
			{
				this->written_ += len;
				if ( this->written_ >= this->stdin_.size() )
				{
					std::cout << "written " << std::endl;
					p.resolve();
					break;
				}
				continue;
			}
			else if(len<=0)
			{
				if( (len == -1) && (errno == EWOULDBLOCK) )
				{
					std::cout << "EWOULDBLOCK " << std::endl;

					this->write(p);
					// do nothing
				}
				else
				{
					std::cout << "E WRITE " << std::endl;

					waitpid(this->pid_, NULL, 0);

					close(this->filedes_[0]);
					close(this->filedes_[1]);

					p.reject(Ex("IoEx in Pipe::write"));

					//ptr->e_->dispose();
				}
			}
			break;
		}
	});

	return p.future();
}


}

#endif
