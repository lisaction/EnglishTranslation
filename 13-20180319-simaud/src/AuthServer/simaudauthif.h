
#ifndef SIMAUDAUTHIF_H_
#define SIMAUDAUTHIF_H_

#include "simaudrequest.h"

int simaud_compare_rule(Request *req);
int simaud_authorize_mqclient(char *buff);

#endif /* SIMAUDAUTHIF_H_ */
