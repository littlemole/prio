#ifndef INCLUDE_PROMISE_LIBEVENT_RESOURCEPOOL_H_
#define INCLUDE_PROMISE_LIBEVENT_RESOURCEPOOL_H_

#include <set>
#include <map>
#include <list>

#include "priocpp/common.h"
#include "priocpp/api.h"
#include "priocpp/task.h"


namespace prio {


template<class T>
class Resource
{
public:
	typedef T type;

	Resource(type* t)
		: ptr_(t), valid_(true)
	{}

	~Resource()
	{}

	void markAsInvalid()
	{
		valid_ = false;
	}

	bool valid()
	{
		return valid_;
	}

	operator T* ()
	{
		return ptr_;
	}

	T* operator->()
	{
		return ptr_;
	}

private:
	type* ptr_;
	bool valid_;
};

template<class L>
class ResourcePool {
public:

	typedef typename L::type type;
	typedef std::shared_ptr<Resource<type>> ResourcePtr;
	typedef repro::Promise<ResourcePtr> PromiseType;
	typedef repro::Future<ResourcePtr> FutureType;

	ResourcePool()
		:min_capacity_(4)
	{

	}

	ResourcePool(int c)
		:min_capacity_(c)
	{

	}

	~ResourcePool()
	{
		shutdown();
	}

	void shutdown()
	{
		shutdown_ = true;
		for( auto it = unused_.begin(); it != unused_.end(); it++)
		{
			while( (*it).second.size() > 0 )
			{
				auto jt = (*it).second.begin();
				L::free(*jt);
				(*it).second.erase(jt);
			}
		}
		for( auto it = used_.begin(); it != used_.end(); it++)
		{
			while( (*it).second.size() > 0 )
			{
				auto jt = (*it).second.begin();
				L::free(*jt);
				(*it).second.erase(jt);
			}
		}

		used_.clear();
		unused_.clear();
	}

	FutureType get(const std::string url)
	{
		auto p = repro::promise<ResourcePtr>();

std::cout << "resourcepool get pending: " << pending_ << std::endl;

		if( unused_.count(url) > 0)
		{
			if(!unused_[url].empty())
			{
				type* r = *(unused_[url].begin());
				unused_[url].erase(r);
				used_[url].insert(r);

				return resolved<ResourcePtr>( make_ptr(url, r) );
			}
		}

		if( used_[url].size() + pending_ < min_capacity_ )
		{
			pending_++;
			L::retrieve(url)
			.then([this,p,url](type* r)
			{
				used_[url].insert(r);
				p.resolve( make_ptr(url,r) );
				pending_--;
			})
			.otherwise( [this,p](const std::exception& ex)
			{
				p.reject(ex);
				pending_--;
			});
		}
		else
		{
			waiting_.push_back(p);
		}
		return p.future();
	}

private:


	void collect(const std::string& url, Resource<type>* t)
	{
		if(shutdown_ == true) return;

		if ( !waiting_.empty() )
		{
			PromiseType p = waiting_.front();
			waiting_.pop_front();
			p.resolve(make_ptr(url,*t));
			return;
		}

		if (used_[url].count(*t) > 0 )
		{
			used_[url].erase(*t);
			unused_[url].insert(*t);

			while(unused_[url].size() > min_capacity_)
			{
				type* tmp = *(unused_[url].begin());
				unused_[url].erase(tmp);
				L::free(tmp);
			}
		}
		else
		{
			L::free(*t);
		}
	}

	void release(const std::string& url, Resource<type>* t)
	{
		if(shutdown_ == true) return;

		if (used_[url].find(*t) != used_[url].end() )
		{
			used_[url].erase(*t);
		}
		L::free(*t);
	}


	ResourcePtr make_ptr(std::string url, type* r)
	{
		return ResourcePtr(
			new Resource<type>(r),
			[this,url](Resource<type>* t)
			{
				if(t->valid())
				{
					collect(url,t);
				}
				else
				{
					release(url,t);
				}
				delete t;
			}
		);
	}

	bool shutdown_ = false;
	unsigned int min_capacity_ = 4;
	int pending_ = 0;

	std::map<std::string,std::set<type*>> used_;
	std::map<std::string,std::set<type*>> unused_;
	std::list<PromiseType> waiting_;
};



} 

#endif /* INCLUDE_PROMISE_LIBEVENT_RESOURCEPOOL_H_ */
