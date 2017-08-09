#ifndef _TiBANK_CONN_IF_HPP_
#define _TiBANK_CONN_IF_HPP_

#include "Log.h"

enum ConnStat {
    kConnWorking = 1,
    kConnPending,
    kConnError,
};

class ConnIf {

public:

    /// Construct a connection with the given socket.
    explicit ConnIf(boost::shared_ptr<ip::tcp::socket> sock_ptr):
        conn_stat_(kConnPending), sock_ptr_(sock_ptr) {
        set_tcp_nonblocking(true);
    }

    virtual void start() = 0;

    virtual void do_read() = 0;
    virtual void do_write() = 0;

    virtual void read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) = 0;
    virtual void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) = 0;

    virtual ~ConnIf() {}

public:
    // some general tiny function
    bool set_tcp_nonblocking(bool set_value) {
        socket_base::non_blocking_io command(set_value);
        sock_ptr_->io_control(command);

        return true;
    }

    bool set_tcp_nodelay(bool set_value) {

        boost::asio::ip::tcp::no_delay nodelay(set_value);
        sock_ptr_->set_option(nodelay);
        boost::asio::ip::tcp::no_delay option;
        sock_ptr_->get_option(option);

        return (option.value() == set_value);
    }

    bool set_tcp_keepalive(bool set_value) {

        boost::asio::socket_base::keep_alive keepalive(set_value);
        sock_ptr_->set_option(keepalive);
        boost::asio::socket_base::keep_alive option;
        sock_ptr_->get_option(option);

        return (option.value() == set_value);
    }

    void sock_shutdown(ip::tcp::socket::shutdown_type s_type) {
        boost::system::error_code ignore_ec;
        sock_ptr_->shutdown(s_type, ignore_ec);
    }

    enum ConnStat get_conn_stat() { return conn_stat_; }
    void set_conn_stat(enum ConnStat stat) { conn_stat_ = stat; }

private:
    enum ConnStat conn_stat_;

protected:
    boost::shared_ptr<ip::tcp::socket> sock_ptr_;
};


#endif //_TiBANK_CONN_IF_HPP_
