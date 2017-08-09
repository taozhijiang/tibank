#ifndef _TiBANK_HTTP_HANDLER_H_
#define _TiBANK_HTTP_HANDLER_H_

// 所有的http uri 路由

#include "TiGeneral.h"
#include "NetConn.h"

typedef boost::function<int (const std::string& post_data, std::string& response, string& status)> HttpPostHandler;

namespace http_handler {

int submit_handler(const std::string& post_data, std::string& response, string& status);
int query_handler(const std::string& post_data, std::string& response, string& status);

} // end namespace


#endif //_TiBANK_HTTP_HANDLER_H_
