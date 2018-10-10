#include "simauduserif.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
// AF_UNIX
#include <sys/un.h>

#include "simaudrequest.h"
#include "simaudruleinterpret.h"

#define SOCK_PATH "/tmp/simaud-socket"
#define SIMAU_MAX_LINK 10

/* SOCKET */

void die_on_error(char const *context){
	perror(context);
	exit(1);
}

int simaud_create_auth_socket(){
	int skfd;
	struct sockaddr_un server_addr;

	// create skfd
	if( (skfd=socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		die_on_error("create socket");
	}

	//bind address
	memset(&server_addr, 0, sizeof(struct sockaddr_un)); /*Clear struct*/
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path)-1);
	if (bind(skfd,(struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1){
		die_on_error("bind");
	}

	//listen
	if (listen(skfd, SIMAU_MAX_LINK) == -1){
		die_on_error("listen");
	}

	return skfd;
}

/* return
 * >0: successful, cfd
 * <0 : error */
int simaud_accept_socket(int skfd){
	int cfd;
	struct sockaddr_un pep_addr;
	socklen_t pep_addr_size;

	/* accept and build connection */
	pep_addr_size = sizeof(struct sockaddr_un);
	cfd = accept (skfd, (struct sockaddr *)&pep_addr, &pep_addr_size);
	if (cfd == -1){
		perror("accept");
		return -1;
	}
	return cfd;
}

/* >0 : the number of bytes received.
 * -1 : fail. */
int simaud_read_socket(int cfd, char *recvBuff){
	int n;
	/* read message */
	/* while, sizeof(recvBuff)=8 */
	if ( (n=recv(cfd, recvBuff, RULE_LEN, 0)) > 0){
		recvBuff[n] = '\0';
		return n;
	}
	perror ("read socket");
	return -1;
}

/* Send the result.
 * Nothing is return because we don't handle it
 * even an error occurs. */
void simaud_send_authres(int cfd, int res){
	const char auth[] = "yes", unauth[] = "no";
	int n;

	/* send */
	if (res == 1)
		n = send(cfd, auth, strlen(auth), 0);
	else n = send(cfd, unauth, strlen(unauth), 0);

	/* n>0: the number of bytes sent
	 * -1: error */
	if (n<0)
		perror("send authres");
}

void simaud_close_connection(int cfd){
	if (close(cfd) < 0)
		perror("close connection");
}

void simaud_delete_socketlink(){
	unlink(SOCK_PATH);
}
