#include "messagequeue.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <amqp_framing.h>

#include "mqutils.h"

/* Maybe we need a config file */
#define HOSTNAME "rabbitmq-server" //the the hostname in /etc/hosts of server we connect to
#define TOPIC_EXCHANGE "command" //receive exchange
#define DIRECT_EXCHANGE "reply" //send exchange
#define BINDKEY_BROADCAST "simau"
#define PORT AMQP_PROTOCOL_PORT
#define NAME_LEN 100

// exit if there is an error.
static void mqclient_declare_queue_and_bind(amqp_connection_state_t conn,
		char const *exchange, char const *bindingkey){

	amqp_bytes_t queuename;

	// Declaring queue with random name
	amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1,
			amqp_empty_bytes, 0, 0, 0, 1,amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
	queuename = amqp_bytes_malloc_dup(r->queue);
	if (queuename.bytes == NULL) {
		die("error when copying queue name");
	}

	/* bind queue with an exist exchange created by producer. */
	amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(bindingkey),
			amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

	/* Register a consumer */
	amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");
}

/* Create the mq skfd */
amqp_connection_state_t mqclient_init_conn (){
	char const *hostname;
	char const *exchange;
	char unicast[NAME_LEN];
	char localhostname[NAME_LEN];
	int port, status;
	amqp_socket_t *socket = NULL;
	amqp_connection_state_t conn;

	hostname = HOSTNAME;
	exchange = TOPIC_EXCHANGE;
	port = PORT;
	gethostname(localhostname, NAME_LEN-1);
	sprintf(unicast, "simau.%s", localhostname);

	conn = amqp_new_connection();
	socket = amqp_tcp_socket_new (conn);
	if (!socket){
		die("creating TCP socket");
	}

	status = amqp_socket_open(socket, hostname, port);
	if (status)	{
		die("opening TCP socket");
	}

	/* Log to server */
	/* Read configuration from a file*/
	// conn state, vhost, channel max (0 no), frame max, heartbeat, method
	// PLAIN method followed by username and password
	die_on_amqp_error(amqp_login(conn, "simau", 0, 131072, 0,
				AMQP_SASL_METHOD_PLAIN, "simau", "simau"),
			"Logging in");
	amqp_channel_open(conn, 1);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

	/* check (or declare?) the exchange */
	amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes("topic"),
			//amqp_empty_bytes, exchange type
			0,//passive: for client to check whether an exchange exists
			0,//durable: be purged if a rabbitmq server restarts
			0,//auto_delete: the exchange is deleted when all queues have finished using it
			0,//internal: if set the exchange may not be used directly by publishers
			amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange:");

	mqclient_declare_queue_and_bind(conn, exchange, unicast);
	mqclient_declare_queue_and_bind(conn, exchange, BINDKEY_BROADCAST);

	return conn;
}

amqp_envelope_t *mqclient_consume_topic(amqp_connection_state_t conn){
	/* amqp_connection_state_t conn = (amqp_connection_state_t )user_data; */
	amqp_rpc_reply_t res;
	amqp_envelope_t *envelope;

	envelope = calloc(1, sizeof(amqp_envelope_t));

	amqp_maybe_release_buffers(conn);
	res = amqp_consume_message(conn, envelope, NULL, 0);

	/* AMQP_RESPONSE_NORMAL - the RPC completed successfully*/
	if (res.reply_type != AMQP_RESPONSE_NORMAL){
		return NULL;
	}

	//dump_envelope(envelope);
	return envelope;
}

void mqclient_reply_rpc(amqp_connection_state_t conn, amqp_envelope_t *envelope)
{
	/* declare exchange */
	/*amqp_exchange_declare(conn, 1, envelope->message.properties.correlation_id, amqp_empty_bytes,
			1,//passive: for client to check whether an exchange exists
			0,//durable: be purged if a rabbitmq server restarts
			0,//auto_delete: the exchange is deleted when all queues have finished using it
			0,//internal: if set the excnage may not be used directly by publishers
			amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange for reply");*/

	/* set properties */
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
		AMQP_BASIC_DELIVERY_MODE_FLAG |
		AMQP_BASIC_CORRELATION_ID_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
	props.correlation_id = envelope->message.properties.correlation_id;

	/* send the mesg (rpc reply) */
	die_on_error(amqp_basic_publish(conn,
				1,
				//envelope->message.properties.correlation_id,
				//envelope->message.properties.reply_to,
				amqp_cstring_bytes("reply"),
				amqp_cstring_bytes("reply"),
				0,
				0,
				&props,
				amqp_cstring_bytes("rpc reply")),
			"Publishing on reply rpc");
	return ;
}

void mqclient_public_direct(amqp_connection_state_t conn, amqp_envelope_t *envelope,
		char *routing_key, char* send_str){
	char const *exchange;
	int len;
	char *ip;
	//char const *routing_key = "callback";

	exchange = DIRECT_EXCHANGE;
	len = 0;

	/* discover direct exchange */
	amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_empty_bytes,
			1,//passive: for client to check whether an exchange exists
			0,//durable: be purged if a rabbitmq server restarts
			0,//auto_delete: the exchange is deleted when all queues have finished using it
			0,//internal: if set the excnage may not be used directly by publishers
			amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Discovering exchange for reply");

	/* set properties */
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
		AMQP_BASIC_DELIVERY_MODE_FLAG |
		AMQP_BASIC_CORRELATION_ID_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
	props.correlation_id = envelope->message.properties.correlation_id;
	//printf("correlation_id=%s\n",(char *) props.correlation_id.bytes);

	/* send the rpc rely mesg */
	die_on_error(amqp_basic_publish(conn,
				1,
				amqp_cstring_bytes(exchange),
				amqp_cstring_bytes(routing_key),
				0,
				0,
				&props,
				amqp_cstring_bytes(send_str)),
			"Publishing on direct exchange");
	return ;
}

