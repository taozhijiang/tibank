#ifndef _MSG_CONNECT_HPP_
#define _MSG_CONNECT_HPP_

#include "General.hpp"
#include "Connect.hpp"
#include "msgConnect.hpp"

#include <boost/enable_shared_from_this.hpp>

namespace Netd {

class msgConnect;

typedef boost::shared_ptr<msgConnect> msgConnect_ptr;
typedef boost::weak_ptr<msgConnect>   msgConnect_weak;

class msgConnect : public Connect,
                   public boost::enable_shared_from_this<msgConnect>
{
public:
    msgConnect(const msgConnect&) = delete;
    msgConnect& operator=(const msgConnect&) = delete;

    /// Construct a connection with the given socket.
    explicit msgConnect(boost::shared_ptr<ip::tcp::socket> p_sock);

    virtual void start();
    virtual void stop() override;
    virtual void do_read() override;
    void do_read_at_least(size_t len);
    virtual void do_write() override;

    virtual void read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) override;
    virtual void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) override;

    // 业务相关代码
    ssize_t process_msg(const char* pdata);

    virtual ~msgConnect() { BOOST_LOG_T(debug) << "msgConnect SOCKET RELEASED!!!"; }

private:
    void conn_set_error();
    bool item_handler();
};

}


#endif //_MSG_CONNECT_HPP_
