#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <amqp_framing.h>

#include "simaudmqconsumer.h"
#include "simaudmqutils.h"
#include "simaudauthority.h"

/* TODO: Maybe we need a config file*/
/* the server to connect to, we need configure the /etc/hosts */
#define HOSTNAME "rabbitmq-server"
#define TOPIC_EXCHANGE "command"
#define DIRECT_EXCHANGE "echo"
//#define BINDKEY_UNICAST "simau."HOSTNAME
#define BINDKEY_BROADCAST "simau"
#define PORT AMQP_PROTOCOL_PORT
#define CMD_LEN 3
#define NAME_LEN 100

/*TODO: handle all die(), remove utils.c from our source */

/* utils : get local ip address */
char* GetLocalIp(){
	int MAXINTERFACES=16;
	char *ip = NULL;
	int fd, intrface, retn = 0;
	struct ifreq buf[MAXINTERFACES];
	struct ifconf ifc;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)	{
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t)buf;
		if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)){
			intrface = ifc.ifc_len / sizeof(struct ifreq);

			while (intrface-- > 0) {
				if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface]))) {
					ip=(inet_ntoa(
					((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
					break;
				}
			}
		}
		close (fd);
		return ip;
	}
	return NULL;
}

void declare_queue_and_bind(amqp_connection_state_t conn,
		char const *exchange, char const *bindingkey){

	amqp_bytes_t queuename;

	// Declaring queue with random name
	amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1,
			amqp_empty_bytes, 0, 0, 0, 1,amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
	queuename = amqp_bytes_malloc_dup(r->queue);
	if (queuename.bytes == NULL) {
		perror("error when copying queue name");
		return;

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
amqp_connection_state_t create_mqconn (){
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

	/* Login. TODO: Read from config file */
	die_on_amqp_error(amqp_login(conn, "simau", 0, 131072, 0,
				AMQP_SASL_METHOD_PLAIN, "simau", "simau"),
			"Logging in");
	amqp_channel_open(conn, 1);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

	/* check (or declare?) the exchange */
	amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_empty_bytes,
			1,//passive: for client to check whether an exchange exists
			0,//durable: be purged if a rabbitmq server restarts
			0,//auto_delete: the exchange is deleted when all queues have finished using it
			0,//internal: if set the excnage may not be used directly by publishers
			amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange for reply");

	declare_queue_and_bind(conn, exchange, unicast);
	declare_queue_and_bind(conn, exchange, BINDKEY_BROADCAST);

	return conn;
}



static void rpc_reply(amqp_connection_state_t conn, amqp_envelope_t *envelope)
{
	/* declare exchange */
	amqp_exchange_declare(conn, 1, envelope->message.properties.correlation_id, amqp_empty_bytes,
			1,//passive: for client to check whether an exchange exists
			0,//durable: be purged if a rabbitmq server restarts
			0,//auto_delete: the exchange is deleted when all queues have finished using it
			0,//internal: if set the excnage may not be used directly by publishers
			amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange for reply");

	/* set properties */
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
		AMQP_BASIC_DELIVERY_MODE_FLAG |
		AMQP_BASIC_CORRELATION_ID_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
	props.correlation_id = envelope->message.properties.correlation_id;

	/* send the rpc rely mesg */
	die_on_error(amqp_basic_publish(conn,
				1,
				envelope->message.properties.correlation_id,
				envelope->message.properties.reply_to,
				0,
				0,
				&props,
				amqp_cstring_bytes("rpc rely")),
			"Publishing on reply rpc");
	return ;
}

// the first CMD_LEN is command
// the string is formatted like 001;
int get_command_code(void const *buffer, size_t len){
	char cmd[CMD_LEN+1];
	memset(cmd,0,sizeof(cmd));
	int c;
	strncpy(cmd, buffer, CMD_LEN);
	sscanf(cmd,"%d",&c);
	return c;
}

static void operation_on_cmd2(amqp_connection_state_t conn, 
		amqp_envelope_t *envelope){
	// cmd2 is an modification command
	modify_rule_and_rewrite((char *)envelope->message.body.bytes+CMD_LEN+1);	

}

static void operation_on_cmd3(amqp_connection_state_t conn, 
		amqp_envelope_t *envelope){
	// cmd3 is an add command
	add_new_rule_by_cmd((char *)envelope->message.body.bytes+CMD_LEN+1);	
}

static void operation_on_cmd4(amqp_connection_state_t conn, 
		amqp_envelope_t *envelope){
	// cmd2 is an delete command
	int ruleid;
	sscanf((char *)envelope->message.body.bytes+CMD_LEN+1,"%d", &ruleid);
	delete_rule(ruleid);
}

static void reply_to_cmd1(amqp_connection_state_t conn, amqp_envelope_t *envelope){
	// cmd1 is return all configuration to controller
	char const *exchange;
	char hostname[NAME_LEN];
	char reply_str[LEN_OF_UNIT_IN_RULE*LEN_OF_UNIT_IN_RULE];
	int len;
	char *ip;
	// cmd1 is using this queue!
	char const *routing_key = "callback";
	
	exchange = DIRECT_EXCHANGE;
	gethostname(hostname, NAME_LEN-1);
	memset(reply_str, 0, sizeof(reply_str));
	len = 0;

	/* declare exchange 'echo' */
	amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_empty_bytes,
			1,//passive: for client to check whether an exchange exists
			0,//durable: be purged if a rabbitmq server restarts
			0,//auto_delete: the exchange is deleted when all queues have finished using it
			0,//internal: if set the excnage may not be used directly by publishers
			amqp_empty_table);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange for reply");

	/* create the respond string */
	// hostname;
	strncpy(reply_str, hostname, strlen(hostname));
	strcat(reply_str, ";");
	len = strlen(hostname)+1;
	// ip;
	ip = GetLocalIp();	
	strncat(reply_str, ip, strlen(ip));
	strcat(reply_str,";");
	len = len + strlen(ip) + 1;
	// cmd
	strcat(reply_str, "001;\n");
	len = len + 5;
	// rule
	simaud_print_conf_tostr(reply_str + len);
	printf("i'm the reply_str:\n%s", reply_str);

	/* set properties */
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
		AMQP_BASIC_DELIVERY_MODE_FLAG |
		AMQP_BASIC_CORRELATION_ID_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
	props.correlation_id = envelope->message.properties.correlation_id;
	printf("correlation_id=%s\n",(char *) props.correlation_id.bytes);

	/* send the rpc rely mesg */
	die_on_error(amqp_basic_publish(conn,
				1,
				amqp_cstring_bytes(exchange),
				amqp_cstring_bytes(routing_key),
				0,
				0,
				&props,
				amqp_cstring_bytes(reply_str)),
			"Publishing on reply rpc");
	return ;

}

bool topic_consumer(amqp_connection_state_t conn){
	/* amqp_connection_state_t conn = (amqp_connection_state_t )user_data; */
	amqp_rpc_reply_t res;
	amqp_envelope_t envelope;
	int cmd;

	amqp_maybe_release_buffers(conn);
	res = amqp_consume_message(conn, &envelope, NULL, 0);

	if (AMQP_RESPONSE_NORMAL != res.reply_type){
		return false;
	}

	printf("----\n");
	printf("Delivery %u, exchange %.*s routingkey %.*s\n",
			(unsigned) envelope.delivery_tag,
			(int) envelope.exchange.len, (char *) envelope.exchange.bytes,
			(int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);
	if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
	{
		printf("Content-type: %.*s\n",
				(int) envelope.message.properties.content_type.len,
				(char *) envelope.message.properties.content_type.bytes);
	}
	/* amqp_dump(envelope.message.body.bytes, envelope.message.body.len); */
	printf("content: %.*s\n", (int) envelope.message.body.len,
			(char *)envelope.message.body.bytes);

	// get command code
	cmd = get_command_code(envelope.message.body.bytes, envelope.message.body.len);
	// dispatch
	if (cmd == 1){
		// return info of this host
		reply_to_cmd1(conn, &envelope);
	}
	else if (cmd == 2){
		//content:002;1;cas.iie.pam.login;pam-rule;0;user=lin:uid=1001:
		operation_on_cmd2(conn, &envelope);
	}
	else if (cmd==3){
		//content: 003;56;iie-add;add-test;1;
		operation_on_cmd3(conn, &envelope);
	}
	else if (cmd == 4){
		//content: 004;56;
		operation_on_cmd4(conn, &envelope);
	}

	if (envelope.message.properties._flags & AMQP_BASIC_REPLY_TO_FLAG)
		rpc_reply(conn, &envelope);

	amqp_destroy_envelope(&envelope);

	return 0;
}
