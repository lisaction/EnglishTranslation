#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <unistd.h>
//#include "simaudkernelif.h"
#include "simauduserif.h"
#include "simaudruleinterpret.h"
#include "simaudrulefile.h"
#include "simaudrequest.h"

static int max (int a, int b){
	return a>b?a:b;
}

void on_sigexit(int iSigNum){
	void simaud_delete_socketlink();
	exit(0);
}

static int simaud_compare_rule(Request *req){
	int res=100, rv;
	FILE *fp;
	char line[RULE_LEN+1];
	Rule *r;
	if (!(fp=simaud_open_rule_file()))
		return -1;

	/* End of the file */
	if (feof(fp)){
		return -1;
	}
	/* read line */
	if ((rv=simaud_read_line(line, fp))<0){
		simaud_close_file(fp);
		return -1;
	}
	while (rv>0){
		/* rule regulator */
		r=simaud_create_rule(line);
		if (req_match_rule(req,r))
			res=r->res<res?r->res:res; //select the small one
		free(r);
		rv=simaud_read_line(line, fp);
	}

	simaud_close_file(fp);
	if (res<100) //there is matched rule
		return res;
	else return 0; //no matched rule

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

	/* what is the "recvBuff" look like? */
	req=simaud_create_req(recvBuff);

	/* compare*/
	rv = simaud_compare_rule(req);

	/* send */
	simaud_send_authres(cfd, rv);

	/* print message */
	printf("*INFO*: Request %s asks for authorization. Result:%d.\n", req->action_id, rv);
	free(req);
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
/*	nlfd = simaud_create_netlink_fd();
	FD_SET(nlfd, &rfds);
	nfds = max (nfds, nlfd);*/


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
/*			else if (FD_ISSET(nlfd, &rfds)){// msg from kernel
				simaud_netlink_handle(nlfd);
			}*/
			else {
				fprintf (stderr, "Warning: Unknown descriptor.\n");
			}
		}

	}
	return 0;
}
