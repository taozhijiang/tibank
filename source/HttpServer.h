#ifndef _TiBANK_HTTP_SERVER_H_
#define _TiBANK_HTTP_SERVER_H_

#include "General.h"

#include <set>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "TCPConnAsync.h"
#include "ThreadPool.h"
#include "HttpHandler.h"

#include "EQueue.h"
#include "BucketSet.h"
#include "HttpParser.h"
#include "AliveTimer.h"

typedef boost::function<int (const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status)> HttpPostHandler;
typedef boost::function<int (const HttpParser& http_parser, std::string& response, string& status)> HttpGetHandler;

typedef TCPConnAsync ConnType;
typedef std::shared_ptr<ConnType> ConnTypePtr;
typedef std::weak_ptr<ConnType>   ConnTypeWeakPtr;

class HttpServer : public boost::noncopyable,
                     public std::enable_shared_from_this<HttpServer> {

    friend class TCPConnAsync;  // can not work with typedef, ugly ...

public:

    /// Construct the server to listen on the specified TCP address and port
    HttpServer(const std::string& address, unsigned short port, size_t t_size, std::string docu_root);
    bool init();


private:
    io_service io_service_;

    // 侦听地址信息
    ip::tcp::endpoint ep_;
    ip::tcp::acceptor acceptor_;

	const std::string docu_root_;
	std::vector<std::string> docu_index_;

    void do_accept();
    void accept_handler(const boost::system::error_code& ec, SocketPtr ptr);

    std::map<std::string, HttpPostHandler> http_post_handler_;
	std::map<std::string, HttpGetHandler> http_get_handler_;

    BucketSet<ConnTypePtr> conns_;

	EQueue<ConnTypeWeakPtr> pending_to_remove_;
	AliveTimer<ConnType>   conns_alive_;

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

	int conn_add(ConnTypePtr p_conn) {
		conns_.INSERT(p_conn);
        conns_alive_.insert(p_conn);
		return 0;
	}

    void conn_touch(ConnTypePtr p_conn) {
		conns_alive_.touch(p_conn);
	}

	int conn_pend_remove(ConnTypePtr p_conn) {
		pending_to_remove_.PUSH(ConnTypeWeakPtr(p_conn));
		conns_alive_.drop(p_conn);
		return 0;
	}

	ThreadPool conn_remove_threads_;
	void conn_remove_run(ThreadObjPtr ptr);
	int  conn_remove_stop_graceful();

public:
    ThreadPool io_service_threads_;
    void io_service_run(ThreadObjPtr ptr);	// main task loop
	int io_service_stop_graceful();
};

#endif //_TiBANK_HTTP_SERVER_H_
