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
    submit_ret.trans_response = TransResponseCode::kTransResponseOK;
    submit_ret.trans_status = TransStatus::kTransSubmitFail;
    submit_ret.trans_err_code = TransErrInfo::kTransNoErr;
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
        query_ret.trans_err_code = TransErrInfo::kTransParamErr;
        goto error_ret;
    }

    mem = root.getMemberNames();
    try {
        for (Json::Value::Members::const_iterator iter = mem.begin(); iter != mem.end(); iter++) {
            mapParam[*iter] = root[*iter].asString();
        }
    } catch (std::exception& e){
        log_err("get string member error for: %s", post_data.c_str());
        query_ret.trans_err_code = TransErrInfo::kTransParamErr;
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

        query_ret.trans_err_code = TransErrInfo::kTransParamErr;
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
    query_ret.trans_response = TransResponseCode::kTransResponseOK;
    query_ret.trans_status = TransStatus::kTransSubmitFail;
    generate_trans_query_ret(query_ret, response);
    status = http_proto::status::ok;
    return 0;

error_ret:
    response = http_proto::content_error;
    status = http_proto::status::internal_server_error;
	return -1;
}

int batch_submit_handler(const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status) {

    Json::Value root;
	Json::Reader reader;
    Json::Value::Members mem;

    std::map<std::string, std::string> mapParam;

    trans_batch_submit_request batch_submit_req;
    trans_batch_submit_response batch_submit_ret;
    trans_submit_response dummy_submit_ret; // dummy, not concerning

    std::vector<trans_submit_request> orders;
    Json::Value orderReqs;

    if (!reader.parse(post_data, root) || root.isNull()) {
        log_err("parse error for: %s", post_data.c_str());

        batch_submit_ret.trans_err_code = TransErrInfo::kTransParamErr;
        goto error_ret;
    }

    mem = root.getMemberNames();
    try {
        for (Json::Value::Members::const_iterator iter = mem.begin(); iter != mem.end(); iter++) {
            mapParam[*iter] = root[*iter].asString();
        }
    } catch (std::exception& e){
        log_err("get string member error for: %s", post_data.c_str());

        batch_submit_ret.trans_err_code = TransErrInfo::kTransParamErr;
        goto error_ret;
    }

    if( mapParam.find("merch_id") == mapParam.end() ||
        mapParam.find("merch_name") == mapParam.end() ||
        mapParam.find("total_count") == mapParam.end() ||
        mapParam.find("total_amount") == mapParam.end() ||
#if 0
        mapParam.find("orders") == mapParam.end() ||
        mapParam.find("sign") == mapParam.end() ) {
#else
        mapParam.find("orders") == mapParam.end() ) {
#endif
        log_err("param check error for: %s", post_data.c_str());

        batch_submit_ret.trans_err_code = TransErrInfo::kTransParamErr;
        goto error_ret;
    }

#if 0
    if (SignHelper::instance().check_sign(mapParam) != 0) {
        log_err("Sign check error for: %s", post_data.c_str());
        submit_ret.trans_err_code = TransErrCode::kTransErrParam;
        submit_ret.trans_err_str = get_trans_err_str(TransErrCode::kTransErrParam);
        goto error_ret2;
    }
#endif

    batch_submit_req.merch_id = mapParam.at("merch_id");
    batch_submit_req.merch_name = mapParam.at("merch_name");
    batch_submit_req.total_count = ::atol(mapParam.at("total_count").c_str());
    batch_submit_req.total_amount = ::atol(mapParam.at("total_amount").c_str());

    if (!reader.parse(mapParam["orders"], orderReqs) || !orderReqs.isArray()) {
    	log_err("parse error for: %s", mapParam["orders"].c_str());

        batch_submit_ret.trans_err_code = TransErrInfo::kTransParamErr;
    	goto error_ret;
    }

    for (size_t i = 0; i < orderReqs.size(); i++) {
    	if (!orderReqs[i]["trans_id"].isString() || !orderReqs[i]["account_no"].isString() ||
    		!orderReqs[i]["account_name"].isString() || !orderReqs[i]["amount"].isString() ||
    		!orderReqs[i]["account_type"].isString() || !orderReqs[i]["branch_name"].isString() ||
    		!orderReqs[i]["branch_no"].isString() || !orderReqs[i]["remarks"].isString()){

            log_err("order error!");

            batch_submit_ret.trans_err_code = TransErrInfo::kTransParamErr;
    		goto error_ret;
    	}

        trans_submit_request req;
        req.merch_id = batch_submit_req.merch_id;
        req.merch_name = batch_submit_req.merch_name;

        req.trans_id = orderReqs[i]["trans_id"].asString();
        req.account_no = orderReqs[i]["account_no"].asString();
        req.account_name = orderReqs[i]["account_name"].asString();
        req.amount = ::atol(orderReqs[i]["amount"].asString().c_str());
        req.account_type = ::atoi(orderReqs[i]["account_type"].asString().c_str());
        req.branch_name = orderReqs[i]["branch_name"].asString();
        req.branch_no = orderReqs[i]["branch_no"].asString();
        req.remarks = orderReqs[i]["remarks"].asString();

        orders.push_back(req);
    }

    batch_submit_req.orders = orders;

    for (size_t i = 0; i < batch_submit_req.orders.size(); i++){
        process_trans_submit(batch_submit_req.orders[i], dummy_submit_ret);
    }


    batch_submit_ret.merch_id = batch_submit_req.merch_id;
    batch_submit_ret.merch_name = batch_submit_req.merch_name;
    batch_submit_ret.message_type = "批量提交回复";
    batch_submit_ret.total_count = batch_submit_req.total_count;
    batch_submit_ret.total_amount = batch_submit_req.total_amount;

    batch_submit_ret.trans_response = TransResponseCode::kTransResponseOK;
    batch_submit_ret.trans_status = TransStatus::kTransInProcess;
    batch_submit_ret.trans_err_code = TransErrInfo::kTransNoErr;


    generate_trans_batch_submit_ret(batch_submit_ret, response);
    status = http_proto::status::ok;
    return 0;

error_ret2:
    batch_submit_ret.trans_response = TransResponseCode::kTransResponseOK;
    batch_submit_ret.trans_status = TransStatus::kTransSubmitFail;
    generate_trans_batch_submit_ret(batch_submit_ret, response);
    status = http_proto::status::ok;
    return 0;

error_ret:
    response = http_proto::content_error;
    status = http_proto::status::internal_server_error;
	return -1;
}

int batch_query_handler(const HttpParser& http_parser, const std::string& post_data, std::string& response, string& status) {

    Json::Value root;
	Json::Reader reader;
    Json::Value::Members mem;

    std::map<std::string, std::string> mapParam;

    trans_batch_query_request batch_query_req;
    trans_batch_query_response batch_query_ret;

    std::vector<trans_query_request> orders;
    Json::Value orderReqs;

    std::vector<trans_query_response> orders_ret;

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
        mapParam.find("total_count") == mapParam.end() ||
        mapParam.find("total_amount") == mapParam.end() ||
#if 0
        mapParam.find("orders") == mapParam.end() ||
        mapParam.find("sign") == mapParam.end() ) {
#else
        mapParam.find("orders") == mapParam.end() ) {
#endif
        log_err("param check error for: %s", post_data.c_str());
        goto error_ret;
    }

#if 0
    if (SignHelper::instance().check_sign(mapParam) != 0) {
        log_err("Sign check error for: %s", post_data.c_str());
        query_req.trans_err_code = TransErrCode::kTransErrParam;
        query_req.trans_err_str = get_trans_err_str(TransErrCode::kTransErrParam);
        goto error_ret2;
    }
#endif

    batch_query_req.merch_id = mapParam.at("merch_id");
    batch_query_req.merch_name = mapParam.at("merch_name");
    batch_query_req.total_count = ::atol(mapParam.at("total_count").c_str());
    batch_query_req.total_amount = ::atol(mapParam.at("total_amount").c_str());

    if (!reader.parse(mapParam["orders"], orderReqs) || !orderReqs.isArray()) {
    	log_err("parse error for: %s", mapParam["orders"].c_str());
    	goto error_ret;
    }

    for (size_t i = 0; i < orderReqs.size(); i++) {
    	if (!orderReqs[i]["trans_id"].isString() || !orderReqs[i]["account_no"].isString()
            || !orderReqs[i]["account_name"].isString() || !orderReqs[i]["account_type"].isString() ){
    		log_err("order error!");
    		goto error_ret;
    	}

        trans_query_request req;
        req.merch_id = batch_query_req.merch_id;
        req.merch_name = batch_query_req.merch_name;

        req.trans_id = orderReqs[i]["trans_id"].asString();
        req.account_no = orderReqs[i]["account_no"].asString();
        req.account_name = orderReqs[i]["account_name"].asString();
        req.account_type = ::atoi(orderReqs[i]["account_type"].asString().c_str());

        orders.push_back(req);
    }

    batch_query_req.orders = orders;

    for (size_t i = 0; i < batch_query_req.orders.size(); i++){
        trans_query_response query_ret;

        query_ret.merch_id = batch_query_req.orders[i].merch_id;
        query_ret.merch_name = batch_query_req.orders[i].merch_name;
        query_ret.trans_id = batch_query_req.orders[i].trans_id;
        query_ret.account_no = batch_query_req.orders[i].account_no;
        query_ret.account_name = batch_query_req.orders[i].account_name;

        process_trans_query(batch_query_req.orders[i], query_ret);
        orders_ret.push_back(query_ret);
    }

    batch_query_ret.merch_id = batch_query_req.merch_id;
    batch_query_ret.merch_name = batch_query_req.merch_name;
    batch_query_ret.message_type = "批量查询回复";
    batch_query_ret.total_count = batch_query_req.total_count;
    batch_query_ret.total_amount = batch_query_req.total_amount;
    batch_query_ret.trans_response = TransResponseCode::kTransResponseOK;
    batch_query_ret.orders = orders_ret;


    generate_trans_batch_query_ret(batch_query_ret, response);
    status = http_proto::status::ok;
    return 0;

error_ret2:
    batch_query_ret.trans_response = TransResponseCode::kTransResponseFail;
    generate_trans_batch_query_ret(batch_query_ret, response);
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

