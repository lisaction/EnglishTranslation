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

void GetHostname(char *localhostname){
	gethostname(localhostname, NAME_LEN-1);
}

/*data={\"msg_id\": \"feedback_request_5a062a1b-11ad-471f-bf95-31fcfbbbd11f\", \"command\": \"001\"}*/
int mqclient_parse_msg(char *data, char *msg_id){
	int i, j, len;
	int s, e, c;
	len = strlen(data);
	char t[5][100]; //only 2 pair

	s = -1;
	j=0;
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

	if (strcmp(t[0], "msg_id") == 0){
		i=1;
		j=3;
	}
	else {
		i=3;
		j=1;
	}
	strncpy(msg_id, t[i], strlen(t[i]));
	if (sscanf(t[j],"%d",&c) < 1)
		return -1;
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

	mod_state =  simaud_authorize_mqclient("cas.iie.kernel-cap.sys-module;");
	sprintf(send_str+len, ", \"mod\": \"%d\"}", mod_state);
	return 2;
}

/* 002: get usb rule in every host*/
int op_cmd2(char *send_str){
	int len, usb_state;
	op_cmd0(send_str);
	len = strlen(send_str);
	len = len - 1; // }

	usb_state =  simaud_authorize_mqclient("cas.iie.kernel-device.add;");
	sprintf(send_str+len, ", \"usb\": \"%d\"}", usb_state);
	return 2;
}

//001: delete rule
//read one after one and rewrite one by one
//msg:rule_id;\n
int op_cmd1_delete(char *msg){
	FILE *fp, *fpw;
	int rid, rv, ff;
	char line[RULE_LEN];
	long file_pos;
	Rule *r;

	fp = simaud_open_rule_file();
	fpw = simaud_open_rule_for_write();

	sscanf(msg,"%d", &rid);

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
			if (!ff && r->id == rid){ // find the rule
				ff = 1;
				fseek(fpw, file_pos, SEEK_SET);
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
int op_cmd2_add(char *msg){
	FILE *fpw;
	fpw = simaud_open_rule_for_write();
	fseek(fpw, 0L, SEEK_END); //set pos to tail
	// Is there an enter?
	simaud_write_line(msg, fpw);
	simaud_close_file(fpw);
	return 0;
}

// 011: modify rule
// msg: 1;cas.iie.pam.login;pam-rule;0;uid;1000;user;lin;\n
int op_cmd3(char *msg){
	op_cmd1(msg); //delete the rule
	op_cmd2(msg); //add new rule
	return 0;
}
