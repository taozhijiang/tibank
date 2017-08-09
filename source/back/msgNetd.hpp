#ifndef _MSG_NETD_HPP_
#define _MSG_NETD_HPP_

#include "Netd/Netd.hpp"
#include "msgConnect.hpp"
#include "msgPool.hpp"
#include "msgWorker.hpp"

#include <boost/bind.hpp>
#include <list>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <unistd.h>

namespace Netd {

extern msgNetd* g_msgNetd;

// 单线程解决方案 +
// 工作线程池
class msgNetd
{
public:
    //不准拷贝、赋值
    msgNetd(const msgNetd&) = delete;
    msgNetd& operator=(const msgNetd&) = delete;

    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    msgNetd(const std::string& address, unsigned short port,
             size_t worker_sz);

    /// Run the server's io_service loop.
    bool initialze();
    void startWorker();
    void run();
    size_t getMsgWorkerCount() {
        return worker_sz_;
    }
    void getMsgWorkerStatus(size_t idx, size_t& id, size_t& capacity, size_t& count) {
        assert(idx < workers_.size());

        id = workers_[idx]->getID();
        capacity = workers_[idx]->getTaskCapacity();
        count = workers_[idx]->getTaskCount();

        return;
    }

    bool pushToDeliver(const MSG& msg, bool oride = false) {
        bool ret = to_deliver_.pushMsg(msg, oride);
        if (ret)
            notify_deliver();

        return ret;
    }

private:
    void notify_deliver() {
        const char* dat = "S";
        boost::system::error_code ignored_error;
        boost::unique_lock<boost::mutex> lock(deliverNotifyMutex_);
        boost::asio::write(*deliverNotifyWrite_, boost::asio::buffer(dat, 1),
                           boost::asio::transfer_all(), ignored_error);
    }

    void detect_deliver() {
            deliverNotifyRead_->async_read_some(
                       boost::asio::null_buffers(),
                       boost::bind(&msgNetd::detect_deliver_handler, this,
                                               boost::asio::placeholders::error));
    }

    void detect_deliver_handler(boost::system::error_code ec);

private:
    friend class msgConnect;

    io_service io_service_;

    // 侦听地址信息
    ip::tcp::endpoint ep_;
    ip::tcp::acceptor acceptor_;

    void do_accept();
    void accept_handler(const boost::system::error_code& ec, socket_ptr ptr);
    std::list<msgConnect_ptr> conns_;

    boost::asio::deadline_timer* timed_checker_;
    void timed_handler(const boost::system::error_code& ec);

    boost::asio::signal_set signals_;
    void signal_handler(const boost::system::error_code& ec, int signal_number);

private:
    // 计算的结果，将要返回给服务器的
    boost::mutex deliverNotifyMutex_;
    asio_fd_ptr deliverNotifyRead_;
    asio_fd_ptr deliverNotifyWrite_;

    // msgPool已经自带互斥保护
    netask::msgPool<MSG> to_deliver_;

    const size_t worker_sz_;
    std::vector<boost::shared_ptr<msgWorker>> workers_;
};

} // END NAMESPACE

#endif //_MSG_NETD_HPP_
