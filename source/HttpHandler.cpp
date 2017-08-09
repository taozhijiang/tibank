
#include "HttpProto.h"
#include "HttpHandler.h"

#include "Log.h"


namespace http_handler {

int submit_handler(const std::string& post_data, std::string& response, string& status) {

	response = "ECHO BACK:";
	response += post_data;
	status = http_proto::status::ok;

	return 0;
}

int query_handler(const std::string& post_data, std::string& response, string& status) {

	response = "ECHO BACK:";
	response += post_data;
	status = http_proto::status::ok;

	return 0;
}

} // end namespace

