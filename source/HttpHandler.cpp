
#include "HttpProto.h"
#include "HttpHandler.h"

#include "TransProcess.h"
#include "SignHelper.h"
#include "Log.h"

#include "json/json.h"

namespace http_handler {

int submit_handler(const std::string& post_data, std::string& response, string& status) {

    Json::Value root;
	Json::Reader reader;
    Json::Value::Members mem;

    std::map<std::string, std::string> mapParam;

    trans_submit_request submit_req;
    trans_submit_response submit_ret;

    if (!reader.parse(post_data, root) || root.isNull()) {
        log_error("parse error for: %s", post_data.c_str());
        goto error_ret;
    }

    mem = root.getMemberNames();
    try {
        for (Json::Value::Members::const_iterator iter = mem.begin(); iter != mem.end(); iter++) {
            mapParam[*iter] = root[*iter].asString();
        }
    } catch (std::exception& e){
        log_error("get string member error for: %s", post_data.c_str());
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
        log_error("param check error for: %s", post_data.c_str());
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
        log_error("Sign check error for: %s", post_data.c_str());
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

int query_handler(const std::string& post_data, std::string& response, string& status) {

    Json::Value root;
	Json::Reader reader;
    Json::Value::Members mem;

    std::map<std::string, std::string> mapParam;

    trans_query_request query_req;
    trans_query_response query_ret;

    if (!reader.parse(post_data, root) || root.isNull()) {
        log_error("parse error for: %s", post_data.c_str());
        goto error_ret;
    }

    mem = root.getMemberNames();
    try {
        for (Json::Value::Members::const_iterator iter = mem.begin(); iter != mem.end(); iter++) {
            mapParam[*iter] = root[*iter].asString();
        }
    } catch (std::exception& e){
        log_error("get string member error for: %s", post_data.c_str());
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
        log_error("param check error for: %s", post_data.c_str());
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
        log_error("Sign check error for: %s", post_data.c_str());
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

} // end namespace

