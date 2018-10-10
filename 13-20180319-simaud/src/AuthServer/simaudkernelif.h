#ifndef NETLINK_H
#define NETLINK_H

#include "simaudrequest.h"

int simaud_create_netlink_fd();
int simaud_recvfrom_kernel(int nlfd, char *data);
int send_to_kernel(int nlfd, int cmd, int seq, int auth);
/* Divide this function */
Request *create_req_for_kernel(char *action_id, char *arg_msg);

#endif	
