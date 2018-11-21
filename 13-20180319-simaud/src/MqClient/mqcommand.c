#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "src/AuthServer/simaudrulefile.h"
#include "src/AuthServer/simaudruleinterpret.h"
#include "src/AuthServer/simaudauthif.h"
#include "mqcommand.h"

#define NAME_LEN 100

static void copy_para(Rule *rnew, Rule *r){
	struct Parameter *p;
	p = calloc(1, sizeof(Parameter));
	strcpy(p->key, "uid");

	if (!rnew->head || !r->head)
		return;
	if (strcmp(r->head->key, "uid") == 0){
		strcpy(p->value, r->head->value);
	}
	else {
		strcpy(p->value, r->head->next->value);
	}
	p->next = NULL;
	rnew->head->next = p;
}

//delete rule
//read one after one and rewrite one by one
//msg:rule_id;\n
static int delete_rule(Rule *rnew){
	FILE *fp, *fpw;
	int rid, rv, ff;
	char line[RULE_LEN];
	long file_pos;
	Rule *r;
	int pam = 0;

	if (rnew->head)
		pam = 1;

	fp = simaud_open_rule_file();
	fpw = simaud_open_rule_for_write();

	ff = 0;
	file_pos = ftell(fp);
	memset(line, 0, sizeof(line));
	rv = simaud_read_line(line, fp);
	while(rv>0) {
		if (ff){ // have found the rule
			//can we write EOF to file? no ,we can't
			//the line begin with '0' is an abandoned line
			fputc('0', fpw);
			break; // id is unique
		}
		else { // haven't found the rule yet.
			r = simaud_create_rule(line);
		}

		if (r){
			if (!ff && strcmp(r->action_id, rnew->action_id) == 0){ // find the rule
				if ((!pam)||(pam&&simaud_compare_user(rnew, r))){ // for the log rule
					ff = 1;
					rnew->id = r->id;
					strcpy(rnew->des, r->des);
					if (pam) // for log rule
						copy_para(rnew, r);
					fseek(fpw, file_pos, SEEK_SET);
				}
			}
			simaud_delete_rule(r);
			r = NULL;
		}
		file_pos = ftell(fp);
		rv = simaud_read_line(line, fp);
	}

	simaud_close_file(fp);
	simaud_close_file(fpw);
	return 0;
}

//010: add rule
//msg: 1;cas.iie.pam.login;pam-rule;0;uid;1000;user;lin;\n
static int add_rule(Rule *r){
	char rule[RULE_LEN];
	FILE *fpw;
	fpw = simaud_open_rule_for_write();
	fseek(fpw, 0L, SEEK_END); //set pos to tail
	simaud_print_rule(r, rule);
	// Is there an enter?
	simaud_write_line(rule, fpw);
	simaud_close_file(fpw);
	return 0;
}

/* utils : get local ip address */
static char* GetLocalIp(){
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

static void GetHostname(char *localhostname){
	gethostname(localhostname, NAME_LEN-1);
}

/*data={\"msg_id\": \"feedback_request_5a062a1b-11ad-471f-bf95-31fcfbbbd11f\", \"command\": \"001\"}*/
int mqclient_parse_msg(char *data, char *msg_id, char *state, char *name){
	int i, j, len;
	int s, e, c;
	len = strlen(data);
	char t[10][100]; //5 pairs at most

	s = -1;
	j=0; // the number of pair
	for (i=0;i<len;i++){
		if (data[i] == '\"'){
			if (s == -1)
				s = i+1;
			else {
				e = i-1; //find a word
				strncpy(t[j], data+s, e-s+1);
				t[j++][e-s+1] = '\0';
				s = -1;
			}
		}
	}

	for (i=0; i<j; i+=2){
		if (strcmp(t[i], "msg_id") == 0){
			strncpy(msg_id, t[i+1], strlen(t[i+1])+1);
		}
		else if (strcmp(t[i], "command") == 0){
			if (sscanf(t[i+1],"%d",&c) < 1)
				return -1;
		}
		else if (strcmp(t[i], "state") == 0){
			strncpy(state, t[i+1], strlen(t[i+1])+1);
		}
		else if (strcmp(t[i], "name") == 0){
			strncpy(name, t[i+1], strlen(t[i+1])+1);
		}

	}
	return c;
}

/* 0 for no reply
 * 1 for rpc reply
 * 2 for direct reply */
/* 000: get host list */
int op_cmd0(char *send_str){
	// cmd0 return ip and hostname to controller

	char *ip;
	char hostname[NAME_LEN];

	// hostname;
	GetHostname(hostname);
	// ip;
	ip = GetLocalIp();

	/*{\"hostname\": \"lin-virtual-machine\", \"ip\": \"127.0.0.1\"}*/
	sprintf(send_str, "{\"hostname\": \"%s\", \"ip\": \"%s\"}", hostname, ip);
	//printf("i'm the reply_str: %s \n", send_str);
	return 2;
}

/* 001: get mod rule in every host */
int op_cmd1(char *send_str){
	int len, mod_state;
	op_cmd0(send_str);
	len = strlen(send_str);
	len = len - 1; // }

	mod_state =  simaud_authorize_mqclient("cas.iie.kernel-cap.sys-module;\n");
	sprintf(send_str+len, ", \"mod\": \"%d\"}", mod_state);
	return 2;
}

/* 002: get usb rule in every host*/
int op_cmd2(char *send_str){
	int len, usb_state;
	op_cmd0(send_str);
	len = strlen(send_str);
	len = len - 1; // }

	usb_state =  simaud_authorize_mqclient("cas.iie.kernel-device.add;\n");
	sprintf(send_str+len, ", \"usb\": \"%d\"}", usb_state);
	return 2;
}

/* 003: get user list in this host*/
int op_cmd3(char *send_str){
	char line[RULE_LEN+1];
	int rv, len;
	FILE *fp;
	Rule *r;
	char name[20], uid[20];

	if (!(fp=simaud_open_rule_file()))
		return -1;

	/* read line */
	if ((rv = simaud_read_line(line, fp)) <0){
		simaud_close_file(fp);
		return -1;
	}

	memset(send_str, 0, sizeof(send_str));
	strcat(send_str, "[");
	len = strlen(send_str);
	while (rv > 0){
		r = simaud_create_rule(line);
		if (strcmp(r->action_id, "cas.iie.pam.login") == 0){
			if (strcmp(r->head->key, "user") == 0){
				strcpy(name, r->head->value);
				strcpy(uid, r->head->next->value);
			}
			else {
				strcpy(uid, r->head->value);
				strcpy(name, r->head->next->value);
			}
			sprintf(send_str+len, "{\"user\": \"%s\", \"uid\": \"%s\"}, ", name, uid);
			len = strlen(send_str);
		}
		free(r);
		rv = simaud_read_line(line, fp);
	}

	simaud_close_file(fp);
	len = len - 2; // , and space
	send_str[len] = ']';
	send_str[len+1] = '\0';
	return 2;
}

/* 004: get mod state*/
int op_cmd4(char *send_str){
	int mod_state;
	mod_state =  simaud_authorize_mqclient("cas.iie.kernel-cap.sys-module;\n");
	sprintf(send_str, "%d", mod_state);
	return 2;
}

/* 005: get usb state*/
int op_cmd5(char *send_str){
	int usb_state;
	usb_state =  simaud_authorize_mqclient("cas.iie.kernel-device.add;\n");
	sprintf(send_str, "%d", usb_state);
	return 2;
}

/* 006: update mod state*/
int op_cmd6(char *state){
	Rule r;
	int mod_state;
	sscanf(state, "%d", &mod_state);
	strcpy(r.action_id, "cas.iie.kernel-cap.sys-module");
	r.res = mod_state;
	r.head = NULL;
	delete_rule(&r);
	add_rule(&r);
	return 0;
}

/* 007: update USB state*/
int op_cmd7(char *state){
	Rule r;
	int usb_state;
	sscanf(state, "%d", &usb_state);
	strcpy(r.action_id, "cas.iie.kernel-device.add");
	r.res = usb_state;
	r.head = NULL;
	delete_rule(&r);
	add_rule(&r);
	return 0;
}

/* 008: update User state*/
int op_cmd8(char *name, char *state){
	Rule r;
	int user_state;
	sscanf(state, "%d", &user_state);
	strcpy(r.action_id, "cas.iie.pam.login");
	r.res = user_state;
	r.head = calloc(1, sizeof(struct Parameter));
	strcpy(r.head->key, "user");
	strcpy(r.head->value, name);
	r.head->next = NULL;
	delete_rule(&r);
	add_rule(&r);
	return 0;
}

/* 009: delete user */
int op_cmd9(char *name){
	Rule r;
	strcpy(r.action_id, "cas.iie.pam.login");
	r.head = calloc(1, sizeof(struct Parameter));
	strcpy(r.head->key, "user");
	strcpy(r.head->value, name);
	r.head->next = NULL;
	delete_rule(&r);
}
