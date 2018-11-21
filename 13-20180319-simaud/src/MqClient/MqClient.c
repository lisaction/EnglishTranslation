#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "messagequeue.h"
#include "mqcommand.h"
#include "mqutils.h"

int mqclient_handle_message(amqp_connection_state_t conn,
		amqp_envelope_t *envelope){
	int cmd, rv;
	char state[200], name[200];
	const int len_of_reply=250;
	char *send_str, *data, msgid[200];

	/* get data */
	data = calloc((int) envelope->message.body.len+1 ,sizeof(char));
	if (data == NULL){
		fprintf(stderr, "Error: No space left.\n");
		return -1;
	}
	strncpy(data, (char *)envelope->message.body.bytes, (int) envelope->message.body.len);
	printf("Get data: %s.\n", data);

	/* create the responding string */
	send_str = calloc (len_of_reply, sizeof(char));
	if (send_str == NULL){
		fprintf(stderr, "Error: No space left.\n");
		free(data);
		return -1;
	}

	// get command code
	cmd = mqclient_parse_msg(data, msgid, state, name);
	// dispatch
	switch(cmd){
	case 0: //get host list
		rv = op_cmd0(send_str);
		break;
	case 1: //get mod rule in every host
		rv = op_cmd1(send_str);
		break;
	case 2: //get usb rule in every host
		rv = op_cmd2(send_str);
		break;
	case 3: //get user list in this host
		rv = op_cmd3(send_str);
		break;
	case 4: //get mod state
		rv = op_cmd4(send_str);
		break;
	case 5: //get usb state
		rv = op_cmd5(send_str);
		break;
	case 6: //update mod state
		rv = op_cmd6(state);
		break;
	case 7: //update usb state
		rv = op_cmd7(state);
		break;
	case 8: //update user state
		rv = op_cmd8(name, state);
		break;
	case 9: //delete user
		rv = op_cmd9(name);
		break;
	default:
		rv = -1;
		fprintf(stderr, "Warning: Unknown command.\n");
	}

	if (rv == 2)
		mqclient_public_direct(conn, envelope, msgid, send_str);

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
