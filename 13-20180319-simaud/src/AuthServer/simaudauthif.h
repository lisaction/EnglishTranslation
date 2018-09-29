#ifndef AUTHIF_H
#define AUTHIF_H

/* socket*/
int simaud_create_auth_socket();
int simaud_accept_socket(int skfd);
char *simaud_read_socket(int cfd);
void simaud_send_authres(int cfd, int res);
void simaud_close_connection(int cfd);

#endif
