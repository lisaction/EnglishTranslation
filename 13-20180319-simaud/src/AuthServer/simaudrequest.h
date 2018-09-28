#ifndef REQUEST_H
#define REQUEST_H

#include <stdbool.h>
#include "simaudruleinterpret.h"

typedef struct Request{
	char action_id[LEN_OF_UNIT+1];
	Parameter *head; // para list
} Request;

/* REQ */
Request *simaud_create_req(char *str);
void simaud_delete_req(Request *r);
/* check whether the rule is to the req */
bool req_match_rule(Request *req, Rule *rule);

#endif
