
#ifndef MQCLIENT_MQCMDINTERPRET_H_
#define MQCLIENT_MQCMDINTERPRET_H_

#define CMD_LEN 3 // 8 commands in total

int mqclient_parse_msg(char *data, char *msg_id);

int op_cmd0(char *msg, char *send_str);
int op_cmd1(char *msg);
int op_cmd2(char *msg);
int op_cmd3(char *msg);

#endif /* MQCLIENT_MQCMDINTERPRET_H_ */
