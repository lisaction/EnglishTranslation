#ifndef AMQP_CONSUMER
#define AMQP_CONSUMER

#include <amqp.h>
#include <stdbool.h>
amqp_connection_state_t create_mqconn();
bool topic_consumer(amqp_connection_state_t conn);

#endif
