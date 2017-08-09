#ifndef _TiBANK_HTTP_SERVER_H_
#define _TiBANK_HTTP_SERVER_H_

#include <set>

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "TiGeneral.h"
#include "NetConn.h"
#include "ThreadPoolHelper.h"

typedef boost::function<int (const std::string& post_data)> HttpPostHandler;

class HttpServer : public boost::noncopyable,
                     public boost::enable_shared_from_this<HttpServer> {
public:

    /// Construct the server to listen on the specified TCP address and port
    HttpServer(const std::string& address, unsigned short port, size_t t_size);
    bool init();

    int register_http_post_handler(std::string uri, HttpPostHandler handler);
    int find_http_post_handler(std::string uri, HttpPostHandler& handler);

private:
    friend class NetConn;
    io_service io_service_;

    // 侦听地址信息
    ip::tcp::endpoint ep_;
    ip::tcp::acceptor acceptor_;

    void do_accept();
    void accept_handler(const boost::system::error_code& ec, socket_shared_ptr ptr);

    std::map<std::string, HttpPostHandler> http_post_handler_;

    boost::mutex net_conns_mutex_;
    std::set<net_conn_ptr> net_conns_;

public:
    ThreadPoolHelper io_service_threads_;
    void io_service_run(ThreadObjPtr ptr);	// main task loop
};

#endif //_TiBANK_HTTP_SERVER_H_
