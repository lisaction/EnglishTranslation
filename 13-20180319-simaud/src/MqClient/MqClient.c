#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "messagequeue.h"
#include "mqcommand.h"

int mqclient_handle_message(amqp_connection_state_t conn,
		amqp_envelope_t *envelope){
	int cmd, rv;
	char *send_str;
	char *data = calloc((int) envelope->exchange.len+1 ,sizeof(char));
	strncpy(data, (char *)envelope->exchange.bytes, (int) envelope->exchange.len);

	// get command code
	cmd = mqclient_get_command(data);
	// dispatch
	switch(cmd){
	case 0: //read
		rv = op_cmd0(data+CMD_LEN+1, send_str);
		break;
	case 1: //delete
		rv = op_cmd1(data+CMD_LEN+1);
		break;
	case 2: //add
		rv = op_cmd2(data+CMD_LEN+1);
		break;
	case 3: //modify
		rv = op_cmd3(data+CMD_LEN+1);
		break;
	default:
		fprintf(stderr, "Warning: Unknown command.\n");
	}

	if (envelope->message.properties._flags & AMQP_BASIC_REPLY_TO_FLAG)
		mqclient_reply_rpc(conn, envelope);
	if (rv == 2)
		mqclient_public_2direct(conn, envelope, send_str);

	if (send_str)
		free(send_str);
	free(data);
	return 1;
}

int main(int argc, char **argv){
	amqp_connection_state_t conn;
	amqp_envelope_t *envelope;

	/* opt */
	if (argc > 1){
		if (strcmp(argv[1],"--no-debug")==0){
			/* printf("test here.\n"); */
			freopen("/dev/null","w", stdout);
			freopen("/dev/null","w", stdin);
			freopen("/dev/null","w", stderr);
		}
	}

	conn=mqclient_init_conn();
	while (1){
		/* block to consume */
		envelope = mqclient_consume_message(conn);

		mqclient_handle_message(conn, envelope);
		amqp_destroy_envelope(envelope);
	}

	return 0;
}
