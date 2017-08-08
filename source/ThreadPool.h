#ifndef __TiBANK_THREAD_POOL_H__
#define __TiBANK_THREAD_POOL_H__

#include <boost/thread.hpp>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "Log.h"

enum ThreadStatus {
	kThreadInit = 1,
	kThreadRunning = 2,
	kThreadSuspend = 3,
	kThreadTerminating = 4,
	kThreadDead = 5,
};

struct ThreadObj {
	ThreadObj(enum ThreadStatus status):
		status_(status) {
	}
	enum ThreadStatus status_;
};

const static uint8_t kMaxiumThreadPoolSize = 200;

typedef boost::shared_ptr<boost::thread> ThreadTtr;
typedef boost::shared_ptr<ThreadObj> ThreadObjPtr;
typedef boost::function<void (ThreadObjPtr)> ThreadRunnable;

class ThreadPool: private boost::noncopyable,
		    		public boost::enable_shared_from_this<ThreadPool>{
public:
	explicit ThreadPool(uint8_t thread_num):
		thread_num_(thread_num_) {
	}

	void callable_wrapper(ThreadObjPtr ptr){
		// 先于线程工作之前的所有预备工作

		while (ptr->status_ == ThreadStatus::kThreadInit)
			::usleep(500*1000);

		func_(ptr);
	}

	virtual ~ThreadPool(){
		graceful_stop_tasks();
	}

	// thread_num 已经校验
	bool init(ThreadRunnable func, uint8_t thread_num) {

		if (thread_num != thread_num_) {
			log_trace("update thread_num_ %d -> %d", thread_num_, thread_num);
			thread_num_ = thread_num;
		}

		if (!func) {
			log_error("Invalid runnable object!");
			return false;
		}
		func_ = func; // record it

		for (int i=0; i<thread_num_; ++i) {
			ThreadObjPtr workobj(new ThreadObj(ThreadStatus::kThreadInit));
			if (!workobj) {
				log_error("create ThreadObj failed!");
				return false;
			}
			ThreadTtr worker(new boost::thread(boost::bind(&ThreadPool::callable_wrapper, shared_from_this(), workobj)));
			if (!worker || !workobj) {
				log_error("create thread failed!");
				return false;
			}

			workers_[worker] = workobj;
			log_trace("Created Task: #%d ...", i);
		}

		return true;
	}

	void start_tasks() {
		std::map<ThreadTtr, ThreadObjPtr>::iterator it;

		for (it = workers_.begin(); it != workers_.end(); ++it) {
			it->second->status_ = ThreadStatus::kThreadRunning;
		}
	}

	void suspend_tasks() {
		std::map<ThreadTtr, ThreadObjPtr>::iterator it;

		for (it = workers_.begin(); it != workers_.end(); ++it) {
			it->second->status_ = ThreadStatus::kThreadSuspend;
		}
	}

	int incr_thread_pool() {
		return spawn_task(1);
	}

	int decr_thread_pool() {
		return reduce_task(1);
	}

	int resize_thread_pool(uint8_t num){

		int diff = num - workers_.size();
		if (diff == 0) {
			return 0;
		} else if (diff > 0) {
			return spawn_task(diff);
		} else if (diff < 0) {
			return reduce_task(::abs(diff));
		}

		return 0;
	}

	uint8_t get_thread_pool_size() {
		return static_cast<uint8_t>(workers_.size());
	}

	void immediate_stop_tasks(){
		std::map<ThreadTtr, ThreadObjPtr>::iterator it;

		for (it = workers_.begin(); it != workers_.end(); ++it) {
			it->second->status_ = ThreadStatus::kThreadTerminating;
		}
		thread_num_ = 0;
		return;
	}

	bool graceful_stop(ThreadTtr worker, uint8_t timed_seconds) {
		std::map<ThreadTtr, ThreadObjPtr>::iterator it;

		it = workers_.find(worker);
		if (it == workers_.end()) {
			log_error("Target worker not found!");
			return false;
		}

		// 处于ThreadStatus::kThreadInit状态的线程此时也可以进来
		enum ThreadStatus old_status = it->second->status_;
		it->second->status_ = ThreadStatus::kThreadTerminating;
		if (timed_seconds) {
			const boost::system_time timeout = boost::get_system_time() + boost::posix_time::milliseconds(timed_seconds * 1000);
			it->first->timed_join(timeout);
		} else {
			it->first->join();
		}

		if (it->second->status_ != ThreadStatus::kThreadDead) {
			log_error("gracefulStop failed!");
			it->second->status_ = old_status; // 恢复状态

			return false;
		}

		// release this thread object
		thread_num_ --;
		workers_.erase(worker);
		return true;
	}

	void graceful_stop_tasks(){
		std::map<ThreadTtr, ThreadObjPtr>::iterator it;

		while (!workers_.empty()) {
			it = workers_.begin();
			graceful_stop(it->first, 0);
		}

		log_trace("Good! thread pool clean up down!");
	}

private:
	int spawn_task(uint8_t num){

		for (int i = 0; i < num; ++i) {
			ThreadObjPtr workobj(new ThreadObj(ThreadStatus::kThreadInit));
			if (!workobj) {
				log_error("create ThreadObj failed!");
				return -1;
			}
			ThreadTtr worker(new boost::thread(boost::bind(&ThreadPool::callable_wrapper, shared_from_this(), workobj)));
			if (!worker || !workobj) {
				log_error("create thread failed!");
				return -1;
			}

			workers_[worker] = workobj;
			thread_num_ ++;
			log_trace("Created Additional Task: #%d ...", i);

            log_trace("Start Additional Task: #%d ...", i);
            workobj->status_ = ThreadStatus::kThreadRunning;
		}

		log_trace("Current ThreadPool size: %d", thread_num_);
		return 0;
	}

	int reduce_task(uint8_t num) {
		size_t max_try = 50;
		size_t currsize = workers_.size();
		std::map<ThreadTtr, ThreadObjPtr>::iterator it;

		do {
			while (!workers_.empty()) {

				it = workers_.begin();
				graceful_stop(it->first, 10);
				max_try --;

				if (!max_try)
					break;

				if ((currsize - workers_.size()) >= num)
					break;
			}
		} while (0);

		log_trace("Current ThreadPool size: %d", thread_num_);
		return ((currsize - workers_.size()) >= num) ? 0 : -1;
	}

private:
	ThreadRunnable func_;

	boost::mutex lock_;
	uint8_t thread_num_;
	std::map<ThreadTtr, ThreadObjPtr> workers_;
};


#endif // __TiBANK_THREAD_POOL_H__
