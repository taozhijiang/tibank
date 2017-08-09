#ifndef _TiBANK_NET_CONN_H_
#define _TiBANK_NET_CONN_H_


#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>

#include "ConnIf.h"
#include "HttpParser.h"
#include "Log.h"

class NetConn;
typedef boost::shared_ptr<NetConn> net_conn_ptr;
typedef boost::weak_ptr<NetConn>   net_conn_weak;


class HttpServer;
class NetConn: public ConnIf, public boost::noncopyable,
               public boost::enable_shared_from_this<NetConn> {

public:

    /// Construct a connection with the given socket.
    NetConn(boost::shared_ptr<ip::tcp::socket> sock_ptr, HttpServer& server);

    virtual ~NetConn() {
		log_debug("NET CONN SOCKET RELEASED!!!");
	}

    virtual void start();
    void stop();

	// http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
	bool handle_socket_ec(const boost::system::error_code& ec);

private:

    virtual void do_read() override { safe_assert(false); }
	virtual void read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) override {
		safe_assert(false);
	}

    virtual void do_write();
    virtual void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred);

    void do_read_head();
    void read_head_handler(const boost::system::error_code &ec, std::size_t bytes_transferred);

    void do_read_body();
    void read_body_handler(const boost::system::error_code &ec, std::size_t bytes_transferred);

    void fill_http_for_send(const char* data, size_t len, const string& status);
    void fill_http_for_send(const string& str, const string& status);

private:

    // 用于读取HTTP的头部使用
    boost::asio::streambuf request_;   // client request_ read

	time_t expired_at_;

private:

    HttpServer& http_server_;
    HttpParser http_parser_;
    boost::shared_ptr<io_service::strand> strand_;

private:
    // 读写的有效负载记录
    size_t r_size_; //读取开始写的位置

    size_t w_size_; // 有效负载的末尾
    size_t w_pos_;  //写可能会一次不能完全发送，这里保存已写的位置

    boost::shared_ptr<std::vector<char> > p_buffer_;
    boost::shared_ptr<std::vector<char> > p_write_;

    bool send_buff_empty() {
        return (w_size_ == 0 || (w_pos_ >= w_size_) );
    }
};


#endif //_TiBANK_NET_CONN_H_