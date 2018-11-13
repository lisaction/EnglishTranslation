#ifndef AMQP_CONSUMER_H_
#define AMQP_CONSUMER_H_

#include <amqp.h>
#include <amqp_framing.h>

amqp_connection_state_t mqclient_init_conn ();
amqp_envelope_t *mqclient_consume_topic(amqp_connection_state_t conn);

void mqclient_reply_rpc(amqp_connection_state_t conn,
		amqp_envelope_t *envelope);
void mqclient_public_direct(amqp_connection_state_t conn,
		amqp_envelope_t *envelope, char *routing_key, char* send_str);

#endif
