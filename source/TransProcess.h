#ifndef _TiBANK_TRANS_PROCESS_H_
#define _TiBANK_TRANS_PROCESS_H_

#include <string>

enum TransStatus {
    kTransSubmitFail = 1,
    kTransSuccess = 2,
    kTransFail = 3,
	kTransInProcess = 4,
    kTransNotFound = 5,
    kTransNonsense = 6,
};

std::string get_trans_status_str(int code);

enum TransResponseCode {
    kTransResponseOK = 0,
    kTransResponseDupReqErr = 1,
    kTransResponseParamErr = 2,
    kTransResponseBalanceErr = 3,
    kTransResponseAmountErr = 4,
    kTransResponseSystemErr = 5,
    kTransResponseUnknownErr = 6,
};

std::string get_trans_response_str(int code);

struct trans_submit_request {
    std::string merch_id;
    std::string merch_name;

    std::string trans_id;
    std::string account_no;
    std::string account_name;
    int64_t      amount;
    int32_t      account_type; // 1对私
    std::string branch_name;
    std::string branch_no;
    std::string remarks;
};

struct trans_submit_response {
    std::string merch_id;
    std::string merch_name;
    std::string message_type;
    std::string trans_id;
    std::string account_no;
    std::string account_name;

    int32_t      trans_resp_code;
    mutable std::string trans_resp_str;
    int32_t      trans_status;
    mutable std::string trans_status_str;
};



struct trans_query_request {
    std::string merch_id;
    std::string merch_name;

    std::string trans_id;
    std::string account_no;
    std::string account_name;
    int32_t      account_type; // 1对私
};

struct trans_query_response {
    std::string merch_id;
    std::string merch_name;
    std::string message_type;
    std::string trans_id;
    std::string account_no;
    std::string account_name;

    int32_t      trans_resp_code;
    mutable std::string trans_resp_str;
    int32_t      trans_status;
    mutable std::string trans_status_str;
};


int process_trans_submit(const struct trans_submit_request& req, struct trans_submit_response& ret);
int generate_trans_submit_ret(const trans_submit_response& ret, std::string& post_body);

int process_trans_query(const struct trans_query_request& req, struct trans_query_response& ret);
int generate_trans_query_ret(const trans_query_response& ret, std::string& post_body);


#define CHECK_CHAR(x) ( x >= '0' && x <='9' )

inline void get_db_table_index(std::string trans_id, int& tb1, int& tb2) {

    if (trans_id.size() >= 2 && CHECK_CHAR(trans_id[trans_id.size()-1]) && CHECK_CHAR(trans_id[trans_id.size()-2])) {
        tb1 = trans_id[trans_id.size()-2] - '0';
        tb2 = trans_id[trans_id.size()-1] - '0';
	} else {
		log_err("Invalid strPartnerOrderId %s ", trans_id.c_str());
		tb1 = 0; tb2 = 0;
	}
}

inline int get_db_table_index(std::string trans_id) {

    if (trans_id.size() >= 2 && CHECK_CHAR(trans_id[trans_id.size()-1]) && CHECK_CHAR(trans_id[trans_id.size()-2])) {
        return 0;
	}

    return (trans_id[trans_id.size()-2] - '0')*10 + (trans_id[trans_id.size()-1] - '0');
}
#undef CHECK_CHAR

#endif //_TiBANK_TRANS_PROCESS_H_
