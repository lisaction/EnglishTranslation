
#ifndef SIMAUDAUTHIF_H_
#define SIMAUDAUTHIF_H_

#include "simaudrequest.h"
#include <stdbool.h>

int simaud_compare_rule(Request *req);
int simaud_authorize_mqclient(char *buff);
bool simaud_compare_user(Rule *a, Rule *b);

#endif /* SIMAUDAUTHIF_H_ */
