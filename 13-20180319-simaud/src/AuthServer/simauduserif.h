#ifndef AUTHIF_H
#define AUTHIF_H

/* Utility*/
void die_on_error(char const *context);

/* socket*/
int simaud_create_auth_socket();
int simaud_accept_socket(int skfd);
int simaud_read_socket(int cfd, char *recvBuff);
void simaud_send_authres(int cfd, int res);
void simaud_close_connection(int cfd);
void simaud_delete_socketlink();

#endif
