#ifndef _TiBANK_HTTP_PROTO_H_
#define _TiBANK_HTTP_PROTO_H_

#include "TiGeneral.h"

namespace http_proto {

    static const string content_ok = "{}";

    static const string content_error =  "<html>"
                  "<head><title>Internal Server Error</title></head>"
                  "<body><h1>500 Internal Server Error</h1></body>"
                  "</html>";
    static const string content_bad_request = "<html>"
                  "<head><title>Bad Request</title></head>"
                  "<body><h1>400 Bad Request</h1></body>"
                  "</html>";
    static const string content_forbidden = "<html>"
                  "<head><title>Forbidden</title></head>"
                  "<body><h1>403 Forbidden</h1></body>"
                  "</html>";
    static const string content_not_found = "<html>"
                  "<head><title>Not Found</title></head>"
                  "<body><h1>404 Not Found</h1></body>"
                  "</html>";

    namespace header_options {

        static const std::string request_method("request_method_");     // (GET/POST)
        static const std::string request_uri("request_uri_");           // raw uri, http://127.0.0.1:8900/docs/index.html?aa=bb
		static const std::string request_path_info("request_path_info_"); // docs/index.html
		static const std::string request_query_str("request_query_str_"); // aa == bb
		static const std::string http_version("http_version_");		    // HTTP/1.0|HTTP/1.1
        static const std::string request_body("request_body_");		    // used for post

        static const std::string host("Host");
        static const std::string accept("Accept");
        static const std::string range("Range");
        static const std::string cookie("Cookie");
        static const std::string referer("Referer");
        static const std::string user_agent("User-Agent");
        static const std::string content_type("Content-Type");
        static const std::string content_length("Content-Length");
        static const std::string content_range("Content-Range");
        static const std::string connection("Connection");
        static const std::string proxy_connection("Proxy-Connection");
        static const std::string accept_encoding("Accept-Encoding");
        static const std::string transfer_encoding("Transfer-Encoding");
        static const std::string content_encoding("Content-Encoding");

    } // namespace header_options


    namespace status {

        static const std::string ok =
          "HTTP/1.1 200 OK\r\n";
        static const std::string created =
          "HTTP/1.1 201 Created\r\n";
        static const std::string accepted =
          "HTTP/1.1 202 Accepted\r\n";
        static const std::string no_content =
          "HTTP/1.1 204 No Content\r\n";
        static const std::string multiple_choices =
          "HTTP/1.1 300 Multiple Choices\r\n";
        static const std::string moved_permanently =
          "HTTP/1.1 301 Moved Permanently\r\n";
        static const std::string moved_temporarily =
          "HTTP/1.1 302 Moved Temporarily\r\n";
        static const std::string not_modified =
          "HTTP/1.1 304 Not Modified\r\n";
        static const std::string bad_request =
          "HTTP/1.1 400 Bad Request\r\n";
        static const std::string unauthorized =
          "HTTP/1.1 401 Unauthorized\r\n";
        static const std::string forbidden =
          "HTTP/1.1 403 Forbidden\r\n";
        static const std::string not_found =
          "HTTP/1.1 404 Not Found\r\n";
        static const std::string internal_server_error =
          "HTTP/1.1 500 Internal Server Error\r\n";
        static const std::string not_implemented =
          "HTTP/1.1 501 Not Implemented\r\n";
        static const std::string bad_gateway =
          "HTTP/1.1 502 Bad Gateway\r\n";
        static const std::string service_unavailable =
          "HTTP/1.1 503 Service Unavailable\r\n";

    }

    namespace status_digit {

        static const unsigned ok = 200;
        static const unsigned created = 201;
        static const unsigned accepted = 202;
        static const unsigned no_content = 204;
        static const unsigned multiple_choices = 300;
        static const unsigned moved_permanently = 301;
        static const unsigned moved_temporarily = 302;
        static const unsigned not_modified = 304;
        static const unsigned bad_request = 400;
        static const unsigned unauthorized = 401;
        static const unsigned forbidden = 403;
        static const unsigned not_found = 404;
        static const unsigned internal_server_error = 500;
        static const unsigned not_implemented = 501;
        static const unsigned bad_gateway = 502;
        static const unsigned service_unavailable = 503;

    }

    static const char header_name_value_separator[] = { ':', ' ' };
    static const char header_crlf[] = { '\r', '\n'};

    static string header_name_value_separator_str = ": ";
    static string header_crlf_str = "\r\n";

    /**
     * 由于最终的底层都是调用c_str()发送的，所以这里不添加额外的字符
     */
    string http_response_generate(const string& content, const string& stat_str);
    string http_response_generate(const char* data, size_t len, const string& stat_str);
    int http_url_decode(const std::string& in, std::string& out);

}  // http_proto

#endif //_TiBANK_HTTP_PROTO_H_
