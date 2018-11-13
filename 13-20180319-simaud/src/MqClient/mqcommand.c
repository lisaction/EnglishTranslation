#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "src/AuthServer/simaudrulefile.h"
#include "src/AuthServer/simaudruleinterpret.h"
#include "mqcommand.h"

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


// the first CMD_LEN is command
// \"command\": \"001\"},
static int mqclient_get_command(char *data){
	int len=strlen(data);
	char cmd[CMD_LEN+1];
	memset(cmd,0,sizeof(cmd));
	int c, i, pos=0;
	for (i=0;i<len;i++){
		if (data[i]==':'){
			pos=i;
			break;
		}
	}

	if (pos==0 || pos>=len)
		return -1; //wrong string
	pos=pos+3; //space and "
	strncpy(cmd, data+pos, CMD_LEN);
	if (sscanf(cmd,"%d",&c) < 1)
		return -1;
	return c;
}

//\{"msg_id\": \"feedback_request_5a062a1b-11ad-471f-bf95-31fcfbbbd11f\"
//pos(:)=9
static int mqclient_get_msgid(char *data, char *msg_id, int len){
	int i, pos=0;
	for (i=0;i<len;i++)
		if (data[i]==':'){
			pos=i;
			break;
		}

	if (pos==0 || pos>=len)
		return -1; //wrong string
	pos=pos+3; //: space and "
	strncpy(msg_id, data+pos, len-pos-1); // "
	return 0;
}

/*data={\"msg_id\": \"feedback_request_5a062a1b-11ad-471f-bf95-31fcfbbbd11f\", \"command\": \"001\"}*/
//pos(,)=66
int mqclient_parse_msg(char *data, char *msg_id){
	int i, len, pos;
	len = strlen(data);

	for (i=0;i<len;i++){
		if (data[i] == ',')
			break;
	}
	if (i<len){ //find','
		pos = i;
	}
	else { // no ','
		return -1;
	}
	mqclient_get_msgid(data, msg_id, pos);
	return mqclient_get_command(data+pos+2); //, and space
}

/* 0 for no reply
 * 1 for rpc reply
 * 2 for direct reply */
/* 000: read rule configuration */
int op_cmd0(char *msg, char *send_str){
	// cmd1 is return all configuration to controller
	/* create the respond string */
/*	memset(reply_str, 0, sizeof(reply_str));
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
	printf("i'm the reply_str:\n%s", reply_str);*/
	send_str = "i'm op_cmd1\n";
	return 2;
}

//001: delete rule
//read one after one and rewrite one by one
//msg:rule_id;\n
int op_cmd1(char *msg){
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
int op_cmd2(char *msg){
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
