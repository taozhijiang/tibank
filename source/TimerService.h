#ifndef __TiBANK_TIMER_SERVICE__
#define __TiBANK_TIMER_SERVICE__

#include <event.h>

#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "EQueue.h"
#include "ThreadPoolHelper.h"


// 提供定时回调接口服务
typedef boost::function<void ()> TimerEventCallable;


struct TimerTask {
	TimerTask(TimerEventCallable func, struct timeval tv,
			  bool persist, bool fast):
		persist_(persist), fast_(fast),
		tv_(tv), func_(func), dead_(false) {
	}

	struct event ev_timer_;
	bool persist_;
	bool fast_;

	struct timeval tv_;
	TimerEventCallable func_;

	bool dead_; // 等待被清理，一般是单次触发完成，
				// 或者显式调用取消事件接口

	~TimerTask () {
	}
};

typedef boost::shared_ptr<TimerTask> TimerTaskPtr;

class TimerService: public boost::noncopyable,
					  public boost::enable_shared_from_this<TimerService> {

	friend void c_timer_cb(int fd, short what, void *arg);
public:
	TimerService():
		timer_defer_(1) {
	}

	~TimerService(){
		event_base_free(ev_base_);
	}

	bool init();

	int start_timer();
	int stop_graceful();

	// persist 是否永久触发
	// fast 是否内联执行还是排队到timerBfRun中执行
	// 成功返回句柄，可以持有删除这个任务
	int64_t register_timer_task(TimerEventCallable func, int64_t msec,
							 bool persist = true, bool fast = true);
	bool revoke_timer_task(int64_t index);

private:
	void timer_cb (int fd, short what, void *arg);

	void timer_run();
	void timer_defer_run(ThreadObjPtr ptr);

	TimerTaskPtr find_task(int64_t index);
	int add_task(TimerTaskPtr pTask);
	void purge_dead_task();

private:
	// base loop 主线程
	boost::thread timer_thread_;

public:
	// 目前而言最好使用单个工作线程串行执行，否则注册的任务可能会并行执行导致不可预料的结果
	ThreadPoolHelper timer_defer_;

private:
	EQueue<TimerEventCallable> defer_ready_;

	boost::mutex tasks_lock_;
	std::map<int64_t, TimerTaskPtr> tasks_;  // pointer cast
	struct event_base *ev_base_;
};

#endif // __TiBANK_TIMER_SERVICE__
