#ifndef AUTH_SOCKET
#define AUTH_SOCKET

#define SOCK_PATH "/tmp/simau-socket"

int simaud_create_auth_socket();
bool simaud_accept_and_send(int skfd);

#endif
