#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "messagequeue.h"
#include "mqcommand.h"
#include "mqutils.h"

int mqclient_handle_message(amqp_connection_state_t conn,
		amqp_envelope_t *envelope){
	int cmd, rv;
	char *send_str;
	char *data = calloc((int) envelope->message.body.len+1 ,sizeof(char));
	char msgid[200];
	strncpy(data, (char *)envelope->message.body.bytes, (int) envelope->message.body.len);

	// get command code
	cmd = mqclient_parse_msg(data, msgid);
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
		rv = -1;
		fprintf(stderr, "Warning: Unknown command.\n");
	}

	//if (rv == 2)
	mqclient_public_direct(conn, envelope, msgid, "rpc reply");

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

	conn = mqclient_init_conn();
	while (1){
		/* block to consume */
		envelope = mqclient_consume_topic(conn);

		mqclient_handle_message(conn, envelope);
		amqp_destroy_envelope(envelope);
	}

	return 0;
}
