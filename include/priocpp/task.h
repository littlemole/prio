#ifndef _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_HANDLER_TASK_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MOD_HTTP_REQUEST_HANDLER_TASK_DEF_GUARD_

#include <atomic>

#include "priocpp/api.h"
#include "priocpp/threadpool.h"
#include <iostream>

namespace prio  		{


inline void task_synchronize_thread(std::atomic<bool>& running)
{
	bool d = running.load();
	while (d)
	{
		std::cout << "task_synchronize_thread " << d << std::endl;
		bool expected = true;
		if (running.compare_exchange_weak(expected, false))
		{
			break;
		}
		d = running.load();
	}
	std::cout << "task_synchronize_main done " << std::endl;
}


inline void task_synchronize_main(std::atomic<bool>& running)
{
	bool d = running.load();
	while (d)
	{
		std::cout << "task_synchronize_main " << d << std::endl;
		d = running.load();
	}	

	std::cout << "task_synchronize_main done"  << std::endl;	
}


template<class T>
class Task
{};

template<class R,class T>
class Task<R(T)>
{
public:
	static repro::Future<R> exec(T t, ThreadPool& pool  )
	{
		auto p = repro::promise<R>();
		auto running = std::make_shared<std::atomic<bool>>(true);

		nextTick( [t,p,running,&pool]()
		{
			pool.enqueue( [t,p,running]()
			{
				try
				{
					std::shared_ptr<R> r = std::make_shared<R>(t());

					task_synchronize_thread(*running);

					std::cout << "task next tick next" << *running << std::endl;
					
					nextTick( [p,r,running] ()
					{
						std::cout << "task next tick inside" << *running << std::endl;
						task_synchronize_main(*running);

						std::cout << "task resolve " << typeid(R).name() << std::endl;
						p.resolve(std::move(*r));
					});
				}
				catch(...)
				{
					std::exception_ptr eptr = std::current_exception();

					std::cout << "task ex " << *running << std::endl;

					task_synchronize_thread(*running);

					nextTick( [p,eptr,running] ()
					{
						std::cout << "task ex nexttick " << *running << std::endl;
						task_synchronize_main(*running);
						p.reject(eptr);
					});
				}
			});
		});
		return p.future();
	}
};


template<class T>
class Task<void(T)>
{
public:

	static repro::Future<> exec(T t, ThreadPool& pool  )
	{
		auto p = repro::promise<>();
		auto running = std::make_shared<std::atomic<bool>>(true);

		nextTick( [t,p,running,&pool]()
		{
			pool.enqueue( [t,p,running]()
			{
				try
				{
					t();

					task_synchronize_thread(*running);

					nextTick( [p,running] ()
					{
						task_synchronize_main(*running);

						p.resolve();
					});
				}
				catch(...)
				{
					std::exception_ptr eptr = std::current_exception();

					task_synchronize_thread(*running);

					nextTick( [p,eptr,running] ()
					{
						task_synchronize_main(*running);

						p.reject(eptr);
					});
				}
			});
		});
		return p.future();
	}
};



template<class T, typename std::enable_if<!repro::ReturnsVoid<T>::value>::type* = nullptr>
auto task(T t, ThreadPool& pool = thePool() ) -> repro::Future<decltype(t())>
{
	return Task<decltype(t())(T)>::exec(t,pool);
}

template<class T, typename std::enable_if<repro::ReturnsVoid<T>::value>::type* = nullptr>
auto task(T t, ThreadPool& pool = thePool() ) -> repro::Future<>
{
	return Task<void(T)>::exec(t,pool);
}



} // close namespaces

#endif

