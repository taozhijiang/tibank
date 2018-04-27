#include <sstream>

#include "SignHelper.h"
#include "Utils.h"
#include "SqlConn.h"
#include "TransProcess.h"
#include "TransProcessTask.h"
#include "Log.h"

#include "json/json.h"

extern const char* TiBANK_DATABASE_PREFIX;

std::string get_trans_status_str(int stat) {

    std::string ret;

    switch (stat){
        case kTransSubmitFail:
            ret = "提交失败";
            break;
        case kTransInProcess:
            ret = "交易处理中";
            break;
        case kTransSuccess:
            ret = "交易成功";
            break;
        case kTransFail:
            ret = "交易失败";
            break;
        case kTransNotFound:
            ret = "交易不存在";
            break;
        default:
            ret = "无意义状态";
            log_err("无意义状态：%d", stat);
    }

    return std::string(ret);
}


std::string get_trans_error_str(int code) {

    std::string ret;

    switch (code){
        case kTransNoErr:
            ret = "OK";
            break;
        case kTransDupReqErr:
            ret = "重复交易";
            break;
        case kTransParamErr:
            ret = "参数错误";
            break;
        case kTransBalanceErr:
            ret = "余额不足";
            break;
        case kTransAmountErr:
            ret = "金额超额(随便说的)";
            break;
        case kTransSystemErr:
            ret = "系统内部错误";
            break;
        case kTransUnknownErr:
            ret = "不明错误";
            break;

        default:
            ret = "未知错误";
            log_err("未知错误：%d", code);
    }

    return std::string(ret);
}

#if 0

 CREATE TABLE `tibank.t_trans_order_00` (
  `F_merch_id` varchar(20) NOT NULL COMMENT '商户号',
  `F_merch_name` varchar(64) NOT NULL COMMENT '商户名',
  `F_trans_id` varchar(20) NOT NULL COMMENT '付款流水号',
  `F_account_no` varchar(24) DEFAULT NULL COMMENT '账号',
  `F_account_name` varchar(128) DEFAULT NULL COMMENT '户名',
  `F_amount` bigint(11) NOT NULL COMMENT '付款金额',
  `F_account_type` tinyint(4) DEFAULT '1' COMMENT '1:对私，0:对公',
  `F_branch_no` char(12) DEFAULT NULL COMMENT '联行号',
  `F_branch_name` varchar(256) NOT NULL COMMENT '支行名称',
  `F_remarks` varchar(128) DEFAULT NULL COMMENT '备注',
  `F_status` bigint(11) NOT NULL COMMENT '交易状态: TransStatus',
  `F_errcode` bigint(11) NOT NULL COMMENT '错误码：TransErrCode, 0为正常',
  `F_create_time` datetime DEFAULT NULL COMMENT '创建时间',
  `F_update_time` datetime DEFAULT NULL COMMENT '更新时间',
  PRIMARY KEY (`F_merch_id`, `F_merch_name`, `F_trans_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ;

#endif



int process_trans_submit(const struct trans_submit_request& req, struct trans_submit_response& ret){

    sql_conn_ptr conn;
    request_scoped_sql_conn(conn);

    safe_assert(conn);
    if (!conn){
        log_err("Get SQL connection failed!");

        ret.trans_response = TransResponseCode::kTransResponseOK;
        ret.trans_status = TransStatus::kTransNonsense;
        ret.trans_err_code = TransErrInfo::kTransSystemErr;
        return -1;
    }

    // TODO 参数检测

    int count = 0;
    if(!conn->sqlconn_execute_query_value(va_format("SELECT COUNT(*) FROM %s.t_trans_order_%02d "
                   "WHERE F_merch_id='%s' AND F_merch_name='%s' AND F_trans_id='%s' AND F_account_no='%s' AND F_account_name='%s' "
                   "AND F_account_type=%d ", TiBANK_DATABASE_PREFIX, get_db_table_index(req.trans_id),
                    req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str(), req.account_no.c_str(),
                    req.account_name.c_str(), req.account_type), count) ) {
        log_err("订单防重查询失败：F_merch_id='%s', F_merch_name='%s', F_trans_id='%s'", req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str());
        ret.trans_response = TransResponseCode::kTransResponseOK;
        ret.trans_status = TransStatus::kTransSubmitFail;
        ret.trans_err_code = TransErrInfo::kTransSystemErr;
        return -1;
    }

    if (count > 0){
        log_err("订单重复：%d, F_merch_id='%s', F_merch_name='%s', F_trans_id='%s'", count,
                   req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str());

        ret.trans_response = TransResponseCode::kTransResponseOK;
        ret.trans_status = TransStatus::kTransSubmitFail;
        ret.trans_err_code = TransErrInfo::kTransDupReqErr;
        return -1;
    }

    if (!conn->sqlconn_execute(va_format("INSERT INTO %s.t_trans_order_%02d SET F_merch_id='%s', F_merch_name='%s', F_trans_id='%s', F_account_no='%s', "
                   "F_account_name='%s', F_amount=%ld, F_account_type=%d, F_branch_no='%s', F_branch_name='%s', F_remarks='%s', "
                   "F_status=%d, F_errcode=0, F_create_time=NOW(), F_update_time=NOW() ",
                    TiBANK_DATABASE_PREFIX, get_db_table_index(req.trans_id),
                    req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str(), req.account_no.c_str(),
                    req.account_name.c_str(), req.amount, req.account_type, req.branch_no.c_str(),
                    req.branch_name.c_str(), req.remarks.c_str(), TransStatus::kTransInProcess))) {

        log_err("插入订单异常： F_merch_id='%s', F_merch_name='%s', F_trans_id='%s'", req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str());

        ret.trans_response = TransResponseCode::kTransResponseOK;
        ret.trans_status = TransStatus::kTransNonsense;
        ret.trans_err_code = TransErrInfo::kTransSystemErr;
        return -1;
    }

    log_info("订单提交OK： F_merch_id='%s', F_merch_name='%s', F_trans_id='%s'", req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str());

    ret.trans_response = TransResponseCode::kTransResponseOK;
    ret.trans_status = TransStatus::kTransInProcess;
    ret.trans_err_code = TransErrInfo::kTransNoErr;

    EQueueDataPtr qd = std::make_shared<EQueueData>(req.merch_id, req.trans_id, req.account_type);
    creat_trans_process_task(qd);

    return 0;
}

int generate_trans_submit_ret(const trans_submit_response& ret, std::string& post_body) {

    std::map<std::string, std::string> map_param;

    if (ret.trans_response != TransResponseCode::kTransResponseOK && ret.trans_response != TransResponseCode::kTransResponseFail){
        ret.trans_response = TransResponseCode::kTransResponseFail;
    }
    ret.trans_status_str = get_trans_status_str(ret.trans_status);
    ret.trans_err_str = get_trans_error_str(ret.trans_err_code);

    map_param["merch_id"] = ret.merch_id;
    map_param["merch_name"] = ret.merch_name;
    map_param["message_type"] = ret.message_type;
    map_param["trans_id"] = ret.trans_id;
    map_param["account_no"] = ret.account_no;
    map_param["account_name"] = ret.account_name;

    map_param["trans_response"] = convert_to_string(ret.trans_response);
    map_param["trans_status"] = convert_to_string(ret.trans_status);
    map_param["trans_status_str"] = ret.trans_status_str;
    map_param["trans_err_code"] = convert_to_string(ret.trans_err_code);
    map_param["trans_err_str"] = ret.trans_err_str;

    std::string sign;
    if (SignHelper::instance().calc_sign(map_param, sign) != 0) {
        log_err("calc sign error!");
        return -1;
    }

    map_param["sign"] = sign;    // store it

    // build request json
    Json::Value root;
    for(std::map<std::string, std::string>::const_iterator itr = map_param.begin()
        ; itr != map_param.end()
        ; itr++) {
        root[itr->first] = itr->second;
    }

    Json::FastWriter fast_writer;
    post_body = fast_writer.write(root);
    log_debug("Return: %s", post_body.c_str());

    return 0;
}

int process_trans_query(const struct trans_query_request& req, struct trans_query_response& ret) {

    sql_conn_ptr conn;
    request_scoped_sql_conn(conn);

    safe_assert(conn);
    if (!conn){
        log_err("Get SQL connection failed!");
        ret.trans_response = TransResponseCode::kTransResponseOK;
        ret.trans_status = TransStatus::kTransNonsense;
        ret.trans_err_code = TransErrInfo::kTransSystemErr;
        return -1;
    }

    shared_result_ptr result(conn->sqlconn_execute_query(va_format("SELECT F_status, F_errcode, F_amount FROM %s.t_trans_order_%02d "
                   "WHERE F_merch_id='%s' AND F_merch_name='%s' AND F_trans_id='%s' AND F_account_no='%s' AND F_account_name='%s' "
                   "AND F_account_type=%d AND DATE(F_create_time)>=DATE_ADD(NOW(),INTERVAL -7 DAY) LIMIT 1 ",
                    TiBANK_DATABASE_PREFIX,  get_db_table_index(req.trans_id),
                    req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str(), req.account_no.c_str(),
                    req.account_name.c_str(), req.account_type)));

    if (!result) {
        log_err("订单查询失败：F_merch_id='%s', F_merch_name='%s', F_trans_id='%s'", req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str());

        ret.trans_response = TransResponseCode::kTransResponseOK;
        ret.trans_status = TransStatus::kTransNonsense;
        ret.trans_err_code = TransErrInfo::kTransSystemErr;
        return -1;
    }

    if (result->rowsCount() == 0){
        log_err("订单未找到：F_merch_id='%s', F_merch_name='%s', F_trans_id='%s'", req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str());

        ret.trans_response = TransResponseCode::kTransResponseOK;
        ret.trans_status = TransStatus::kTransNotFound;
        ret.trans_err_code = TransErrInfo::kTransNoErr;
        return 0;
    }

    int64_t F_status; int64_t F_errcode; int64_t F_amount;
    result->next();
    if(cast_raw_value(result, 1, F_status, F_errcode, F_amount)) {
        log_info("订单状态：F_merch_id='%s', F_merch_name='%s', F_trans_id='%s => %s, %s'", req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str(),
                  get_trans_status_str(F_status).c_str(), get_trans_error_str(F_errcode).c_str());
    } else {
        log_err("查询数据库异常：F_merch_id='%s', F_merch_name='%s', F_trans_id='%s'", req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str());

        ret.trans_response = TransResponseCode::kTransResponseOK;
        ret.trans_status = TransStatus::kTransNonsense;
        ret.trans_err_code = TransErrInfo::kTransSystemErr;

        return -1;
    }

    ret.trans_response = TransResponseCode::kTransResponseOK;
    ret.amount = F_amount;
    ret.trans_status = F_status;
    ret.trans_err_code = F_errcode;

    return 0;
}

int generate_trans_query_ret(const trans_query_response& ret, std::string& post_body) {

    std::map<std::string, std::string> map_param;

    if (ret.trans_response != TransResponseCode::kTransResponseOK && ret.trans_response != TransResponseCode::kTransResponseFail){
        ret.trans_response = TransResponseCode::kTransResponseFail;
    }
    ret.trans_status_str = get_trans_status_str(ret.trans_status);
    ret.trans_err_str = get_trans_error_str(ret.trans_err_code);

    map_param["merch_id"] = ret.merch_id;
    map_param["merch_name"] = ret.merch_name;
    map_param["message_type"] = ret.message_type;
    map_param["trans_id"] = ret.trans_id;
    map_param["account_no"] = ret.account_no;
    map_param["account_name"] = ret.account_name;
    map_param["amount"] = convert_to_string(ret.amount);

    map_param["trans_response"] = convert_to_string(ret.trans_response);
    map_param["trans_status"] = convert_to_string(ret.trans_status);
    map_param["trans_status_str"] = ret.trans_status_str;
    map_param["trans_err_code"] = convert_to_string(ret.trans_err_code);
    map_param["trans_err_str"] = ret.trans_err_str;

    std::string sign;
    if (SignHelper::instance().calc_sign(map_param, sign) != 0) {
        log_err("calc sign error!");
        return -1;
    }

    map_param["sign"] = sign;    // store it

    // build request json
    Json::Value root;
    for(std::map<std::string, std::string>::const_iterator itr = map_param.begin()
        ; itr != map_param.end()
        ; itr++) {
        root[itr->first] = itr->second;
    }

    Json::FastWriter fast_writer;
    post_body = fast_writer.write(root);
    log_debug("Return: %s", post_body.c_str());

    return 0;
}



int generate_trans_batch_submit_ret(const trans_batch_submit_response& ret, std::string& post_body) {

    std::map<std::string, std::string> map_param;

    if (ret.trans_response != TransResponseCode::kTransResponseOK && ret.trans_response != TransResponseCode::kTransResponseFail){
        ret.trans_response = TransResponseCode::kTransResponseFail;
    }
    ret.trans_status_str = get_trans_status_str(ret.trans_status);
    ret.trans_err_str = get_trans_error_str(ret.trans_err_code);

    map_param["merch_id"] = ret.merch_id;
    map_param["merch_name"] = ret.merch_name;
    map_param["message_type"] = ret.message_type;

    map_param["trans_response"] = convert_to_string(ret.trans_response);
    map_param["trans_status"] = convert_to_string(ret.trans_status);
    map_param["trans_status_str"] = ret.trans_status_str;
    map_param["trans_err_code"] = convert_to_string(ret.trans_err_code);
    map_param["trans_err_str"] = ret.trans_err_str;

    std::string sign;
    if (SignHelper::instance().calc_sign(map_param, sign) != 0) {
        log_err("calc sign error!");
        return -1;
    }

    map_param["sign"] = sign;    // store it

    // build request json
    Json::Value root;
    for(std::map<std::string, std::string>::const_iterator itr = map_param.begin()
        ; itr != map_param.end()
        ; itr++) {
        root[itr->first] = itr->second;
    }

    Json::FastWriter fast_writer;
    post_body = fast_writer.write(root);
    log_debug("Return: %s", post_body.c_str());

    return 0;
}


int generate_trans_batch_query_ret(const trans_batch_query_response& ret, std::string& post_body) {

    std::map<std::string, std::string> map_param;

    if (ret.trans_response != TransResponseCode::kTransResponseOK && ret.trans_response != TransResponseCode::kTransResponseFail) {
        ret.trans_response = TransResponseCode::kTransResponseFail;
    }

    // orderLists
    Json::Value ordersJson;
    for (size_t i = 0; i < ret.orders.size(); i++) {
        const trans_query_response &order = ret.orders[i];
        Json::Value orderjson;

        orderjson["trans_id"] = order.trans_id;
        orderjson["account_no"] = order.account_no;
        orderjson["account_name"] = order.account_name;
        orderjson["amount"] = convert_to_string(order.amount);

        orderjson["trans_status"] = convert_to_string(order.trans_status);
        orderjson["trans_status_str"] = get_trans_status_str(order.trans_status);
        orderjson["trans_err_code"] = convert_to_string(order.trans_err_code);
        orderjson["trans_err_str"] = get_trans_error_str(order.trans_err_code);

        ordersJson.append(orderjson);
    }

    Json::FastWriter fast_writer;
    map_param["orders"] = fast_writer.write(ordersJson);

    map_param["merch_id"] = ret.merch_id;
    map_param["merch_name"] = ret.merch_name;
    map_param["message_type"] = ret.message_type;
    map_param["trans_response"] = convert_to_string(ret.trans_response);
    map_param["total_count"] = convert_to_string(ret.total_count);
    map_param["total_amount"] = convert_to_string(ret.total_amount);

    std::string sign;
    if (SignHelper::instance().calc_sign(map_param, sign) != 0) {
        log_err("calc sign error!");
        return -1;
    }

    map_param["sign"] = sign;    // store it

    // build request json
    Json::Value root;
    for(std::map<std::string, std::string>::const_iterator itr = map_param.begin()
        ; itr != map_param.end()
        ; itr++) {
        root[itr->first] = itr->second;
    }

    post_body = fast_writer.write(root);
    log_debug("Return: %s", post_body.c_str());

    return 0;
}