#ifndef _TiBANK_HTTP_PARSER_H_
#define _TiBANK_HTTP_PARSER_H_

#include <map>
#include <sstream>

#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "TiGeneral.h"
#include "HttpProto.h"
#include "Log.h"

class HttpParser: private boost::noncopyable {
public:
    HttpParser():
        request_headers_() {}

    bool parse_request_header(const char* header_ptr) {
        if (!header_ptr || !strlen(header_ptr) || !strstr(header_ptr, "\r\n\r\n")) {
            log_error( "Check raw header package failed ...");
            return false;
        }

        return do_parse_request(std::string(header_ptr));
    }

    bool parse_request_header(const std::string& header) {
        return do_parse_request(header);
    }

    std::string find_request_header(std::string option_name) {

        if (!option_name.size())
            return "";

        std::map<std::string, std::string>::const_iterator it;
        for (it = request_headers_.cbegin(); it!=request_headers_.cend(); ++it) {
            if (boost::iequals(option_name, it->first))
                return it->second;
        }

        return "";
    }

private:
    bool do_parse_request(const std::string& header) {

        request_headers_.clear();
        request_headers_.insert(std::make_pair(http_proto::header_options::request_body, header.substr(header.find("\r\n\r\n") + 4)));
        std::string header_part = header.substr(0, header.find("\r\n\r\n") + 4);

        std::istringstream resp(header_part);
        std::string item;
        std::string::size_type index;

        while (std::getline(resp, item) && item != "\r") {
            index = item.find(':', 0);
            if(index != std::string::npos) { // 直接Key-Value
                request_headers_.insert(std::make_pair(
                        boost::algorithm::trim_copy(item.substr(0, index)),
                        boost::algorithm::trim_copy(item.substr(index + 1)) ));
            } else { // HTTP 请求行，特殊处理
                boost::smatch what;
                if (boost::regex_match(item, what,
                                         boost::regex("([a-zA-Z]+)[ ]+([^ ]+)([ ]+(.*))?")))
                {
                    request_headers_.insert(std::make_pair(http_proto::header_options::request_method,
                                                       boost::algorithm::trim_copy(boost::to_upper_copy(string(what[1])))));
                    string uri = boost::algorithm::trim_copy(boost::to_lower_copy(string(what[2])));

                    while (uri[uri.size()-1] == '/') {  // 全部的大写字母，去除尾部
                        uri = uri.substr(0, uri.size()-1);
                    }

                    request_headers_.insert(std::make_pair(http_proto::header_options::request_uri, uri));
                    request_headers_.insert(std::make_pair(http_proto::header_options::http_version, boost::algorithm::trim_copy(string(what[3]))));
                }
            }
        }

        return true;
    }

private:
    std::map<std::string, std::string> request_headers_;
};


#endif // _TiBANK_HTTP_PARSER_H_
