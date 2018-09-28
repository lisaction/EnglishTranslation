#ifndef RULEINTERPRET_H
#define RULEINTERPRET_H

#define LEN_OF_UNIT 50

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

Rule *simaud_create_rule(char *str);
void simaud_delete_rule(Rule *r);

#endif
