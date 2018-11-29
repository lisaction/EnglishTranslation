#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "simaudrequest.h"

/* Request */

/* Create a request from a string.
 * str:name(action_id);p1_n;p1_v;p2_n;p2_v;...;\n */
Request *simaud_create_req(char *str){
	int pos;
	int len = strlen(str)-1; //-1 for \n

	Parameter *t, *para;

	Request *r = calloc (1, sizeof(Request));
	if (r == NULL){
		fprintf (stderr, "Error: Allocate space for request.\n");
		return NULL;
	}

	if(sscanf(str, "%[^;\n];",r->action_id) != 1){
		/* fail */
		fprintf (stderr, "Error: Parse string when create request.\n");
		return NULL;
	}

	/* get the colon pos before para */
	pos = get_colon_pos(1, str);

	/* assert(pos != 0); */
	if (pos == 0){
		fprintf (stderr, "Warning: There is no colon in req string.\n");
		return NULL;
	}

	if (pos == len){
		/* no parameters */
		r->head = NULL;
		return r;
	}

	/* create head for parameter*/
	para = calloc (1, sizeof(Parameter));
	if (para == NULL) {
		fprintf (stderr, "Error: Allocate space for parameter.\n");
		goto fail;
	}
	r->head = para;

	/* read parameter */
	while (sscanf(str+pos,"%[^;\n];%[^;\n];",para->key, para->value) == 2){
		para->next = NULL; // successful

		/* change the colon pos to the next parameter */
		pos = pos + get_colon_pos(2, str+pos);
		assert(pos != 0);
		if (pos == len){
			/* no more parameters */
			break;
		}

		t = para;
		para = calloc (1, sizeof(Parameter));
		if (para == NULL) {
			fprintf (stderr, "Error: Allocate space for parameter.\n");
			goto fail;
		}
		t->next = para;
	}
	return r;
fail:
	simaud_delete_req(r);
	return NULL;
}

void simaud_delete_req(Request *r){

	if (r->head == NULL)
		free(r);
	else if (r->head->next == NULL){
		free(r->head);
		free(r);
	}
	else {
		Parameter *p1 = r->head;
		Parameter *p2 = p1->next;
		while (p2!=NULL){
			free(p1);
			p1=p2;
			p2=p1->next;
		}
		free(p1);
		free(r);
	}
}

bool req_match_rule(Request *req, Rule *rule){
	Parameter *p1, *p2;
	bool flag = false;

	/* compare: action id */
	if (strcmp(req->action_id, rule->action_id) != 0)
		return false;

	/* compare: para list */
	p1 = req->head;
	p2 = rule->head;

	/* no para */
	if (p1 == NULL && p2 == NULL)
		return true;

	/* one of them has para */
	if (p1 == NULL || p2 == NULL )
		return false;

	/* both have para */
	while (p1){
		while (p2){
			if (strcmp(p1->key, p2->key) == 0 ||
					strcmp (p1->value, p2->value) == 0){
				/* req para is matched */
				flag = true;
				break;
			}
			p2 = p2->next;
		}
		if (flag){
			/* this para is matched again */
			if (p1->next == NULL){
				/* this is last para in req */
				return true;
			}
			/* go to match the next */
			flag = false;
			p1 = p1->next;
			p2 = rule->head;
		}
		else {
			/* the is a para that has no matching */
			return false;
		}
	}

	/* we won't be here */
	return false;
}
