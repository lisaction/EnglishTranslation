#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <linux/netlink.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "simaudkernelif.h"
#include "simauduserif.h"
#include "simaudruleinterpret.h"

/* Debug kernel is a tough task.
 * So, I edit this file as little as possible. */

// netlink family defined by myself in kernel (polkit.h)
#define NETLINK_POLKITD 23
// local id
#define USER_PORT 12517
// MSG_LEN from kernel
#define MSG_LEN 700

int simaud_create_netlink_fd (){
	int nlfd;
	/* address */
	struct sockaddr_nl local;
	/* temp return */
	int ret;

	/* Create socket */
	nlfd = socket (AF_NETLINK, SOCK_RAW, NETLINK_POLKITD);
	if (nlfd == -1)	{
		die_on_error("create netlink socket");
	}

	/* fill the local address */
	memset (&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pid = USER_PORT;
	local.nl_groups = 0;

	/* bind the socket with the local address */
	ret = bind (nlfd, (struct sockaddr *)&local, sizeof(local));
	if (ret == -1 )	{
		die_on_error("bind netlink socket");
	}
	return nlfd;
}

/* 1 - success
 * -1 - failure */
int simaud_recvfrom_kernel (int nlfd, char *data){
	int ret;
	struct nlmsghdr *nlh = NULL;

	/* allocate space for the msg */
	nlh = (struct nlmsghdr *) malloc ( NLMSG_SPACE(MSG_LEN));
	if (!nlh){
		perror("allocate space for nlh");
		return -1;
	}
	memset (nlh, 0, NLMSG_SPACE(MSG_LEN));

	/* recv msg */
	ret = recvfrom (nlfd ,nlh, NLMSG_SPACE(MSG_LEN), 0, NULL, NULL);
	if (ret < 0){
		perror ("netlink recv");
		free(nlh);
		return -1;
	}

	strncpy(data, (char *)NLMSG_DATA(nlh), MSG_LEN);
	free(nlh);
	return 1;
}

Request *create_req_for_kernel(char *action_id, char *arg_msg){
	int pos,s,len;
	int num; // there is a arg 'len' in the string, we have to kick it off
	char str[RULE_LEN];
	memset(str,0,sizeof(str));

	//joint the string to:
	//action_id;key;value;key;value;....;
	strncpy(str, action_id, strlen(action_id));
	strcat(str,";");

	//create parameters for kernel
	// I've remove the paremeters
	/*
	num = 0;
	//TODO: edge check
	len = strlen(arg_msg);
	for (pos=0, s=0;pos<len;pos++){
		if ( pos && arg_msg[pos]==' ' && arg_msg[pos-1]!='\\'){
			arg_msg[pos]='\0';
			if (num && s!=pos){
				strcat(str, arg_msg+s);
				strcat(str,";");
			}
			else num++;
			s=pos+1;
			// pos=+1;
		}
	}*/

	strcat(str, "\n");
	printf ("str=%s",str);
	return simaud_create_req(str);
}

int send_to_kernel(int nlfd, int cmd, int seq, int auth){
	struct sockaddr_nl dest_addr;
	struct nlmsghdr *nlh = NULL;
	char message[MSG_LEN];
	int ret;


	/* fill the dest addr */
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;
	dest_addr.nl_groups = 0;

	/* allocate space for the msg */
	nlh = (struct nlmsghdr *) malloc (NLMSG_SPACE(MSG_LEN));
	if (!nlh) {
		perror("alloc mem in send_to_kernel");
		return -1;
	}
	/* fill the sending message */
	memset(nlh, 0, NLMSG_SPACE(MSG_LEN));
	nlh->nlmsg_len = NLMSG_SPACE(MSG_LEN);
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_type = 0;

	sprintf (message, "%dkkk%dk%dkk\n", cmd, seq, auth);
	memcpy (NLMSG_DATA(nlh), message, strlen(message)+1);

	/* Send message */
	ret = sendto (nlfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (!ret){
		perror ("error sent in send_to_kernel");
		free(nlh);
		return -1;
	}

	free(nlh);
	return 0;
}
