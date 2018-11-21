
#ifndef MQCLIENT_MQCMDINTERPRET_H_
#define MQCLIENT_MQCMDINTERPRET_H_

#define CMD_LEN 3 // 8 commands in total

int mqclient_parse_msg(char *data, char *msg_id, char *key, char *name);

int op_cmd0(char *send_str);
int op_cmd1(char *send_str);
int op_cmd2(char *send_str);
int op_cmd3(char *send_str);
int op_cmd4(char *send_str);
int op_cmd5(char *send_str);
int op_cmd6(char *state);
int op_cmd7(char *state);
int op_cmd8(char *name, char *state);
int op_cmd9(char *name);

#endif /* MQCLIENT_MQCMDINTERPRET_H_ */
