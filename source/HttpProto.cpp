#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "HttpProto.h"

namespace http_proto {

struct header {
  std::string name;
  std::string value;
};


/**
 * 由于最终的底层都是调用c_str()发送的，所以这里不添加额外的字符
 */
string http_response_generate(const string& content, const string& stat_str) {

    std::vector<header> headers(6);

    // reply fixed header
    headers[0].name = "Server";
    headers[0].value = "TiBank Server/1.0";
    headers[1].name = "Date";
    headers[1].value = to_simple_string(second_clock::universal_time());
    headers[2].name = "Content-Length";
    headers[2].value = std::to_string(static_cast<long long unsigned>(content.size()));
    headers[3].name = "Content-Type";
    headers[3].value = "text/html";
    headers[4].name = "Connection";
    headers[4].value = "keep-alive";
    headers[5].name = "Access-Control-Allow-Origin";
    headers[5].value = "*";

    string str = stat_str;
    for (size_t i=0; i< headers.size(); ++i) {
        str += headers[i].name;
        str += header_name_value_separator_str;
        str += headers[i].value;
        str += header_crlf_str;
    }

    str += header_crlf_str;
    str += content;

    return str;
}

string http_response_generate(const char* data, size_t len, const string& stat_str) {

    std::vector<header> headers(6);

    // reply fixed header
    headers[0].name = "Server";
    headers[0].value = "TiBank Server/1.0";
    headers[1].name = "Date";
    headers[1].value = to_simple_string(second_clock::universal_time());
    headers[2].name = "Content-Length";
    headers[2].value = std::to_string(static_cast<long long int>(len));
    headers[3].name = "Content-Type";
    headers[3].value = "text/html";
    headers[4].name = "Connection";
    headers[4].value = "keep-alive";
    headers[5].name = "Access-Control-Allow-Origin";
    headers[5].value = "*";

    string str = stat_str;
    for (size_t i=0; i< headers.size(); ++i) {
        str += headers[i].name;
        str += header_name_value_separator_str;
        str += headers[i].value;
        str += header_crlf_str;
    }

    str += header_crlf_str;
    str += data;

    return str;
}

}  // http_proto

