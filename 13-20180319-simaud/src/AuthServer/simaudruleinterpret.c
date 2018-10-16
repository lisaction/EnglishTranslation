#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "simaudruleinterpret.h"

/* Rule */

/* 0 - no colon or other errors.
 * 1 ~ - the position of num-th colon in str.*/
int get_colon_pos(int num, char *str){
	int i, n, pos=0, len;
	len = strlen(str);
	for (i=0,n=0;n<num&&i<len;i++){
		if (str[i]==';')
			n++;
		pos=i+1;
	}

	if (n==0){
		/* no colon */
		return 0;
	}
	return pos;
}

/* Create a rule from a string.
 * str:id;name;des;res;p1_n;p1_v;p2_n;p2_v;...;\n */
Rule *simaud_create_rule(char *str){
	int pos;
	int len=strlen(str)-1; //-1 is for the \n at the last

	Parameter *t, *para;

	Rule *r = calloc(1,sizeof(Rule));
	if (r == NULL){
		fprintf (stderr, "Error: Allocate space for rule.\n");
		return NULL;
	}

	if(sscanf(str, "%d;%[^;\n];%[^;\n];%d;",&r->id, r->action_id, r->des, &r->res) != 4){
		/* fail */
		// fprintf (stderr, "Error: Parse string when create rule.\n");
		return NULL;
	}

	/* get the colon pos before para */
	pos = get_colon_pos(4, str);
	assert(pos != 0);
	if (pos== len ){
		/* no parameters */
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
	simaud_delete_rule(r);
	return NULL;
}

void simaud_delete_rule(Rule *r){

	if (r->head == NULL) // no para
		free(r);
	else if (r->head->next == NULL){ //one para
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
