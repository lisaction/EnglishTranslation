#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
// AF_UNIX
#include <sys/un.h>

#include "simaudauthserver.h"
#include "simaudauthority.h"

/* #define SOCK_PATH "/tmp/simau-socket" */
#define SIMAU_MAX_LINK 5
#define BUFFER_SIZE LEN_OF_UNIT_IN_RULE*15


int simaud_create_auth_socket(){
	int skfd;
	struct sockaddr_un server_addr;

	// create skfd
	if( (skfd=socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		perror("socket error");
		return -1;
	}

	//bind address
	memset(&server_addr, 0, sizeof(struct sockaddr_un)); /*Clear struct*/
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path)-1);
	if (bind(skfd,(struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1){
		perror("bind error");
		return -1;
	}

	if (listen(skfd, SIMAU_MAX_LINK) == -1){
		perror("listen error");
		return -1;
	}

	return skfd;
}

bool simaud_accept_and_send(int skfd){
	int cfd, n;
	struct sockaddr_un pep_addr;
	socklen_t pep_addr_size;
	char recvBuff[BUFFER_SIZE];
	const char auth[] = "yes", unauth[] = "no";
	char sendBuff[10];

	pep_addr_size = sizeof(struct sockaddr_un);
	cfd = accept (skfd, (struct sockaddr *)&pep_addr, &pep_addr_size);
	if (cfd == -1)
		perror("accept error");
	if ( (n=recv(cfd, recvBuff,sizeof(recvBuff), 0)) > 0){
		recvBuff[n] = '\0';
		struct Simau_Rule *req = req_create(recvBuff);
		int ret;
		if (req)
			ret = simaud_authorize(req);
		else ret = 0;
		if (ret == 1)
			strncpy (sendBuff,auth, sizeof(auth));
		else strncpy (sendBuff, unauth, sizeof(unauth));

		// send
		send(cfd, sendBuff, strlen(sendBuff), 0);
	}
	else
		perror ("error receive");
	close(cfd);
	return true;
}
