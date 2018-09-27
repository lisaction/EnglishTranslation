#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <linux/netlink.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "simaudauthority.h"

// netlink family defined by myself in kernel (polkit.h)
#define NETLINK_POLKITD 23
// local id
#define USER_PORT 12517
// MSG_LEN from kernel
#define MSG_LEN 700

int simaud_create_netlink_fd (){
	int skfd;
	/* address */
	struct sockaddr_nl local;
	/* temp return */
	int ret;

	/* Create socket */
	skfd = socket (AF_NETLINK, SOCK_RAW, NETLINK_POLKITD);
	if (skfd == -1)	{
		perror("netlink socket");
		return -1;
	}

	/* fill the local address */
	memset (&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pid = USER_PORT;
	local.nl_groups = 0;

	/* bind the socket with the local address */
	ret = bind (skfd, (struct sockaddr *)&local, sizeof(local));
	if (ret == -1 )	{
		perror("netlink bind");
		close(skfd);
		return -1;
	}
	return skfd;
}

static int recv_from_kernel (int nlfd, char *data){
	int ret;
	struct nlmsghdr *nlh = NULL;

	/* allocate space for the msg */
	nlh = (struct nlmsghdr *) malloc (NLMSG_SPACE(MSG_LEN));
	if (!nlh){
		perror("malloc for nlh");
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
	/* printf ("Recv msg: %s, p=%p\n", data, nlh); */
	free(nlh);

	/* if (num%2 ==0 && len < MSG_LEN) */
	/* return auth_details_to_gvariant(num,data+len); */
	return 1;
}

static struct Simau_Rule *create_req_for_kernel(char *action_id, char *arg_msg){
	int pos,s,len;
	int num; // there is a arg 'len' in the string, we have to kick it off
	char str[LEN_OF_UNIT_IN_RULE*15];
	memset(str,0,sizeof(str));

	//joint the string to:
	//action_id;key;value;key;value;....;
	strncpy(str, action_id, sizeof(str));
	strcat(str,";");

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
			/* pos=+1; */
		}
	}

	printf ("str=%s\n",str);
	return req_create(str);
}

static int send_to_kernel(int skfd, int cmd, int seq, int auth){
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
	ret = sendto (skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (!ret){
		perror ("error sent in send_to_kernel");
		free(nlh);
		return -1;
	}

	free(nlh);
	return 0;
}


void simaud_netlink_handle(int nlfd){
	char data[MSG_LEN];
	int len, num, seq, r;
	char action_id[LEN_OF_UNIT_IN_RULE];

	struct Simau_Rule *req = NULL;
	if (recv_from_kernel(nlfd, data) > 0){
		printf ("data is %s\n", data);
		// seq is the id of this req
		// num is the number of arg, for compatibility, reserve
		// len is the offset of arg
		sscanf(data, "%d %s %d %d\n",&seq, action_id, &num, &len);

		// operate on this string
		req = create_req_for_kernel(action_id, data+len);
	}

	num = 0;
	if (req) {
		simaud_print_rule(req,stdout);
		num = simaud_authorize(req);
	}
	if (num == 1)
		send_to_kernel(nlfd,2,seq,1);
	else send_to_kernel(nlfd,2,seq,0);
}
