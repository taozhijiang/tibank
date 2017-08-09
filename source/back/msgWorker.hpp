#ifndef _MSG_WORKER_HPP_
#define _MSG_WORKER_HPP_

#include "General.hpp"
#include "msgPool.hpp"
#include "Netd/Netd.hpp"

#include "Message.hpp"

namespace Netd {

// 工作线程池
class msgWorker
{
public:
    //不准拷贝、赋值
    msgWorker(const msgWorker&) = delete;
    msgWorker& operator=(const msgWorker&) = delete;

    explicit msgWorker(size_t task_cnt, size_t task_sz = netask::MAX_MSG_SIZE) :
        tasks_(task_cnt),
        id_(WORKER_ID++){
        BOOST_LOG_T(info) << "new msgWorker object create: " << id_ << "!";
    }

    bool pushWorkerTask(const MSG& msg, bool oride = false) {
        return tasks_.pushMsg(msg, oride);
    }

    size_t getID() { return id_; }
    size_t getTaskCapacity() {
        return tasks_.getMsgCapacity();
    }
    size_t getTaskCount() {
        return tasks_.getMsgCount();
    }

    void run();

    ~msgWorker() {
        BOOST_LOG_T(info) << "msgWorker object destructed: " << id_ << "!";
    }

private:
    // 当前工作线程的任务队列
    netask::msgPool<MSG> tasks_;

private:
    static size_t WORKER_ID;
    size_t id_;
};

} // END NAMESPACE

#endif //_RPC_WORKER_HPP_
