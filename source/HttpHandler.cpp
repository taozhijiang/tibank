#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

#include "HttpProto.h"
#include "HttpHandler.h"
#include "ServiceManager.h"
#include "HttpServer.h"

#include "TransProcess.h"
#include "SignHelper.h"
#include "Log.h"

#include "json/json.h"

namespace http_handler {

int submit_handler(const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status) {

    Json::Value root;
	Json::Reader reader;
    Json::Value::Members mem;

    std::map<std::string, std::string> mapParam;

    trans_submit_request submit_req;
    trans_submit_response submit_ret;

    if (!reader.parse(post_data, root) || root.isNull()) {
        log_err("parse error for: %s", post_data.c_str());
        goto error_ret;
    }

    mem = root.getMemberNames();
    try {
        for (Json::Value::Members::const_iterator iter = mem.begin(); iter != mem.end(); iter++) {
            mapParam[*iter] = root[*iter].asString();
        }
    } catch (std::exception& e){
        log_err("get string member error for: %s", post_data.c_str());
        goto error_ret;
    }

    if( mapParam.find("merch_id") == mapParam.end() ||
        mapParam.find("merch_name") == mapParam.end() ||
        mapParam.find("trans_id") == mapParam.end() ||
        mapParam.find("account_no") == mapParam.end() ||
        mapParam.find("account_name") == mapParam.end() ||
        mapParam.find("amount") == mapParam.end() ||
        mapParam.find("account_type") == mapParam.end() ||
        mapParam.find("branch_name") == mapParam.end() ||
        mapParam.find("branch_no") == mapParam.end() ||
#if 0
        mapParam.find("remarks") == mapParam.end() ||
        mapParam.find("sign") == mapParam.end() ) {
#else
        mapParam.find("remarks") == mapParam.end()) {
#endif
        log_err("param check error for: %s", post_data.c_str());
        goto error_ret;
    }

    submit_req.merch_id = mapParam.at("merch_id");
    submit_req.merch_name = mapParam.at("merch_name");
    submit_req.trans_id = mapParam.at("trans_id");
    submit_req.account_no = mapParam.at("account_no");
    submit_req.account_name = mapParam.at("account_name");
    submit_req.amount = ::atol(mapParam.at("amount").c_str());
    submit_req.account_type = ::atol(mapParam.at("account_type").c_str());
    submit_req.branch_name = mapParam.at("branch_name");
    submit_req.branch_no = mapParam.at("branch_no");
    submit_req.remarks = mapParam.at("remarks");

    submit_ret.merch_id = submit_req.merch_id;
    submit_ret.merch_name = submit_req.merch_name;
    submit_ret.message_type = "提交回复";
    submit_ret.trans_id = submit_req.trans_id;
    submit_ret.account_no = submit_req.account_no;
    submit_ret.account_name = submit_req.account_name;

#if 0
    if (SignHelper::instance().check_sign(mapParam) != 0) {
        log_err("Sign check error for: %s", post_data.c_str());
        submit_ret.trans_err_code = TransErrCode::kTransErrParam;
        submit_ret.trans_err_str = get_trans_err_str(TransErrCode::kTransErrParam);
        goto error_ret2;
    }
#endif
    process_trans_submit(submit_req, submit_ret);
    generate_trans_submit_ret(submit_ret, response);
    status = http_proto::status::ok;
    return 0;

error_ret2:
    submit_ret.trans_status = TransStatus::kTransSubmitFail;
    submit_ret.trans_status_str = get_trans_status_str(TransStatus::kTransSubmitFail);
    generate_trans_submit_ret(submit_ret, response);
    status = http_proto::status::ok;
    return 0;

error_ret:
    response = http_proto::content_error;
    status = http_proto::status::internal_server_error;
	return -1;
}

int query_handler(const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status) {

    Json::Value root;
	Json::Reader reader;
    Json::Value::Members mem;

    std::map<std::string, std::string> mapParam;

    trans_query_request query_req;
    trans_query_response query_ret;

    if (!reader.parse(post_data, root) || root.isNull()) {
        log_err("parse error for: %s", post_data.c_str());
        goto error_ret;
    }

    mem = root.getMemberNames();
    try {
        for (Json::Value::Members::const_iterator iter = mem.begin(); iter != mem.end(); iter++) {
            mapParam[*iter] = root[*iter].asString();
        }
    } catch (std::exception& e){
        log_err("get string member error for: %s", post_data.c_str());
        goto error_ret;
    }

    if( mapParam.find("merch_id") == mapParam.end() ||
        mapParam.find("merch_name") == mapParam.end() ||
        mapParam.find("trans_id") == mapParam.end() ||
        mapParam.find("account_no") == mapParam.end() ||
        mapParam.find("account_name") == mapParam.end() ||
#if 0
        mapParam.find("account_type") == mapParam.end() ||
        mapParam.find("sign") == mapParam.end() ) {
#else
        mapParam.find("account_type") == mapParam.end()) {
#endif
        log_err("param check error for: %s", post_data.c_str());
        goto error_ret;
    }

    query_req.merch_id = mapParam.at("merch_id");
    query_req.merch_name = mapParam.at("merch_name");
    query_req.trans_id = mapParam.at("trans_id");
    query_req.account_no = mapParam.at("account_no");
    query_req.account_name = mapParam.at("account_name");
    query_req.account_type = ::atol(mapParam.at("account_type").c_str());

    query_ret.merch_id = query_req.merch_id;
    query_ret.merch_name = query_req.merch_name;
    query_ret.message_type = "查询回复";
    query_ret.trans_id = query_req.trans_id;
    query_ret.account_no = query_req.account_no;
    query_ret.account_name = query_req.account_name;

#if 0
    if (SignHelper::instance().check_sign(mapParam) != 0) {
        log_err("Sign check error for: %s", post_data.c_str());
        query_req.trans_err_code = TransErrCode::kTransErrParam;
        query_req.trans_err_str = get_trans_err_str(TransErrCode::kTransErrParam);
        goto error_ret2;
    }
#endif
    process_trans_query(query_req, query_ret);
    generate_trans_query_ret(query_ret, response);
    status = http_proto::status::ok;
    return 0;

error_ret2:
    query_ret.trans_status = TransStatus::kTransSubmitFail;
    query_ret.trans_status_str = get_trans_status_str(TransStatus::kTransSubmitFail);
    generate_trans_query_ret(query_ret, response);
    status = http_proto::status::ok;
    return 0;

error_ret:
    response = http_proto::content_error;
    status = http_proto::status::internal_server_error;
	return -1;
}

static const std::string& get_document_root() {
	return ServiceManager::instance().http_server_ptr_->get_document_root();
}

static const std::vector<std::string>& get_document_index() {
	return ServiceManager::instance().http_server_ptr_->get_document_index();
}

static bool check_and_sendfile(std::string regular_file_path, std::string& response, string& status) {

	// check dest is directory or regular?
	struct stat sb;
	if (stat(regular_file_path.c_str(), &sb) == -1) {
		log_err("Stat file error: %s", regular_file_path.c_str());
		response = http_proto::content_error;
		status = http_proto::status::internal_server_error;
		return false;
	}

	if (sb.st_size > 100*1024*1024 /*100M*/) {
		log_err("Too big file size: %ld", sb.st_size);
		response = http_proto::content_bad_request;
		status = http_proto::status::bad_request;
		return false;
	}

	std::ifstream fin(regular_file_path);
	fin.seekg(0);
	std::stringstream buffer;
	buffer << fin.rdbuf();
	response = buffer.str();
	status = http_proto::status::ok;

	return true;
}


int default_http_get_handler(const HttpParser& http_parser, std::string& response, string& status) {

	const UriParamContainer& params = http_parser.get_request_uri_params();
	if (!params.EMPTY()) {
		log_err("Default handler just for static file transmit, we can not handler uri parameters...");
	}

	std::string real_file_path = get_document_root() + "/" + http_parser.find_request_header(http_proto::header_options::request_path_info);

	// check dest exist?
	if (::access(real_file_path.c_str(), R_OK) != 0) {
		log_err("File not found: %s", real_file_path.c_str());
		response = http_proto::content_not_found;
		status = http_proto::status::not_found;
		return -1;
	}

	// check dest is directory or regular?
	struct stat sb;
	if (stat(real_file_path.c_str(), &sb) == -1) {
		log_err("Stat file error: %s", real_file_path.c_str());
		response = http_proto::content_error;
		status = http_proto::status::internal_server_error;
		return -1;
	}

	switch (sb.st_mode & S_IFMT) {
		case S_IFREG:
			check_and_sendfile(real_file_path, response, status);
			break;

		case S_IFDIR:
			{
				bool OK = false;
				const std::vector<std::string> &indexes = get_document_index();
				for (std::vector<std::string>::const_iterator iter = indexes.cbegin(); iter != indexes.cend(); ++iter) {
					std::string file_path = real_file_path + "/" + *iter;
					log_info("Trying: %s", file_path.c_str());
					if (check_and_sendfile(file_path, response, status)) {
						OK = true;
						break;
					}
				}

				if (!OK) {
					// default, 404
					response = http_proto::content_not_found;
					status = http_proto::status::not_found;
				}
			}
			break;

		default:
			break;
	}

	return 0;
}

} // end namespace

