#ifndef _TiBANK_HTTP_HANDLER_H_
#define _TiBANK_HTTP_HANDLER_H_

// 所有的http uri 路由

#include "TiGeneral.h"
#include "NetConn.h"

typedef boost::function<int (const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status)> HttpPostHandler;
typedef boost::function<int (const HttpParser& http_parser, std::string& response, string& status)> HttpGetHandler;

namespace http_handler {

int submit_handler(const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status);
int query_handler (const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status);

int default_http_get_handler(const HttpParser& http_parser, std::string& response, string& status);

} // end namespace


#endif //_TiBANK_HTTP_HANDLER_H_
