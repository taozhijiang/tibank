#ifndef __TiBANK_THREAD_POOL_HELPER_H__
#define __TiBANK_THREAD_POOL_HELPER_H__

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "ThreadPool.h"

class ThreadPoolHelper: private boost::noncopyable {
public:
	ThreadPoolHelper():
		thread_num_init_(1) {
		threads_ptr_ = boost::make_shared<ThreadPool>(thread_num_init_);
		if (!threads_ptr_) {
			log_error("Init create thread pool failed, CRITICAL!!!!");
			::abort();
		}
	}

	explicit ThreadPoolHelper(uint8_t thread_num):
		thread_num_init_(thread_num) {
		if (thread_num_init_ == 0 || thread_num_init_ > kMaxiumThreadPoolSize ){
			log_error("Invalid thread_number %d,, CRITICAL !!!", thread_num_init_);
			::abort();
		}

		threads_ptr_ = boost::make_shared<ThreadPool>(thread_num_init_);
		if (!threads_ptr_) {
			log_error("Init create thread pool failed, CRITICAL!!!!");
			::abort();
		}
	}

	virtual ~ThreadPoolHelper(){
		threads_ptr_->graceful_stop_tasks();
	}

	bool init_threads(ThreadRunnable func, uint8_t thread_num) {
		thread_num_init_ = thread_num;
		if (thread_num_init_ == 0 || thread_num_init_ > kMaxiumThreadPoolSize ){
			return false;
		}

		return threads_ptr_->init(func, thread_num_init_);
	}

	bool init_threads(ThreadRunnable func) {
		return threads_ptr_->init(func, thread_num_init_);
	}

	void start_threads() {
		return threads_ptr_->start_tasks();
	}

	void suspend_threads() {
		return threads_ptr_->suspend_tasks();
	}

	void graceful_stop_threads() {
		return threads_ptr_->graceful_stop_tasks();
	}

	void immediate_stop_threads() {
		return threads_ptr_->immediate_stop_tasks();
	}

	int resize_threads(uint8_t thread_num) {
		return threads_ptr_->resize_thread_pool(thread_num);
	}

	size_t get_thread_pool_size() {
		return threads_ptr_->get_thread_pool_size();
	}

private:
	uint8_t thread_num_init_;
	boost::shared_ptr<ThreadPool> threads_ptr_;
};


#endif // __TiBANK_THREAD_POOL_HELPER_H__
