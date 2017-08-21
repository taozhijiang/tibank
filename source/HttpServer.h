#ifndef _TiBANK_HTTP_SERVER_H_
#define _TiBANK_HTTP_SERVER_H_

#include <set>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "TiGeneral.h"
#include "NetConn.h"
#include "ThreadPoolHelper.h"
#include "HttpHandler.h"

#include "EQueue.h"
#include "BucketSet.h"
#include "HttpParser.h"
#include "AliveTimer.h"

typedef boost::function<int (const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status)> HttpPostHandler;
typedef boost::function<int (const HttpParser& http_parser, std::string& response, string& status)> HttpGetHandler;

class HttpServer : public boost::noncopyable,
                    public boost::enable_shared_from_this<HttpServer> {
public:

    /// Construct the server to listen on the specified TCP address and port
    HttpServer(const std::string& address, unsigned short port, size_t t_size, std::string docu_root);
    bool init();


private:
    friend class NetConn;
    io_service io_service_;

    // 侦听地址信息
    ip::tcp::endpoint ep_;
    ip::tcp::acceptor acceptor_;

	const std::string docu_root_;
	std::vector<std::string> docu_index_;

    void do_accept();
    void accept_handler(const boost::system::error_code& ec, socket_shared_ptr ptr);

    std::map<std::string, HttpPostHandler> http_post_handler_;
	std::map<std::string, HttpGetHandler> http_get_handler_;

    BucketSet<net_conn_ptr> net_conns_;
	EQueue<net_conn_weak> pending_to_remove_;

	AliveTimer<NetConn> alived_conns_;

public:

    int register_http_post_handler(std::string uri, HttpPostHandler handler);
    int find_http_post_handler(std::string uri, HttpPostHandler& handler);

	int register_http_get_handler(std::string uri, HttpGetHandler handler);
    int find_http_get_handler(std::string uri, HttpGetHandler& handler);

	const std::string& get_document_root() {
		return docu_root_;
	}

	const std::vector<std::string>& get_document_index() {
		return docu_index_;
	}

	int add_net_conn(net_conn_ptr conn_ptr) {
		net_conns_.INSERT(conn_ptr);
        alived_conns_.insert(conn_ptr);
	}

    void touch_net_conn(net_conn_ptr conn_ptr) {
		alived_conns_.touch(conn_ptr);
	}

	int add_net_conn_to_remove(net_conn_ptr conn_ptr) {
		pending_to_remove_.PUSH(net_conn_weak(conn_ptr));
	}

	ThreadPoolHelper net_conn_remove_threads_;
	void net_conn_remove_run(ThreadObjPtr ptr);
	int net_conn_remove_stop_graceful();

    AliveTimer<NetConn>& get_keep_alived() {
        return alived_conns_;
    }

public:
    ThreadPoolHelper io_service_threads_;
    void io_service_run(ThreadObjPtr ptr);	// main task loop
	int io_service_stop_graceful();
};

#endif //_TiBANK_HTTP_SERVER_H_
