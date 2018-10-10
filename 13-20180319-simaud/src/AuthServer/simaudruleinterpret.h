#ifndef RULEINTERPRET_H
#define RULEINTERPRET_H

/* MACRO */
#define LEN_OF_UNIT 50
#define RULE_LEN LEN_OF_UNIT*12

typedef struct Parameter {
	char key[LEN_OF_UNIT+1];
	char value[LEN_OF_UNIT+1];
	struct Parameter *next;
} Parameter;

typedef struct Rule {
	int id;
	char action_id[LEN_OF_UNIT+1];
	int res;
	char des[LEN_OF_UNIT*2+1];
	Parameter *head; //para list
} Rule;

/* Utilities */
/* return the position of num-th colon in str. */
int get_colon_pos(int num, char *str);

/* RULE */
Rule *simaud_create_rule(char *str);
void simaud_delete_rule(Rule *r);

#endif
