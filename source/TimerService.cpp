#include <iostream>
#include <evutil.h>

#include "General.h"
#include "Log.h"
#include "Utils.h"
#include "TimerService.h"

bool TimerService::init() {

    ev_base_ = event_base_new();
    if (!ev_base_) {
        log_err("Creating event_base failed!");
        return false;
    }

    log_info("Current Using Method: %s", event_base_get_method(ev_base_)); // epoll

    // add purge task
    if (register_timer_task(boost::bind(&TimerService::purge_dead_task, this), 5*1000, true, true) == 0) {
        log_err("Register purge task failed!");
        return false;
    }

    return true;
}

void c_timer_cb(int fd, short what, void *arg) {

    auto self = helper::request_timer_service();
    self->timer_cb(fd, what, arg);
}

void TimerService::timer_cb (int fd, short what, void *arg) {

    if (fd != -1 || !(what & (EV_READ | EV_TIMEOUT) ) ) {
        log_err("Invalid check: %d, %d, %d, %d ", fd, what, EV_READ, EV_TIMEOUT);
        return;
    }

    TimerTaskPtr task_ptr = find_task(reinterpret_cast<int64_t>(arg));
    if (!task_ptr) {
        return;
    }

    if (task_ptr->fast_) { // 肯定是串行的
        task_ptr->func_();
        if (!task_ptr->persist_) {
            evtimer_del(&task_ptr->ev_timer_);
            task_ptr->dead_ = true;
        } else {
            evtimer_add(&task_ptr->ev_timer_, &task_ptr->tv_);
        }
    } else {
        defer_ready_.PUSH(task_ptr->func_); // func_是拷贝的，无所谓
        if (!task_ptr->persist_) {
            evtimer_del(&task_ptr->ev_timer_);
            task_ptr->dead_ = true;
        } else {
            evtimer_add(&task_ptr->ev_timer_, &task_ptr->tv_);
        }
    }
}


void TimerService::timer_run() {

    // EVLOOP_NO_EXIT_ON_EMPTY not support on 1.x branch, so
    // we must register at least forever task to avoid event_base_loop exit

    int nResult = event_base_loop(ev_base_, 0);
    log_err("event_base_loop terminating here with: %d", nResult);
}

void TimerService::timer_defer_run(ThreadObjPtr ptr){

    std::stringstream ss_id;
    ss_id << boost::this_thread::get_id();
    log_info("TimerService thread %s is about to work... ", ss_id.str().c_str());

    TimerEventCallable func;

    while (true) {

        if (unlikely(ptr->status_ == ThreadStatus::kThreadTerminating)) {
            break;
        }

        // 线程启动
        if (unlikely(ptr->status_ == ThreadStatus::kThreadSuspend)) {
            ::usleep(500*1000);
            continue;
        }

        if (!defer_ready_.POP(func, 3 * 1000 /*3s*/)) {
            // yk_api::log_info("timer task timeout return!");
            continue;
        }

        if (!func){
            log_err("timer task empty func return!");
            continue;
        }

        func(); // call it
    }

    ptr->status_ = ThreadStatus::kThreadDead;
    log_info("TimerService thread %s is about to terminate ... ", ss_id.str().c_str());

    return;
}

int TimerService::start_timer(){

    timer_thread_ = boost::thread(boost::bind(&TimerService::timer_run, this));

    if (! timer_defer_.init_threads(boost::bind(&TimerService::timer_defer_run, this,_1))) {
        log_err("TimerService::init failed!");
        return -1;
    }

    timer_defer_.start_threads();

    return 0;
}

int TimerService::stop_graceful() {
    {
        std::map<int64_t, TimerTaskPtr>::iterator it, tmp;
        boost::lock_guard<boost::mutex> lock(tasks_lock_);

        for (it = tasks_.begin(); it != tasks_.end(); /**/ ) {
            evtimer_del(&it->second->ev_timer_);
            log_info("purge task %ld", it->first);
            tmp = it ++;
            tasks_.erase(tmp);
        }
    }

    log_info("CURRENT PENDING WORK: %lu", defer_ready_.SIZE());
    timer_defer_.graceful_stop_threads();

    return 0;
}

int TimerService::join() {
    timer_defer_.join_threads();
    return 0;
}

int64_t TimerService::register_timer_task(TimerEventCallable func, int64_t msec,
                                           bool persist, bool fast) {

    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_usec = (msec % 1000) * 1000;
    tv.tv_sec = msec / 1000;

    TimerTaskPtr task_ptr = std::make_shared<TimerTask>(func, tv, persist, fast);
    if (!task_ptr) {
        return 0;
    }

    evtimer_set(&task_ptr->ev_timer_, c_timer_cb, task_ptr.get());
    event_base_set(ev_base_, &task_ptr->ev_timer_);
    add_task(task_ptr);
    evtimer_add(&task_ptr->ev_timer_, &task_ptr->tv_);

    return reinterpret_cast<int64_t>(task_ptr.get());
}

bool TimerService::revoke_timer_task(int64_t index) {
    TimerTaskPtr ret;

    boost::lock_guard<boost::mutex> lock(tasks_lock_);
    std::map<int64_t, TimerTaskPtr>::iterator it = tasks_.find(index);
    if (it != tasks_.end()) {
        evtimer_del(&it->second->ev_timer_);
    }
    return !!tasks_.erase(index);
}

TimerTaskPtr TimerService::find_task(int64_t index) {
    TimerTaskPtr ret;

    boost::lock_guard<boost::mutex> lock(tasks_lock_);
    std::map<int64_t, TimerTaskPtr>::iterator it = tasks_.find(index);
    if (it != tasks_.end()) {
        ret = it->second;
    }

    return ret;
}

int TimerService::add_task(TimerTaskPtr task_ptr) {
    if (!task_ptr)
        return -1;

    int64_t index = reinterpret_cast<int64_t>(task_ptr.get());
    boost::lock_guard<boost::mutex> lock(tasks_lock_);
    std::map<int64_t, TimerTaskPtr>::iterator it = tasks_.find(index);
    if (it != tasks_.end()) {
        log_err("The timer task already found: %ld", reinterpret_cast<int64_t>(task_ptr.get()));
        return -1;
    }

    tasks_[reinterpret_cast<int64_t>(task_ptr.get())] = task_ptr; // add here

    return 0;
}

// 删除dead_的定时任务，这个函数只可能在主线程中串行执行，
// bf线程的执行函数都是拷贝构造的，所以不会有安全问题
void TimerService::purge_dead_task(){

    // log_debug("purge_dead_task check ...");

    boost::lock_guard<boost::mutex> lock(tasks_lock_);
    std::map<int64_t, TimerTaskPtr>::iterator it;
    std::map<int64_t, TimerTaskPtr>::iterator tmp;

    for (it = tasks_.begin(); it != tasks_.end(); /**/ ) {
        if(it->second->dead_) {
            log_info("purge task %ld", it->first);
            tmp = it ++;
            tasks_.erase(tmp);
        } else {
            ++it;
        }
    }
}

