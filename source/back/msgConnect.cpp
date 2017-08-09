#include "General.hpp"
#include "msgNetd.hpp"

#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

#include <openssl/md5.h>

namespace Netd {

extern msgNetd* g_msgNetd;

bool SockSendBuf_FillError(P_MSG pMsg, const char *strError);
bool SockSendBuf_SendError(P_MSG pMsg, const char *strError);
bool SockSendBuf(const P_MSG pMsg);
bool SockSendBuf(const MSG& Msg);

msgConnect::msgConnect(boost::shared_ptr<ip::tcp::socket> p_sock):
    Connect(p_sock)
{
    // p_buffer_ & p_write_
    // already allocated @ connection

    set_tcp_nodelay(true);
    set_tcp_keepalive(true);

    /**
    * 通常一个数据包不会有这么大的，如果超过了这里的限定，觉得可能收到错包了
    * 这个时候应该断开连接，让客户端重连进行校正
    * p_buffer_->size()
    */
    p_buffer_ = boost::make_shared<std::vector<char> >(32*1024, 0);
    p_write_  = boost::make_shared<std::vector<char> >(32*1024, 0);
}

void msgConnect::start()
{
    /**
     * 这里需要注意的是，如果do_read()不是虚函数，而派生类只是简单的覆盖，
     * 那么在accept　handler中调用的new_c->start()会导致这里会调用基类
     * 版本的do_read
     */
    set_stats(connWork);
    r_size_ = w_size_ = w_pos_ = 0;
    do_read();
}

void msgConnect::stop()
{
    set_stats(connPend);
}

void msgConnect::do_read()
{
    if (get_stats() != connWork)
    {
        BOOST_LOG_T(error) << "SOCK STATUS: " << get_stats();
        return;
    }

    touch_sock_time();
    p_sock_->async_read_some(buffer(p_buffer_->data() + r_size_, p_buffer_->size() - r_size_),
                             boost::bind(&msgConnect::read_handler,
                                  this,
                                  boost::asio::placeholders::error,
                                  boost::asio::placeholders::bytes_transferred));

    return;
}


void msgConnect::do_read_at_least(size_t len)
{
    if (get_stats() != connWork)
    {
        BOOST_LOG_T(error) << "SOCK STATUS: " << get_stats();
        return;
    }

    BOOST_LOG_T(info) << "read async_read at least for size " << len;

    touch_sock_time();
    async_read(*p_sock_, buffer(p_buffer_->data() + r_size_, len - r_size_),
                    boost::asio::transfer_at_least(len - r_size_),
                                 boost::bind(&msgConnect::read_handler,
                                     shared_from_this(),
                                     boost::asio::placeholders::error,
                                     boost::asio::placeholders::bytes_transferred));
    return;
}

void msgConnect::do_write()
{
    if ( get_stats() != connWork )
    {
        BOOST_LOG_T(error) << "SOCK STATUS: " << get_stats();
        return;
    }

    touch_sock_time();
    assert(w_size_);
    assert(w_pos_ < w_size_);

    BOOST_LOG_T(info) << "write async_write exactly for size " << w_size_ - w_pos_;
    async_write(*p_sock_, buffer(p_write_->data() + w_pos_, w_size_ - w_pos_),
                    boost::asio::transfer_at_least(w_size_ - w_pos_),
                                 boost::bind(&msgConnect::write_handler,
                                     shared_from_this(),
                                     boost::asio::placeholders::error,
                                     boost::asio::placeholders::bytes_transferred));
    return;
}

void msgConnect::read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (!ec && bytes_transferred)
    {
        r_size_ += bytes_transferred;

        BOOST_LOG_T(debug) << "this term read bytes length:" << bytes_transferred;
        char *ptr = p_buffer_->data();

        ssize_t ret = process_msg(ptr);
        if (ret == -1) {
            BOOST_LOG_T(error) << "process_msg_v4() return error";

            boost::system::error_code ignored_ec;
            p_sock_->shutdown(ip::tcp::socket::shutdown_send, ignored_ec);
            p_sock_->cancel();

            conn_set_error();
        }

        if (ret > 0) {
            BOOST_LOG_T(debug) << "return: " << ret << ", r_size_: " << r_size_;
            if (r_size_ != ret)
                ::memmove(ptr, ptr + ret, r_size_ - ret);
            r_size_ -= ret;
        }

        // continue read again!
        do_read();
    }
    else if (ec != boost::asio::error::operation_aborted)
    {
        BOOST_LOG_T(error) << "WRITE ERROR FOUND:" << ec;

        boost::system::error_code ignored_ec;
        p_sock_->shutdown(ip::tcp::socket::shutdown_send, ignored_ec);
        p_sock_->cancel();

        conn_set_error();
    }
}

void msgConnect::write_handler(const boost::system::error_code& ec, size_t bytes_transferred)
{
    if (!ec && bytes_transferred)
    {
        w_pos_ += bytes_transferred;

        if (w_pos_ < w_size_)
        {
            BOOST_LOG_T(error) << "ADDITIONAL WRITE: " << w_pos_ << " ~ " << w_size_;
            do_write();
        }
        else
        {
            w_pos_ = w_size_ = 0;
        }
    }
    else if (ec != boost::asio::error::operation_aborted)
    {
        BOOST_LOG_T(error) << "WRITE ERROR FOUND:" << ec;

        boost::system::error_code ignored_ec;
        p_sock_->shutdown(ip::tcp::socket::shutdown_send, ignored_ec);
        p_sock_->cancel();

        conn_set_error();
    }
}

void msgConnect::conn_set_error()
{
    set_stats(connError);
    return;
}


// 后端采用的线程池，为了保持会话粘滞性，这里就需要解析消息体，
// 比如按照site_id & session_id 进行会话分配

#include <cstdlib>

ssize_t msgConnect::process_msg(const char* pdata)
{
    if (!pdata || pdata[0] != 'C') {
        netask::netask_abort("Check Message Header!");
    }

    if (r_size_ < 3) {
        BOOST_LOG_T(error) << "数据包头太小：" << r_size_;
        return -1;
    }

    unsigned short msg_len = (unsigned char)pdata[1];  //高位在前
	msg_len = (msg_len << 8) + (unsigned char)pdata[2];

    if (msg_len >= netask::MAX_MSG_SIZE) {
        BOOST_LOG_T(debug) << "数据包头太大：" << msg_len << ":" << netask::MAX_MSG_SIZE;
        netask::netask_abort("数据包太大！");
    }

    if (r_size_ < msg_len) {
        BOOST_LOG_T(debug) << "数据包还没接受完：" << r_size_ << ":" << msg_len;
        return 0;
    }

    P_MSG pMsg = boost::make_shared<MSG>(shared_from_this(), pdata, msg_len);
    std::cout << "recv len:" << pMsg->data_count << " content:" << pMsg->data_buf << std::endl;

    // 处理，可能的话会压入任务队列
    // 早起的错误检查也会直接返回错误结果
    // Papapa...

    size_t worker_idx = ::random() % g_msgNetd->workers_.size();
    g_msgNetd->workers_[worker_idx]->pushWorkerTask(*pMsg);

    return msg_len;
}


}
