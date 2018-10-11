#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
// the string is formatted like 001;
int mqclient_get_command(char *data){
	char cmd[CMD_LEN+1];
	memset(cmd,0,sizeof(cmd));
	int c;
	strncpy(cmd, data, CMD_LEN);
	sscanf(cmd,"%d",&c);
	return c;
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
//read one after and and rewrite one by one
//msg:rule_id
int op_cmd1(char *msg){
	int ruleid;
	sscanf(msg,"%d", &ruleid);

	return 0;
}

//010: add rule
//msg: 1;cas.iie.pam.login;pam-rule;0;uid=1000:user=lin:\n
int op_cmd2(char *msg){
	return 0;
}

// 011: modify rule
// msg: 1;cas.iie.pam.login;pam-rule;0;uid=1000:user=lin:\n
int op_cmd3(char *msg){
	return 0;
}
