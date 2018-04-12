#ifndef _TiBANK_HTTP_HANDLER_H_
#define _TiBANK_HTTP_HANDLER_H_

// 所有的http uri 路由

#include "General.h"
#include "HttpParser.h"

typedef boost::function<int (const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status_line)> HttpPostHandler;
typedef boost::function<int (const HttpParser& http_parser, std::string& response, string& status_line)> HttpGetHandler;

namespace http_handler {

// 批量接口
int submit_handler(const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status_line);
int query_handler (const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status_line);

// 单笔接口
int batch_submit_handler(const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status_line);
int batch_query_handler (const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status_line);

int default_http_get_handler(const HttpParser& http_parser, std::string& response, string& status_line);

} // end namespace


#endif //_TiBANK_HTTP_HANDLER_H_
