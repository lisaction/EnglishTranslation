#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>

#include <sys/types.h>
#include <unistd.h>
#include "simauduserif.h"
#include "simaudkernelif.h"
#include "simaudruleinterpret.h"
#include "simaudauthif.h"

static int max (int a, int b){
	return a>b?a:b;
}

static void on_sigexit(int iSigNum){
	void simaud_delete_socketlink();
	exit(0);
}

static void simaud_authorize_userspace(int fd){
	char recvBuff[RULE_LEN+1];
	int cfd, rv;
	Request *req;

	//accept and build connection
	if ((cfd=simaud_accept_socket(fd))<0)
		return;

	//read socket
	if ((rv=simaud_read_socket(cfd, recvBuff))<0){
		close(cfd);
		return;
	}

	/* the "recvBuff" look like: action_id;p1;v1;p2;v2; */
	req = simaud_create_req(recvBuff);

	/* compare*/
	rv = simaud_compare_rule(req);

	/* send */
	simaud_send_authres(cfd, rv);

	/* print message */
	printf("*INFO*: Request %s asks for authorization. Result:%d.\n", req->action_id, rv);
	free(req);
}

static void simaud_authorize_kernel(int nlfd){
	char data[RULE_LEN];
	int len, num, seq, r;
	char action_id[LEN_OF_UNIT];

	struct Request *req = NULL;
	if (simaud_recvfrom_kernel(nlfd, data) > 0){
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
		num = simaud_compare_rule(req);
		/* print message */
		printf("*INFO*: Request %s asks for authorization. Result:%d.\n", req->action_id, num);
		free(req);
	}
	if (num == 1)
		send_to_kernel(nlfd,2,seq,1);
	else send_to_kernel(nlfd,2,seq,0);
	return;
}


int main (int argc, char **argv){
	int retval;
	int skfd, nlfd;
	int nfds = 0;
	fd_set rfds, cpy_fds;

	/* opt */
	if (argc > 1){
		if (strcmp(argv[1],"--no-debug")==0){
			/* printf("test here.\n"); */
			freopen("/dev/null","w", stdout);
			freopen("/dev/null","w", stdin);
			freopen("/dev/null","w", stderr);
		}
	}

	/* install signal handler */
	signal(SIGTERM, on_sigexit);
	signal(SIGINT, on_sigexit);

	/* clear set and add new fd */
	FD_ZERO(&rfds);
	// user-space socket
	skfd = simaud_create_auth_socket();
	FD_SET(skfd, &rfds);
	nfds = max (nfds, skfd);

	// netlink fd
	nlfd = simaud_create_netlink_fd();
	FD_SET(nlfd, &rfds);
	nfds = max (nfds, nlfd);

	// save initial state
	cpy_fds = rfds;
	while (1){
		rfds = cpy_fds;
		retval = select(nfds+1, &rfds, NULL, NULL, NULL);
		if (retval == -1)
			die_on_error("select error");
		else if (retval){
			if (FD_ISSET(skfd, &rfds)){	//skfd is ready
				simaud_authorize_userspace(skfd);
			}
			else if (FD_ISSET(nlfd, &rfds)){// msg from kernel
				simaud_authorize_kernel(nlfd);
			}
			else {
				fprintf (stderr, "Warning: Unknown descriptor.\n");
			}
		}

	}
	return 0;
}
