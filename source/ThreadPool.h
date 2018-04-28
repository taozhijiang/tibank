#ifndef __TiBANK_THREAD_POOL_H__
#define __TiBANK_THREAD_POOL_H__

#include <memory>
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

typedef std::shared_ptr<boost::thread>       ThreadPtr;
typedef std::shared_ptr<ThreadObj>           ThreadObjPtr;
typedef boost::function<void (ThreadObjPtr)> ThreadRunnable;

class ThreadPool: private boost::noncopyable {
        // 先于线程工作之前的所有预备工作
    class Impl;
    std::unique_ptr<Impl> impl_ptr_;

public:

    ThreadPool(uint8_t thread_num);
    ~ThreadPool();


    bool init_threads(ThreadRunnable func);

    void start_threads();
    void suspend_threads();

    // release this thread object
    void graceful_stop_threads();
    void immediate_stop_threads();
    void join_threads();

    int resize_threads(uint8_t thread_num);
    size_t get_thread_pool_size();
};


#endif // __TiBANK_THREAD_POOL_H__
