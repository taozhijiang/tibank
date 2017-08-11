#include <sstream>

#include "SignHelper.h"
#include "Utils.h"
#include "TransProcess.h"
#include "Log.h"
#include "ServiceManager.h"

#include "json/json.h"

std::string get_trans_status_str(enum TransStatus stat) {

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
        default:
            ret = "未知状态";
            log_error("未知状态：%d", stat);
    }

    return ret;
}

std::string get_trans_err_str(enum TransErrCode code) {

    std::string ret;

    switch (code){
        case kTransErrOK:
            ret = "交易成功";
            break;
        case kTransErrDup:
            ret = "重复交易";
            break;
        case kTransErrParam:
            ret = "参数错误";
            break;
        case kTransErrBalance:
            ret = "余额不足";
            break;
        case kTransErrNotFound:
            ret = "订单不存在";
            break;
        case kTransErrUnknow:
            ret = "不明错误";
            break;

        default:
            ret = "未知错误";
            log_error("未知错误：%d", code);
    }

    return ret;
}

#if 0

 CREATE TABLE `tibank.t_trans_order_201708` (
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
    char buff[4096];
    sprintf(buff, "INSERT INTO tibank.t_trans_order_201708 SET F_merch_id='%s', F_merch_name='%s', F_trans_id='%s', F_account_no='%s', "
                   "F_account_name='%s', F_amount=%ld, F_account_type=%d, F_branch_no='%s', F_branch_name='%s', F_remarks='%s', "
                   "F_status=%d, F_errcode=0, F_create_time=NOW(), F_update_time=NOW() ",
                    req.merch_id.c_str(), req.merch_name.c_str(), req.trans_id.c_str(), req.account_no.c_str(),
                    req.account_name.c_str(), req.amount, req.account_type, req.branch_no.c_str(),
                    req.branch_name.c_str(), req.remarks.c_str(), TransStatus::kTransInProcess );
    if (conn->execute_command(buff)) {
        return -1;
    }

    ret.trans_status = TransStatus::kTransInProcess;
    ret.trans_status_str = get_trans_status_str(TransStatus::kTransInProcess);
    ret.trans_err_code = TransErrCode::kTransErrOK;
    ret.trans_err_str = get_trans_err_str(TransErrCode::kTransErrOK);

    return 0;
}

int generate_trans_submit_ret(const trans_submit_response& ret, std::string& post_body) {

    std::map<std::string, std::string> map_param;

    map_param["merch_id"] = ret.merch_id;
    map_param["merch_name"] = ret.merch_name;
    map_param["message_type"] = ret.message_type;
    map_param["trans_id"] = ret.trans_id;
    map_param["account_no"] = ret.account_no;
    map_param["account_name"] = ret.account_name;
    map_param["trans_status"] = convert_to_string(ret.trans_status);
    map_param["trans_status_str"] = ret.trans_status_str;
    map_param["trans_err_code"] = convert_to_string(ret.trans_err_code);
    map_param["trans_err_str"] = ret.trans_err_str;

    std::string sign;
    if (SignHelper::instance().calc_sign(map_param, sign) != 0) {
        log_error("calc sign error!");
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

    ret.trans_status = TransStatus::kTransInProcess;
    ret.trans_status_str = get_trans_status_str(TransStatus::kTransInProcess);

    ret.trans_err_code = TransErrCode::kTransErrOK;
    ret.trans_err_str = get_trans_err_str(TransErrCode::kTransErrOK);

    return 0;
}


int generate_trans_query_ret(const trans_query_response& ret, std::string& post_body) {

    std::map<std::string, std::string> map_param;

    map_param["merch_id"] = ret.merch_id;
    map_param["merch_name"] = ret.merch_name;
    map_param["message_type"] = ret.message_type;
    map_param["trans_id"] = ret.trans_id;
    map_param["account_no"] = ret.account_no;
    map_param["account_name"] = ret.account_name;
    map_param["trans_status"] = convert_to_string(ret.trans_status);
    map_param["trans_status_str"] = ret.trans_status_str;
    map_param["trans_err_code"] = convert_to_string(ret.trans_err_code);
    map_param["trans_err_str"] = ret.trans_err_str;

    std::string sign;
    if (SignHelper::instance().calc_sign(map_param, sign) != 0) {
        log_error("calc sign error!");
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