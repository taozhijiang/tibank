#ifndef _TiBANK_MQ_CONN_H_
#define _TiBANK_MQ_CONN_H_

#include <boost/optional.hpp>

#include "General.h"

#include "Log.h"
#include "ConnPool.h"
#include "ConnWrap.h"
#include "RabbitMQ.h"


// RabbitMQ针对生产者和消费者，需要对连接做不同的设置，所以这里
// 发送和接受消息的时候，会对连接的模式进行判别，如果不符合就重新
// 进行连接设置，这样对应用者来说将会是透明的—— 同一个连接可以发送、接受消息

class MqConn;
typedef std::shared_ptr<MqConn> mq_conn_ptr;

struct rabbitmq_character_t {
    std::string exchange_name_;
    std::string queue_name_;
    std::string route_key_;
};

struct MqConnPoolHelper {
    friend class MqConn;

public:
    MqConnPoolHelper(const std::vector<std::string>& connect_urls,
                     const std::string& exchange, const std::string& queue, const std::string& route_key):
                     connect_urls_(connect_urls) {
        properity_.exchange_name_ = exchange;
        properity_.queue_name_ = queue;
        properity_.route_key_ = route_key;

        log_info("Info: exchange %s, queue %s, route_key: %s",
                 properity_.exchange_name_.c_str(), properity_.queue_name_.c_str(), properity_.route_key_.c_str());
    }

    MqConnPoolHelper(){}

private:
    std::vector<std::string> connect_urls_;
    rabbitmq_character_t properity_;
};

enum MqWorkState {
    kProducer = 1,
    kConsumer = 2,
    kGetter   = 3,
};


class MqConn: public ConnWrap,
              public boost::noncopyable {
public:
    explicit MqConn(ConnPool<MqConn, MqConnPoolHelper>& pool);
    ~MqConn();

    bool init(int64_t conn_uuid, const MqConnPoolHelper& helper);

private:

    amqp_channel_t   t_;
    AMQP::RabbitMQHelper mq_;
    enum MqWorkState state_;
    MqConnPoolHelper helper_;

    // may be used in future
    ConnPool<MqConn, MqConnPoolHelper>& pool_;

    // consume是推模式的消费，而get是拉模式的消费
    bool mq_setup_channel_publish(AMQP::RabbitChannelPtr pChannel, void* pArg);
    bool mq_setup_channel_consume(AMQP::RabbitChannelPtr pChannel, void* pArg);
    bool mq_setup_channel_get(AMQP::RabbitChannelPtr pChannel, void* pArg);

    bool check_and_set_work_mode(enum MqWorkState state);
    bool check_and_repair();

public:
    // if ret < 0, the connection is ill, user should release it!
    int publish(const std::string& msg);
    int consume(std::string& msgout, int timeout_sec); // 0 block forever
    int get(std::string& msgout);
};



#endif  // _TiBANK_MQ_CONN_H_
