#include <iostream>

#include "msgNetd.hpp"

#include <boost/format.hpp>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>


namespace Netd {

msgNetd::msgNetd(const std::string& address, unsigned short port,
                 size_t worker_sz) :
    io_service_(),
    ep_(ip::tcp::endpoint(ip::address::from_string(address), port)),
    acceptor_(io_service_, ep_),
    conns_(),
    to_deliver_(512),
    worker_sz_(worker_sz),
    workers_(worker_sz, nullptr),
    timed_checker_(0),
    signals_(io_service_, SIGUSR1),
    deliverNotifyMutex_()
{
    BOOST_LOG_T(info) << "msgNetd server running at " << address << ":" << port ;

    acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor_.listen();
}

bool msgNetd::initialze() {
    int ret;
    int sv[2];

    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    if (ret == -1) {
        netask::netask_abort("Create deliver socketpair failed!");
    }

    deliverNotifyRead_ = boost::make_shared<asio_fd>(io_service_, sv[0]);
    deliverNotifyWrite_ = boost::make_shared<asio_fd>(io_service_, sv[1]);
    deliverNotifyRead_->non_blocking(true);
    deliverNotifyWrite_->non_blocking(false);

    BOOST_LOG_T(info) << "start count: " << worker_sz_ << " msgWorker thread... ";

    for (size_t i=0; i<worker_sz_; ++i) {
        workers_[i] = boost::make_shared<msgWorker>(512);
        assert(workers_[i]);
    }

    do_accept();

    return true;
}

void msgNetd::startWorker() {

    bool ret = initialze();
    assert(ret);

    for (size_t i = 0; i < worker_sz_; ++i) {
        assert(workers_[i]);

        // start worker threads
        netask::all_threads_groups.create_thread(
                boost::bind(&msgWorker::run, workers_[i]) );
    }
}

/// Run the server's io_service loop.

void msgNetd::run()
{
    timed_checker_ = new boost::asio::deadline_timer (io_service_,
                                              boost::posix_time::millisec(200)); //200ms
    assert(timed_checker_);

    timed_checker_->async_wait(boost::bind(&msgNetd::timed_handler, this,
                                               boost::asio::placeholders::error));


    signals_.async_wait(boost::bind(&msgNetd::signal_handler, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::signal_number));

    detect_deliver();

    // single-threaded
    io_service_.run();
}

void msgNetd::do_accept()
{
    socket_ptr p_sock(new ip::tcp::socket(io_service_));
    acceptor_.async_accept(*p_sock,
                           boost::bind(&msgNetd::accept_handler, this,
                                       boost::asio::placeholders::error, p_sock));
}

void msgNetd::accept_handler(const boost::system::error_code& ec, socket_ptr p_sock)
{
    if (ec)
    {
        BOOST_LOG_T(error) << "Error during accept!";
        return;
    }

    BOOST_LOG_T(debug) << "Client Info: " << p_sock->remote_endpoint().address() << ":" <<
        p_sock->remote_endpoint().port();

    msgConnect_ptr newConn = boost::make_shared<msgConnect>(p_sock);
    conns_.push_back(newConn);
    newConn->start();

    // 再次启动接收异步请求
    do_accept();
}


void msgNetd::timed_handler(const boost::system::error_code& ec)
{
    BOOST_LOG_T(info) << "timed_handler called ...";

    auto iter = conns_.begin();
    while ( iter != conns_.end() ) {
        if ((*iter)->get_stats() == connError) {
            BOOST_LOG_T(info) << "Removing connError connection: " << (*iter)->get_id();
            iter = conns_.erase(iter);
        }
        else
            ++ iter;
    }

    // 设定一个时间间隔。由于检测需要时间，总体的时间会不准确，
    // 但不是关键业务，可以忽略误差
    timed_checker_->expires_from_now(
                boost::posix_time::millisec(3000)); //500ms TAO
    timed_checker_->async_wait(boost::bind(&msgNetd::timed_handler, this,
                                               boost::asio::placeholders::error));

    return;
}

void msgNetd::detect_deliver_handler(boost::system::error_code ec){
    boost::system::error_code error;

    MSG deli;

    if (!ec) {

        std::vector<char> buf(128);
        size_t cnt = 0;

        {
            boost::unique_lock<boost::mutex> lock(deliverNotifyMutex_);
            cnt = deliverNotifyRead_->read_some(boost::asio::buffer(buf), error);
        }
        if (!error) {
            for (size_t i=0; i<cnt; ++i) {
                /**
                 * 有可能在override的pushMsg情况下，Notify中记录的消息
                 * 数目大于实际存储的消息数目的情况
                 */
                if(to_deliver_.try_popMsg(deli))
                {
                    if (conn_ptr pconn = deli.conn.lock()) {
                        pconn->fill_and_send(deli.data_buf, deli.data_count);
                    }
                }
            }
        }
    }

    // Detect it again!@
    detect_deliver();
}


void msgNetd::signal_handler(const boost::system::error_code& ec, int signal_number){
    if (signal_number == SIGUSR1) {
        BOOST_LOG_T(info) << "SIGUSR1 Detected";
        //notify_deliver();
    }
    else {
        netask::netask_abort("Unkown signal detected!");
    }

    // listen the signal again
    signals_.async_wait(boost::bind(&msgNetd::signal_handler, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::signal_number));
}



} // END NAMESPACE


