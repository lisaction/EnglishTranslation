
#ifndef AUTHORITY_H
#define AUTHORITY_H

// regulate the max length of string in struct rule.
#define LEN_OF_UNIT_IN_RULE 50
// the rule file to be read.
// #define PATH_OF_RULE "./rule"

#include <stdbool.h>
#include <stdio.h>

// struct Simau_Rule;
struct Simau_Rule {
	int id;
	char action_id[LEN_OF_UNIT_IN_RULE+1];
	int res;
	char des[LEN_OF_UNIT_IN_RULE*2+1];
	struct Para_node *head_parameter; //para list
	struct Simau_Rule *next;
	struct Simau_Rule *pre;
};

int simaud_authorize(struct Simau_Rule *req);
struct Simau_Rule *req_create(char *str);
bool simaud_set_conf();
void simaud_print_conf(FILE *f);
void simaud_print_rule(struct Simau_Rule *p, FILE *f);
int simaud_create_inotify_fd();
void simaud_print_conf_tostr(char *s);
void modify_rule_and_rewrite(char *msg);
void add_new_rule_by_cmd(char *msg);
void delete_rule(int ruleid);
// int simaud_print_rule_tostr(struct Simau_Rule *p, char *s);

#endif
