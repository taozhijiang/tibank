#include "EQueue.h"

#include "Log.h"
#include "Utils.h"
#include "TimerService.h"
#include "TransProcess.h"

#include "TransProcessTask.h"

extern const char* TiBANK_DATABASE_PREFIX;

bool TransProcessTask::init() {

	if (! trans_process_task_.init_threads(boost::bind(&TransProcessTask::trans_process_task_run, shared_from_this(),_1))) {
		log_error("TransProcessTask::init failed!");
		return false;
	}

	// customized init ...

	return true;
}


int do_process_task(EQueueDataPtr qd) {
    sql_conn_ptr conn;
    int nResult = 0;

    request_scoped_sql_conn(conn);
    safe_assert(conn);
    if (!conn){
        log_error("Get SQL connection failed!");
        return -1;
    }

    int F_status = 0;
    int F_errcode = 0;

    do {
        shared_result_ptr result(conn->sqlconn_execute_query(
                                    va_format(" SELECT F_account_no, F_account_name, F_amount, F_branch_no, F_branch_name, F_status FROM %s.t_trans_order_%02d "
                                               " WHERE F_merch_id='%s' AND F_trans_id='%s' AND F_account_type=%d LIMIT 1 ",
                                                TiBANK_DATABASE_PREFIX, get_db_table_index(qd->trans_id_),
                                                qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_)));

        if (!result || result->rowsCount() != 1){
            log_error("Failed to query trans order info" );
            return -1;
        }

        std::string F_account_no;
        std::string F_account_name;
        int64_t F_amount = 0;
        std::string F_branch_no;
        std::string F_branch_name;
        int F_status_l = 0;

        result->next();
        if (!cast_raw_value(result, 1, F_account_no, F_account_name, F_amount, F_branch_no, F_branch_name, F_status_l)){
            log_error("Failed to cast trans order info ..." );
            nResult = -1;
            break;
        }

        log_trace("Trans Info: %s %s %ld %s %s -> %d", F_account_no.c_str(), F_account_name.c_str(), F_amount,
                                    F_branch_no.c_str(), F_branch_name.c_str(), F_status_l);
        if (F_status_l != TransStatus::kTransInProcess) {
            log_error("Invalid trans status: %d", F_status_l);
            nResult = -1;
            break;
        }

        if (F_amount >= 100*10000*100) { // 单笔不超过100W
            log_error("F_amount error: %ld", F_amount);
            F_status = TransStatus::kTransFail;
            F_errcode = TransResponseCode::kTransResponseAmountErr;
            break;
        }

        if (F_amount % 7 == 0){
            F_status = TransStatus::kTransFail;
            F_errcode = TransResponseCode::kTransResponseUnknownErr;
        } else {
            F_status = TransStatus::kTransSuccess;
            F_errcode = TransResponseCode::kTransResponseOK;
        }

    } while (0);

    if (F_status) {
        int affected = conn->sqlconn_execute_update(
                                    va_format(" UPDATE %s.t_trans_order_%02d SET F_status=%d, F_errcode=%d "
                                               " WHERE F_merch_id='%s' AND F_trans_id='%s' AND F_account_type=%d ",
                                                TiBANK_DATABASE_PREFIX, get_db_table_index(qd->trans_id_),
                                                F_status, F_errcode, qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_));

        if (affected != 1){
            log_error("Failed to update trans order info" );
            nResult = -1;
        } else
            nResult = 0;
    }

    return nResult;
}

void TransProcessTask::trans_process_task_run(ThreadObjPtr ptr) {

	std::stringstream ss_id;
	ss_id << boost::this_thread::get_id();
	log_trace("TransProcessTask thread %s is about to work... ", ss_id.str().c_str());

	// main task loop here!

	int nResult = 0;

	while (true) {

		if (unlikely(ptr->status_ == ThreadStatus::kThreadTerminating)) {
			break;
		}

		if (unlikely(ptr->status_ == ThreadStatus::kThreadSuspend)) {
			::usleep(500*1000);
			continue;
		}

		std::list<EQueueDataPtr> tasks;
		nResult = get_unfinished_trans_process_task(tasks, 5, 60 /*1min*/);
		if (nResult) {
			log_error("Retrive task error!!!!!!");
			::sleep(5);
			continue;
		}

		if (tasks.empty()) {
			// yk_api::log_trace("Retrive back task empty!");
			::sleep(5);
			continue;
		}

		for (std::list<EQueueDataPtr>::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
			if (do_process_task(*it) == 0) {	 //back ok
				log_trace("task finished: (%s %s %d)", (*it)->merch_id_.c_str(), (*it)->trans_id_.c_str(), (*it)->account_type_);
				finish_trans_process_task(*it, TaskStatusType::kTaskStatusFinished);
			}
			else {
				// 继续等待被处理
                touch_trans_process_task(*it);
			}
		}
	}

	ptr->status_ = ThreadStatus::kThreadDead;
	log_trace("TransProcessTask thread %s is about to terminate ... ", ss_id.str().c_str());

	return;
}



int creat_trans_process_task(EQueueDataPtr qd) {

    sql_conn_ptr conn;
    request_scoped_sql_conn(conn);

    safe_assert(conn);
    if (!conn){
        log_error("Get SQL connection failed!");
        return -1;
    }

	time_t next_process_tm = ::time(NULL) + 10 /* 10s */;
    int count_affected = conn->sqlconn_execute_update(
                                va_format("INSERT INTO %s.t_equeue_data "
                                            " SET F_task_type = %d, F_task_status = %d, F_merch_id = '%s', F_trans_id = '%s', F_account_type = %d, "
                                            " F_process_eqtime = %lu, F_process_count = 0, F_next_handle_time = DATE_ADD(NOW(), INTERVAL 5 MINUTE), "
                                            " F_create_time = NOW(), F_update_time = NOW() "
                                            " ON DUPLICATE KEY UPDATE F_task_status = %d, F_process_eqtime = %lu, F_next_handle_time = DATE_ADD(NOW(), INTERVAL 5 MINUTE), F_update_time = NOW()",
                                            TiBANK_DATABASE_PREFIX, TaskType::kTaskTypeTransProcess, TaskStatusType::kTaskStatusWaiting,
                                            qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_, next_process_tm,
                                            TaskStatusType::kTaskStatusWaiting, next_process_tm));

	// on duplicate 生效的时候 affected == 2
	if(count_affected != 1 && count_affected != 2){
		log_error("Failed to insert %s.t_equeue_data for task (%s, %s, %d)",
						  TiBANK_DATABASE_PREFIX, qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_);
        return -1;
	}

	return 0;
}


int touch_trans_process_task(EQueueDataPtr qd) {
    sql_conn_ptr conn;
    request_scoped_sql_conn(conn);

    safe_assert(conn);
    if (!conn){
        log_error("Get SQL connection failed!");
        return -1;
    }

    if (!qd) {
        log_error("Invalid task qd");
        return -1;
    }

    int numAffected = conn->sqlconn_execute_update( va_format(
                				    " UPDATE %s.t_equeue_data SET F_update_time = NOW(), F_task_status = %d "
                                    " WHERE F_task_type = %d AND F_merch_id = '%s' AND F_trans_id = '%s' AND F_account_type = %d",
                                    TiBANK_DATABASE_PREFIX, TaskStatusType::kTaskStatusWaiting, TaskType::kTaskTypeTransProcess,
                                    qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_));

    log_trace("touch task (%s, %s, %d)", qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_);
	return numAffected == 1;
}


int finish_trans_process_task(EQueueDataPtr qd, enum TaskStatusType stat) {

    sql_conn_ptr conn;
    request_scoped_sql_conn(conn);
    safe_assert(conn);

    return finish_trans_process_task(conn, qd, stat);
}

int finish_trans_process_task(sql_conn_ptr conn, EQueueDataPtr qd, enum TaskStatusType stat) {
	int nResult = 0;

    safe_assert(conn);
    if (!conn){
        log_error("Get SQL connection failed!");
        return -1;
    }

    if (!qd){
        log_error("Invalid task qd");
        return -1;
    }

	if (stat != TaskStatusType::kTaskStatusFinished && stat != TaskStatusType::kTaskStatusFailed ) {
		log_error("Invalid task target stat: %d", stat);
		return -1;
	}

	shared_result_ptr result;

	do {
		result.reset(conn->sqlconn_execute_query(
                        va_format( " SELECT F_process_count, F_create_time "
                    				" FROM %s.t_equeue_data "
                    				" WHERE F_task_type = %d AND F_merch_id = '%s' AND F_trans_id = '%s' AND F_account_type = %d LIMIT 1",
                    				 TiBANK_DATABASE_PREFIX, TaskType::kTaskTypeTransProcess,
                                     qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_)));

		if (!result || result->rowsCount() != 1){
			log_error("Failed to query t_equeue_data for finish ..." );
			nResult = -1;
			break;
		}

        int F_process_count;
        std::string F_create_time;
        result->next();

        if (!cast_raw_value(result, 1, F_process_count, F_create_time)){
            log_error("Failed to cast t_equeue_data for finish ..." );
			nResult = -1;
			break;
        }


		// 插入到完成表当中
        // 非核心事务
        int numAffected = conn->sqlconn_execute_update( va_format(
            						 " INSERT INTO %s.t_equeue_data_finished "
            						 " SET F_task_type = %d, F_task_status = %d, F_merch_id = '%s', F_trans_id = '%s', "
                                     " F_account_type = %d, F_process_count = %d, F_create_time = '%s', F_finish_time = NOW()",
                                     TiBANK_DATABASE_PREFIX, TaskType::kTaskTypeTransProcess, stat,
                                     qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_, F_process_count, F_create_time.c_str()));

		if(numAffected != 1){
			log_error("insert finished task to t_equeue_data_finished failed for taskId(%s, %s, %d)",
                                qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_);
            nResult = -1;
		}

			// 从任务队列中删除
        numAffected = conn->sqlconn_execute_update( va_format(
                                 " DELETE FROM %s.t_equeue_data "
                                 " WHERE F_merch_id = '%s' AND F_trans_id = '%s' AND F_account_type = %d AND F_task_type = %d; ",
                                 TiBANK_DATABASE_PREFIX, qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_,
                                 TaskType::kTaskTypeTransProcess));

        if(numAffected != 1){
            log_error("delete finish task failed!");
            nResult = -1;
        }

	} while (0);

	return nResult;
}



int get_unfinished_trans_process_task(EQueueList& qlist, size_t batch_size, size_t hold_time) {

    int nResult = 0;
    sql_conn_ptr conn;
    request_scoped_sql_conn(conn);

    safe_assert(conn);
    if (!conn){
        log_error("Get SQL connection failed!");
        return -1;
    }

    shared_result_ptr result;
	do {
		conn->begin_transaction();

        result.reset(conn->sqlconn_execute_query(
                                va_format("SELECT F_id, F_merch_id, F_trans_id, F_account_type, F_process_count "
                                            " FROM %s.t_equeue_data "
                                            " WHERE F_task_type = %d AND F_process_eqtime < %lu AND DATE(F_create_time) >= DATE_ADD(NOW(),interval -20 DAY) AND "
                                            " ( (F_task_status = %d) OR (F_next_handle_time <= NOW() AND F_task_status = %d) ) "
                                            " ORDER BY F_update_time "
                                            " limit %ld FOR UPDATE ",
                                            TiBANK_DATABASE_PREFIX, TaskType::kTaskTypeTransProcess, ::time(NULL),
                                            TaskStatusType::kTaskStatusWaiting, TaskStatusType::kTaskStatusInProcess, batch_size)));

        if (!result) {
            log_error("查找任务失败！");
            nResult = -1;
            break;
        }

		while(result->next()){

			int64_t F_id = 0;
			std::string F_merch_id;
			std::string F_trans_id;
            int F_account_type = 0;
            int F_process_count = 0;

            if (!cast_raw_value(result, 1, F_id, F_merch_id, F_trans_id, F_account_type, F_process_count)){
                log_error("cast date error!");
                continue;
            }

			EQueueDataPtr qd = boost::make_shared<EQueueData>(F_merch_id, F_trans_id, F_account_type);
			if (!qd) {
				log_error("New process task object failed: '%s', '%s', '%d' ", F_merch_id.c_str(), F_trans_id.c_str(), F_account_type);
				continue;
			}
			qd->process_count_ = F_process_count;

			// 查看回盘操作的次数限制
			if (qd->process_count_ >= 5) {
				log_error("Max process count exceeded: %d - %d, ('%s', '%s', %d)", F_process_count, 5, F_merch_id.c_str(), F_trans_id.c_str(), F_account_type);
				finish_trans_process_task(conn, qd, TaskStatusType::kTaskStatusFailed);
				continue;
			}

			time_t now_tm = ::time(NULL);
			int numAffected = conn->sqlconn_execute_update(
                        va_format(
    						" UPDATE %s.t_equeue_data "
    						" SET F_update_time = NOW(), F_process_eqtime = %lu, F_next_handle_time = FROM_UNIXTIME(%ld), "
    						" F_process_count = F_process_count+1, F_task_status = %d "
    						" WHERE F_id = %ld AND F_task_type = %d ",
    						TiBANK_DATABASE_PREFIX, (now_tm + 10), (now_tm + hold_time),
                            TaskStatusType::kTaskStatusInProcess, F_id, TaskType::kTaskTypeTransProcess));

			if(numAffected != 1){
				log_error("Failed to update %s.t_equeue_data for taskId(%ld)", TiBANK_DATABASE_PREFIX, F_id);
				nResult = -1;
				break;
			}

			// add it!
			qlist.push_back(qd);
		}

	}while(0);

	if (nResult) {
		conn->rollback();
	} else {
		conn->commit();
	}

	return nResult;
}
