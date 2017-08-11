#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

#include "HttpServer.h"
#include "NetConn.h"


NetConn::NetConn(boost::shared_ptr<ip::tcp::socket> sock_ptr,
                       HttpServer& server):
    ConnIf(sock_ptr),
    http_server_(server),
    http_parser_(),
    strand_(boost::make_shared<io_service::strand>(server.io_service_)) {

    set_tcp_nodelay(true);

    r_size_ = 0;
    w_size_ = 0;
    w_pos_  = 0;

    set_tcp_nonblocking(true);

    // 可以被后续resize增加
    p_buffer_ = boost::make_shared<std::vector<char> >(16*1024, 0);
    p_write_  = boost::make_shared<std::vector<char> >(16*1024, 0);

}

void NetConn::start() override {
    set_conn_stat(ConnStat::kConnWorking);
    r_size_ = w_size_ = w_pos_ = 0;
    do_read_head();
}

void NetConn::stop() {
    set_conn_stat(ConnStat::kConnPending);
}

// Wrapping the handler with strand.wrap. This will return a new handler, that will dispatch through the strand.
// Posting or dispatching directly through the strand.

void NetConn::do_read_head() {

    if (get_conn_stat() != ConnStat::kConnWorking) {
        log_error("Socket Status Error: %d", get_conn_stat());
        return;
    }

    std::stringstream output;
    output << "strand read read_until ... in " << boost::this_thread::get_id();
    log_debug(output.str().c_str());

    async_read_until(*sock_ptr_, request_,
                             "\r\n\r\n",
                             strand_->wrap(
                                 boost::bind(&NetConn::read_head_handler,
                                     shared_from_this(),
                                     boost::asio::placeholders::error,
                                     boost::asio::placeholders::bytes_transferred)));
    return;
}


void NetConn::do_read_body() {

    if (get_conn_stat() != ConnStat::kConnWorking) {
        log_error("Socket Status Error: %d", get_conn_stat());
        return;
    }

    size_t len = ::atoi(http_parser_.find_request_header(http_proto::header_options::content_length).c_str());

    std::stringstream output;
    output << "strand read async_read exactly... in " << boost::this_thread::get_id();
    log_debug(output.str().c_str());

    async_read(*sock_ptr_, buffer(p_buffer_->data() + r_size_, len - r_size_),
                    boost::asio::transfer_at_least(len - r_size_),
                             strand_->wrap(
                                 boost::bind(&NetConn::read_body_handler,
                                     shared_from_this(),
                                     boost::asio::placeholders::error,
                                     boost::asio::placeholders::bytes_transferred)));
    return;
}

void NetConn::do_write() override {

    if (get_conn_stat() != ConnStat::kConnWorking) {
        log_error("Socket Status Error: %d", get_conn_stat());
        return;
    }

    safe_assert(w_size_);
    safe_assert(w_pos_ < w_size_);

    std::stringstream output;
    output << "strand write async_write exactly... in " << boost::this_thread::get_id();
    log_debug(output.str().c_str());

    async_write(*sock_ptr_, buffer(p_write_->data() + w_pos_, w_size_ - w_pos_),
                    boost::asio::transfer_at_least(w_size_ - w_pos_),
                              strand_->wrap(
                                 boost::bind(&NetConn::write_handler,
                                     shared_from_this(),
                                     boost::asio::placeholders::error,
                                     boost::asio::placeholders::bytes_transferred)));
    return;
}

void NetConn::read_head_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

    if (!ec && bytes_transferred) {

        std::string head_str (boost::asio::buffers_begin(request_.data()),
                                boost::asio::buffers_begin(request_.data()) + request_.size());

        request_.consume(bytes_transferred); // skip the head

        if (!http_parser_.parse_request_header(head_str.c_str())) {
            log_error( "Parse request error: %s", head_str.c_str());
            goto error_return;
        }

        if (! boost::iequals(http_parser_.find_request_header(http_proto::header_options::request_method), "POST") ) {
            log_error("Invalid request method: %s", http_parser_.find_request_header(http_proto::header_options::request_method).c_str());
            fill_http_for_send(http_proto::content_bad_request, http_proto::status::bad_request);
            goto write_return;
        }

        size_t len = ::atoi(http_parser_.find_request_header(http_proto::header_options::content_length).c_str());
        r_size_ = 0;
        size_t additional_size = request_.size(); // net additional body size

        safe_assert( additional_size <= len );
        if (len + 1 > p_buffer_->size()) {
            log_trace( "relarge receive buffer size to: %d", (len + 256));
            p_buffer_->resize(len + 256);
        }

        // first async_read_until may read more additional data, if so
        // then move additional data possible
        if( additional_size ) {

            std::string additional (boost::asio::buffers_begin(request_.data()),
                      boost::asio::buffers_begin(request_.data()) + additional_size);

            memcpy(p_buffer_->data(), additional.c_str(), additional_size + 1);
            r_size_ = additional_size;

            request_.consume(additional_size); // skip the head part
        }

        // normally, we will return these 2 cases
        if (additional_size < len) {
            // need to read more data here, write to r_size_
            do_read_body();
        }
        else {
            // call the process callback directly
            read_body_handler(ec, additional_size);   // already updated r_size_
        }

        return;

    } else {

		if (handle_socket_ec(ec)) {
			boost::system::error_code ignored_ec;
			sock_ptr_->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
			sock_ptr_->cancel();
		}

        return;
    }

error_return:
    fill_http_for_send(http_proto::content_error, http_proto::status::internal_server_error);
    request_.consume(request_.size());
    r_size_ = 0;

write_return:
    do_write();

    start();   // 算了，强制一个读操作，从而可以引发其错误处理

    return;
}


void NetConn::read_body_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

    if (!ec && bytes_transferred) {

        size_t len = ::atoi(http_parser_.find_request_header(http_proto::header_options::content_length).c_str());
        r_size_ += bytes_transferred;

        if (r_size_ < len) {
            // need to read more, do again!
            do_read_body();
            return;
        }

        HttpPostHandler handler;
        if (http_server_.find_http_post_handler(http_parser_.find_request_header(http_proto::header_options::request_uri), handler) != 0){
            log_error("uri %s handler not found!", http_parser_.find_request_header(http_proto::header_options::request_uri).c_str());
            fill_http_for_send(http_proto::content_not_found, http_proto::status::not_found);
        } else {
            if (handler) {
				std::string response_body;
				std::string response_status;
                handler(std::string(p_buffer_->data(), r_size_), response_body, response_status); // call it!
                if (response_body.empty() || response_status.empty()) {
                    log_error("caller not generate response body!");
                    fill_http_for_send(http_proto::content_ok, http_proto::status::ok);
                } else {
					fill_http_for_send(response_body, response_status);
				}
            } else {
                log_error("uri %s found, but handler empty!", http_parser_.find_request_header(http_proto::header_options::request_uri).c_str());
                fill_http_for_send(http_proto::content_bad_request, http_proto::status::bad_request);
            }
        }

        // default, OK
		// go through write return;

	} else {

		if (handle_socket_ec(ec)) {
			boost::system::error_code ignored_ec;
			sock_ptr_->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
			sock_ptr_->cancel();
		}

        return;
    }

write_return:
    do_write();

    // 同上
    start();
    return;
}


void NetConn::write_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

    if (!ec && bytes_transferred) {

        w_pos_ += bytes_transferred;

        if (w_pos_ < w_size_) {
            log_debug("Need additional write operation: %lu ~ %lu", w_pos_, w_size_);
            do_write();
        }
        else {
            w_pos_ = w_size_ = 0;
        }

	} else {

		if (handle_socket_ec(ec)) {
			boost::system::error_code ignored_ec;
			sock_ptr_->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
			sock_ptr_->cancel();
		}
    }
}


void NetConn::fill_http_for_send(const char* data, size_t len, const string& status = http_proto::status::ok) {
    safe_assert(data && len);

    if (!data || !len) {
        log_error("Check send data...");
        return;
    }

    string content = http_proto::http_response_generate(data, len, status);
    if (content.size() + 1 > p_write_->size())
        p_write_->resize(content.size() + 1);

    ::memcpy(p_write_->data(), content.c_str(), content.size() + 1); // copy '\0' but not transform it

    w_size_ = content.size();
    w_pos_  = 0;

    return;

}


void NetConn::fill_http_for_send(const string& str, const string& status = http_proto::status::ok) {

    string content = http_proto::http_response_generate(str, status);
    if (content.size() + 1 > p_write_->size())
        p_write_->resize(content.size() + 1);

    ::memcpy(p_write_->data(), content.c_str(), content.size() + 1); // copy '\0' but not transform it

    w_size_ = content.size();
    w_pos_  = 0;

    return;
}

// http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
bool NetConn::handle_socket_ec(const boost::system::error_code& ec) {

	bool close_socket = false;

	if (ec == boost::asio::error::eof) {
		log_debug("Peer closed up...");
		close_socket = true;
	} else if (ec == boost::asio::error::connection_reset) {
		log_debug("Connection reset by peer...");
		close_socket = true;
	} else if (ec == boost::asio::error::operation_aborted) {
		log_debug("Operation aborted..."); // like timer ...
	} else {
		log_debug("Undetected error...");
		close_socket = true;
	}

	if (close_socket) {
		set_conn_stat(ConnStat::kConnError);
		http_server_.add_net_conn_to_remove(shared_from_this());
	}

	return close_socket;
}
