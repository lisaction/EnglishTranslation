#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "simaudkernelif.h"
#include "simauduserif.h"

#include "simaudauthority.h"


int nfds = 0;

int max (int a, int b){
	return a>b?a:b;
}

void on_sigint(int iSigNum){
	for (int i=0;i<nfds;i++)
		close(i);
	//remove(SOCK_PATH);
	exit(1);
}

int main (int argc, char **argv){
	// initial test
	/* puts ("Hello World!"); */
	/* puts ("This is " PACKAGE_STRING "."); */

	/* opt */
	if (argc > 1){
		if (strcmp(argv[1],"--no-debug")==0){
			/* printf("test here.\n"); */
			freopen("/dev/null","w", stdout);
			freopen("/dev/null","w", stdin);
			freopen("/dev/null","w", stderr);
		}
	}

	// read rule test
	if (simaud_set_conf())
		simaud_print_conf(stdout);

	/* struct Simau_Rule *r = rule_create(); */
	/* strcpy (r->action_id, "cas.iie.pam.login"); */

	/* printf("%d\n", simaud_authorize(r)); */

	// sigint handle
	signal(SIGINT, on_sigint);

	// select test
	int retval;
	int skfd, inofd, nlfd, mqfd;
	char buff[4096];
	fd_set rfds, cpy_fds;
	int len;

	/* clear set and add new fd */
	FD_ZERO(&rfds);
	// listen socket
	skfd = simaud_create_auth_socket();
	FD_SET(skfd, &rfds);
	nfds = max (nfds, skfd);
	// inotify fd
	inofd = simaud_create_inotify_fd();
	FD_SET(inofd, &rfds);
	nfds = max (nfds, inofd);
	// netlink fd
	nlfd = simaud_create_netlink_fd();
	FD_SET(nlfd, &rfds);
	nfds = max (nfds, nlfd);
	// amqp fd
	// disable the mq
	/* conn = create_mqconn(); */
	/* mqfd = amqp_get_sockfd(conn); */
	mqfd = 5;
	FD_SET (mqfd, &rfds);
	nfds = max (nfds, mqfd);

	// the init state
	cpy_fds = rfds;
	while (1){
		rfds = cpy_fds;
		retval = select(nfds+1, &rfds, NULL, NULL, NULL);
		if (retval == -1)
			perror("select error");
		else if (retval){
			if (FD_ISSET(skfd, &rfds)){	//skfd is ready
				simaud_accept_socket(skfd);
			}
			else if (FD_ISSET(inofd, &rfds)){ // file is change
				len = read(inofd, buff, sizeof(buff));
				if (len == -1){
					perror("read rule file");
					break;
				}
				printf("len=%d\n",len);

				if (simaud_set_conf())
					simaud_print_conf(stdout);
				
				// send to controller

				// Should i do this ????
				close (inofd);
				inofd = simaud_create_inotify_fd();
			}
			else if (FD_ISSET(nlfd, &rfds)){	// msg from kernel
				simaud_netlink_handle(nlfd);
			}
			else {
				printf("Unkonw descriptor.\n");
			}
		}

	}

	return 0;
}
