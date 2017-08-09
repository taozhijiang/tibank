#include <iostream>
#include <sstream>

#include <boost/format.hpp>
#include <boost/thread.hpp>

#include "HttpServer.h"
#include "NetConn.h"
#include "HttpHandler.h"

#include "Log.h"

HttpServer::HttpServer(const std::string& address, unsigned short port, size_t c_cz) :
    io_service_(),
    ep_(ip::tcp::endpoint(ip::address::from_string(address), port)),
    acceptor_(io_service_, ep_),
    io_service_threads_(c_cz),
	net_conn_remove_threads_(1) {

    acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor_.listen();

    do_accept();
}

bool HttpServer::init() {

	if (!io_service_threads_.init_threads(boost::bind(&HttpServer::io_service_run, shared_from_this(), _1))) {
		log_error("HttpServer::io_service_run init task failed!");
		return false;
	}

	if (!net_conn_remove_threads_.init_threads(boost::bind(&HttpServer::net_conn_remove_run, shared_from_this(), _1))) {
		log_error("HttpServer::net_conn_remove_run init task failed!");
		return false;
	}

	register_http_post_handler("/submit", http_handler::submit_handler);
	register_http_post_handler("/query", http_handler::query_handler);

    return true;
}


// main task loop
void HttpServer::io_service_run(ThreadObjPtr ptr) {

	std::stringstream ss_id;
	ss_id << boost::this_thread::get_id();
	log_trace("HttpServer io_service thread %s is about to work... ", ss_id.str().c_str());

    while (true) {

		if (unlikely(ptr->status_ == ThreadStatus::kThreadTerminating)) {
			log_error("Thread %s is about to terminating...", ss_id.str().c_str());
			break;
		}

		// 线程启动
		if (unlikely(ptr->status_ == ThreadStatus::kThreadSuspend)) {
			::usleep(1*1000*1000);
			continue;
		}

        boost::system::error_code ec;
        io_service_.run(ec);

        if (ec){
            log_error("io_service stopped...");
            break;
        }
    }

	ptr->status_ = ThreadStatus::kThreadDead;
	log_trace("HttpServer io_service thread %s is about to terminate ... ", ss_id.str().c_str());

	return;
}

void HttpServer::do_accept() {

    socket_shared_ptr sock_ptr(new ip::tcp::socket(io_service_));
    acceptor_.async_accept(*sock_ptr,
                           boost::bind(&HttpServer::accept_handler, this,
                                       boost::asio::placeholders::error, sock_ptr));
}

void HttpServer::accept_handler(const boost::system::error_code& ec, socket_shared_ptr sock_ptr) {

    if (ec) {
        log_error("Error during accept!");
        return;
    }

    std::stringstream output;
    output << "Client Info: " << sock_ptr->remote_endpoint().address() << "/" <<
        sock_ptr->remote_endpoint().port();
	log_debug(output.str().c_str());

    net_conn_ptr new_conn = boost::make_shared<NetConn>(sock_ptr, *this);
	net_conns_.insert(new_conn);

    new_conn->start();

    // 再次启动接收异步请求
    do_accept();
}

int HttpServer::register_http_post_handler(std::string uri, HttpPostHandler handler){

    uri = boost::algorithm::trim_copy(boost::to_lower_copy(uri));
    while (uri[uri.size()-1] == '/' && uri.size() > 1)  // 全部的大写字母，去除尾部 /
        uri = uri.substr(0, uri.size()-1);

    std::map<std::string, HttpPostHandler>::const_iterator it;
    for (it = http_post_handler_.cbegin(); it!=http_post_handler_.cend(); ++it) {
        if (boost::iequals(uri, it->first))
            log_error("Handler for %s already exists, override it!", uri.c_str());
    }

    http_post_handler_[uri] = handler;
    return 0;
}


int HttpServer::find_http_post_handler(std::string uri, HttpPostHandler& handler){

    uri = boost::algorithm::trim_copy(boost::to_lower_copy(uri));
    while (uri[uri.size()-1] == '/' && uri.size() > 1)  // 全部的小写字母，去除尾部 /
        uri = uri.substr(0, uri.size()-1);

    std::map<std::string, HttpPostHandler>::const_iterator it;
    for (it = http_post_handler_.cbegin(); it!=http_post_handler_.cend(); ++it) {
        if (boost::iequals(uri, it->first)){
            handler = it->second;
            return 0;
        }
    }

    return -1;
}

int HttpServer::io_service_stop_graceful() {

	log_error("About to stop io_service... ");
	io_service_.stop();
	io_service_threads_.graceful_stop_threads();

	return 0;
}

// main task loop
void HttpServer::net_conn_remove_run(ThreadObjPtr ptr) {

	std::stringstream ss_id;
	ss_id << boost::this_thread::get_id();
	log_trace("HttpServer net_conn_remove thread %s is about to work... ", ss_id.str().c_str());

    while (true) {

		if (unlikely(ptr->status_ == ThreadStatus::kThreadTerminating)) {
			if (pending_to_remove_.EMPTY()) {
				log_debug("net_conn_remove queue is empty, safe terminate");
				break;
			}
		}

		// 线程启动
		if (unlikely(ptr->status_ == ThreadStatus::kThreadSuspend)) {
			::usleep(1*1000*1000);
			continue;
		}

		net_conn_weak net_conn_weak_ptr;
		if (!pending_to_remove_.POP(net_conn_weak_ptr, 3 * 1000 /*3s*/)) {
			//log_debug("net_conn remove timeout return!");
			continue;
		}

		if (net_conn_ptr shared_ptr = net_conn_weak_ptr.lock()) {
			if (shared_ptr->get_conn_stat() != ConnStat::kConnError) {
				log_error("Warning, remove unerror conn: %d", shared_ptr->get_conn_stat());
			}

			//
			log_debug("do remove ... ");
		}

    }

	ptr->status_ = ThreadStatus::kThreadDead;
	log_trace("HttpServer net_conn_remove thread %s is about to terminate ... ", ss_id.str().c_str());

	return;
}


int HttpServer::net_conn_remove_stop_graceful() {

	log_error("About to stop net_conn_remove thread ... ");
	net_conn_remove_threads_.graceful_stop_threads();

	return 0;
}
