#include <iostream>
#include <sstream>

#include <boost/format.hpp>
#include <boost/thread.hpp>

#include "HttpHandler.h"
#include "Utils.h"
#include "SrvManager.h"
#include "TimerService.h"

#include "Log.h"
#include "HttpServer.h"

static const size_t bucket_size_ = 0xFF;
static size_t bucket_hash_index_call(const std::shared_ptr<ConnType>& ptr) {
    return std::hash<ConnType *>()(ptr.get());
}

HttpServer::HttpServer(const std::string& address, unsigned short port, size_t c_cz, std::string docu_root) :
    io_service_(),
    ep_(ip::tcp::endpoint(ip::address::from_string(address), port)),
    acceptor_(io_service_, ep_),
	docu_root_(docu_root),
	docu_index_({"index.html", "index.htm", "index.xhtml"}),
    conns_(bucket_size_, bucket_hash_index_call),
	pending_to_remove_(),
	conns_alive_(),
	conn_remove_threads_(1),
    io_service_threads_(static_cast<uint8_t>(c_cz)) {

    acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor_.listen();

    do_accept();
}

bool HttpServer::init() {

	if (!conns_.init()) {
		log_err("Init net_conns_ BucketSet failed!");
		return false;
	}

	if (!io_service_threads_.init_threads(boost::bind(&HttpServer::io_service_run, shared_from_this(), _1))) {
		log_err("HttpServer::io_service_run init task failed!");
		return false;
	}

	if (!conn_remove_threads_.init_threads(boost::bind(&HttpServer::conn_remove_run, shared_from_this(), _1))) {
		log_err("HttpServer::net_conn_remove_run init task failed!");
		return false;
	}

	// customize route uri handler
	register_http_post_handler("/submit", http_handler::submit_handler);
	register_http_post_handler("/query", http_handler::query_handler);

    register_http_post_handler("/batch_submit", http_handler::batch_submit_handler);
	register_http_post_handler("/batch_query", http_handler::batch_query_handler);


    // add purge task
	int conn_time_out = 0;
	int conn_time_linger = 0;
	if (!get_config_value("http.conn_time_out", conn_time_out) || !get_config_value("http.conn_time_linger", conn_time_linger) ){
        log_err("Error, get value error");
        return false;
    }
	log_debug("socket conn time_out: %ds, linger: %ds", conn_time_out, conn_time_linger);
    conns_alive_.init(boost::bind(&HttpServer::conn_pend_remove, this, _1),
									conn_time_out, conn_time_linger);

    if (TimerService::instance().register_timer_task(
                                    boost::bind(&AliveTimer<ConnType>::clean_up, &conns_alive_),
                                    5*1000, true, false) == 0) {
		log_err("Register alive purge task failed!");
		return false;
	}

    return true;
}


// main task loop
void HttpServer::io_service_run(ThreadObjPtr ptr) {

	std::stringstream ss_id;
	ss_id << boost::this_thread::get_id();
	log_info("HttpServer io_service thread %s is about to work... ", ss_id.str().c_str());

    while (true) {

		if (unlikely(ptr->status_ == ThreadStatus::kThreadTerminating)) {
			log_err("Thread %s is about to terminating...", ss_id.str().c_str());
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
            log_err("io_service stopped...");
            break;
        }
    }

	ptr->status_ = ThreadStatus::kThreadDead;
	log_info("HttpServer io_service thread %s is about to terminate ... ", ss_id.str().c_str());

	return;
}

void HttpServer::do_accept() {

    SocketPtr sock_ptr(new ip::tcp::socket(io_service_));
    acceptor_.async_accept(*sock_ptr,
                           boost::bind(&HttpServer::accept_handler, this,
                                       boost::asio::placeholders::error, sock_ptr));
}

void HttpServer::accept_handler(const boost::system::error_code& ec, SocketPtr sock_ptr) {

    if (ec) {
        log_err("Error during accept!");
        return;
    }

    std::stringstream output;
    output << "Client Info: " << sock_ptr->remote_endpoint().address() << "/" <<
        sock_ptr->remote_endpoint().port();
	log_debug(output.str().c_str());

    ConnTypePtr new_conn = std::make_shared<ConnType>(sock_ptr, *this);
	conn_add(new_conn);

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
            log_err("Handler for %s already exists, override it!", uri.c_str());
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

int HttpServer::register_http_get_handler(std::string uri, HttpGetHandler handler){

    uri = boost::algorithm::trim_copy(boost::to_lower_copy(uri));
    while (uri[uri.size()-1] == '/' && uri.size() > 1)  // 全部的大写字母，去除尾部 /
        uri = uri.substr(0, uri.size()-1);

    std::map<std::string, HttpGetHandler>::const_iterator it;
    for (it = http_get_handler_.cbegin(); it!=http_get_handler_.cend(); ++it) {
        if (boost::iequals(uri, it->first))
            log_err("Handler for %s already exists, override it!", uri.c_str());
    }

    http_get_handler_[uri] = handler;
    return 0;
}


int HttpServer::find_http_get_handler(std::string uri, HttpGetHandler& handler){

    uri = boost::algorithm::trim_copy(boost::to_lower_copy(uri));
    while (uri[uri.size()-1] == '/' && uri.size() > 1)  // 全部的小写字母，去除尾部 /
        uri = uri.substr(0, uri.size()-1);

    std::map<std::string, HttpGetHandler>::const_iterator it;
    for (it = http_get_handler_.cbegin(); it!=http_get_handler_.cend(); ++it) {
        if (boost::iequals(uri, it->first)){
            handler = it->second;
            return 0;
        }
    }

    return -1;
}

int HttpServer::io_service_stop_graceful() {

	log_err("About to stop io_service... ");
	io_service_.stop();
	io_service_threads_.graceful_stop_threads();

	return 0;
}

// main task loop
void HttpServer::conn_remove_run(ThreadObjPtr ptr) {

	std::stringstream ss_id;
	ss_id << boost::this_thread::get_id();
	log_info("HttpServer net_conn_remove thread %s is about to work... ", ss_id.str().c_str());

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

		ConnTypeWeakPtr net_conn_weak_ptr;
		if (!pending_to_remove_.POP(net_conn_weak_ptr, 3 * 1000 /*3s*/)) {
			//log_debug("net_conn remove timeout return!");
			continue;
		}

		if (ConnTypePtr shared_conn_ptr = net_conn_weak_ptr.lock()) {
			if (shared_conn_ptr->get_conn_stat() != ConnStat::kConnError) {
				log_err("Warning, remove unerror conn: %d", shared_conn_ptr->get_conn_stat());
			}

			// log_debug("do remove ... ");
			conns_.ERASE(shared_conn_ptr);
		}

		// log_info("Current net_conns_ size: %ld ", net_conns_.SIZE());

    }

	ptr->status_ = ThreadStatus::kThreadDead;
	log_info("HttpServer net_conn_remove thread %s is about to terminate ... ", ss_id.str().c_str());

	return;
}


int HttpServer::conn_remove_stop_graceful() {

	log_err("about to stop net_conn_remove thread ... ");
	conn_remove_threads_.graceful_stop_threads();

	return 0;
}
