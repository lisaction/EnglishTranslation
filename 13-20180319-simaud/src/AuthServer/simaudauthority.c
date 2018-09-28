#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <sys/inotify.h>
#include "simaudauthority.h"

// regulate the max length of string in struct rule.
// in ".h" either
// #define LEN_OF_UNIT_IN_RULE 50
// the rule file to be read.
#define PATH_OF_RULE "./rule"

// structures for saving the rule configurations

/* struct Simau_Rule { */
/* char action_id[LEN_OF_UNIT_IN_RULE+1]; */
/* int res; */
/* struct Para_node *head_parameter; //para list */
/* struct Simau_Rule *next; */
/* struct Simau_Rule *pre; */
/* }; */

struct Para_node {
	char key[LEN_OF_UNIT_IN_RULE+1];
	char value[LEN_OF_UNIT_IN_RULE+1];
	struct Para_node *next;
};

struct Conf{
	struct Simau_Rule *head_rule; //rule list
	struct Simau_Rule *tail_rule;
};

// static variable
static struct Conf *conf;
static bool conf_lock; // need to be initiated.
// We are not mulit-thread, why we need a lock?

// error handle
// TODO: not a good choice
static void terminate(const char *message){
	printf("%s\n", message);
	exit(EXIT_FAILURE);
}

// compare the paras in a rule and a req
static bool simaud_paras_compare(struct Para_node *a, struct Para_node *b){
	struct Para_node *p1, *p2;
	p1 = a;
	p2 = b;
	bool f;

	while (p1){
		f = false;
		while (p2){
			if ( (strcmp (p1->key, p2->key)==0) &&
					(strcmp(p1->value, p2->value)==0) ){
				f = true;
				break;
			}
			p2 = p2->next;
		}
		if (!f)
			return false;
		p2 = b;
		p1 = p1->next;
	}
	return true;
}

// authorize; read info in conf; check lock before read
int simaud_authorize(struct Simau_Rule *req){
	int r = -1;
	struct Simau_Rule *p;
	p = conf->tail_rule;
	while (p){
		if (strcmp(p->action_id, req->action_id) == 0){ //have the same action_id
			if (!p->head_parameter){ // rule regulates no parameters
				r = p->res;
				break;
			}
			else{ // there is parameters in rule
				// compare the parameter list
				if (simaud_paras_compare(p->head_parameter, req->head_parameter)){
					r = p->res;
					break;
				}
			}
		}
		p=p->pre;
	}
	printf ("action_id=%s,auth_res=%d\n",req->action_id, r);
	return r;
}

// malloc and free
static struct Para_node *para_create(void){
	struct Para_node *para = malloc (sizeof(struct Para_node));
	if (para == NULL)
		terminate("Error in para_create");
	para->next=NULL;
	return para;
}

static int get_colon_pos(int num, char *str){
	int i, n, pos, len;
	len = strlen(str);
	for (i=0,n=0;n<num&&i<len;i++){
		if (str[i]==';')
			n++;
		pos=i+1;
	}
	return pos;
}

static struct Simau_Rule *rule_create_from_str(char *str){
	char action_id[LEN_OF_UNIT_IN_RULE+1];
	char description[LEN_OF_UNIT_IN_RULE*2+1];
	char key[LEN_OF_UNIT_IN_RULE+1];
	char value[LEN_OF_UNIT_IN_RULE+1];
	int res,id;
	struct Simau_Rule *rule;
	int len, pos;

	len = strlen(str);
	// TODO: check for the edge
	if(sscanf(str, "%d;%[^;\n];%[^;\n];%d;",&id, action_id, description, &res) != 4){
		return NULL;		
	}

	// I have to know the length
	pos = get_colon_pos(4,str);
	// create a new rule
	rule = malloc (sizeof(struct Simau_Rule));
	if (rule == NULL){
		perror("malloc for rule");
		return NULL;
	}
	rule->head_parameter = NULL;
	rule->pre = NULL;
	rule->next= NULL;

	strncpy(rule->action_id, action_id, sizeof(rule->action_id));
	strncpy(rule->des, description, sizeof(rule->des));
	rule->res = res;
	rule->id = id;

	//TODO: if the variable is shorter than reading string?
	while (sscanf(str+pos,"%[^;\n];%[^;\n];",key, value) == 2){
		struct Para_node *para = malloc (sizeof(struct Para_node));
		if (para == NULL) {
			perror("Error in para_create");
			goto fail;
		}
		strncpy(para->key, key, sizeof(para->key));
		strncpy(para->value, value, sizeof(para->value));

		para->next = rule->head_parameter;
		rule->head_parameter = para;

		pos = pos + get_colon_pos(2, str+pos);
	}
	return rule;
fail:
	free(rule);
	return NULL;
}

struct Simau_Rule *req_create(char *str){
	// We can't create a rule from here
	// rule str is different to req str.
	char key[LEN_OF_UNIT_IN_RULE];
	char value[LEN_OF_UNIT_IN_RULE];
	int offset;
	struct Simau_Rule *rule = malloc (sizeof(struct Simau_Rule));
	if (rule == NULL){
		perror("malloc error");
		return NULL;
	}
	rule->head_parameter = NULL;
	rule->next = NULL;
	rule->res = -1;

	// TODO: check for the edge
	if (sscanf(str, "%[^;\n];",rule->action_id) != 1){
		printf("Matching Error when read action_id from string\n");
		goto fail;
	}

	offset = strlen (rule->action_id) + 1;

	while (sscanf(str + offset, "%[^;\n];%[^;\n];",key, value) == 2){
		struct Para_node *para = malloc (sizeof(struct Para_node));
		if (para == NULL){
			perror("para_create from rule string");
			goto fail;
		}
		strncpy(para->key, key, sizeof(para->key));
		strncpy(para->value, value, sizeof(para->value));

		para->next = rule->head_parameter;
		rule->head_parameter = para;

		offset += strlen (para->key) + 1;
		offset += strlen (para->value) + 1;
	}
	return rule;
fail:
	free(rule);
	return NULL;
}

static struct Conf *conf_create(void){
	struct Conf *conf = malloc (sizeof(struct Conf));
	if (conf == NULL)
		terminate("Error in conf_create");
	conf->head_rule = conf->tail_rule = NULL;
	return conf;
}

static void rule_destroy(struct Simau_Rule *r){
	struct Para_node *p, *t;
	p = r->head_parameter;
	while (p){
		t = p->next;
		free(p);
		p = t;
	}
	free(r);
}

static void conf_destroy(struct Conf *cf){
	// rediculous function ...
	// how can i make this .... 2333
	// i'll correct it later! 
	// later is not far from now !!
	if (cf){
		if (cf->head_rule){
			rule_destroy(cf->head_rule);
		}
		free(cf);
	}
}

void add_new_rule_by_cmd(char *msg){
	// msg should be 1;cas.iie.pam.login;pam-rule;0;\n
	// TODO: with no para!! and exclusive id!
	struct Simau_Rule *r, *p;
	FILE *fp;

	r = rule_create_from_str(msg);
	p = conf->tail_rule;
	if (p == NULL) {// empty conf
		conf->head_rule = r;
		conf->tail_rule = r;
	}
	else {
		p->next = r;
		r->pre = p;
		conf->tail_rule = r;
	}
	// print the conf to file
	fp = fopen(PATH_OF_RULE, "w");
	if (fp == NULL)
		terminate("Error in add_new_rule_by_cmd when fopen()");
	simaud_print_conf(fp);
	fclose (fp);
}

void delete_rule(int ruleid){
	struct Simau_Rule *p = conf->head_rule;
	FILE *fp;
	while (p){
		if (p->id == ruleid){
			if (p->pre) // p is not head_rule
				p->pre->next = p->next;
			if (p->next)
				p->next->pre = p->pre;
			if (p == conf->head_rule)
				conf->head_rule = p->next;
			if (p == conf->tail_rule)
				conf->tail_rule = p->pre;
			rule_destroy(p);
			break;
		}
		p = p->next;
	}	

	// print the conf to file
	fp = fopen(PATH_OF_RULE, "w");
	if (fp == NULL)
		terminate("Error in delete_rule when fopen()");
	simaud_print_conf(fp);
	fclose (fp);

}


//modify conf and write them to file
void modify_rule_and_rewrite(char *msg){
	// msg should be 1;cas.iie.pam.login;pam-rule;0;uid=1000:user=lin:\n
	int len = strlen(msg);
	struct Simau_Rule *r, *p;
	FILE *fp;
	int fid = 0;

	for (int i=0;i<len;i++){
		if (msg[i] == '=' || msg[i]== ':')
			msg[i]=';';
		if (msg[i] == '\n')
			msg[i] = '\0';
	}
	// printf("modify_rule msg:%s\n", msg);
	// create the rule struct
	r = rule_create_from_str(msg);

	// list conf to find this
	p = conf->head_rule;
	while (p) {
		simaud_print_rule(p, stdout);
		simaud_print_rule(r, stdout);
		if (p->id == r->id){ //have the same action_id
			fid = 1;
			r->next = p->next;
			if (p->pre)
				p->pre->next = r;

			if (p == conf->tail_rule)
				conf->tail_rule = r;
			if (p == conf->head_rule)
				conf->head_rule = r;
			free(p);
			break;
		}
		p=p->next;
	}
	// we don't find the same id ??
	if (!fid)
		return;
	// print the conf to file
	fp = fopen(PATH_OF_RULE, "w");
	if (fp == NULL)
		terminate("Error in modify_rule_and_rewrite when fopen()");
	simaud_print_conf(fp);
	fclose (fp);
}

// read file
static bool simaud_read_rule(FILE *fp, struct Conf *conf_tmp){
	char action_id[LEN_OF_UNIT_IN_RULE+1];
	char description[LEN_OF_UNIT_IN_RULE*2+1];
	char key[LEN_OF_UNIT_IN_RULE+1];
	char value[LEN_OF_UNIT_IN_RULE+1];
	int res,id;
	struct Simau_Rule *rule;

	// TODO: check for the edge
	if(fscanf(fp, "%d;%[^;\n];%[^;\n];%d;",&id, action_id, description, &res) != 4){
		if (feof(fp))
			return true;
		// TODO: if fail, we can use the old one?
		// TODO: if fail, skip this line.
		if (!feof(fp) && !ferror(fp))
			printf("Matching Error when read action_id\n");
		if (ferror(fp))
			printf("Reading Error when read action_id\n");
		return false;
	}

	// create a new rule
	rule = malloc (sizeof(struct Simau_Rule));
	if (rule == NULL){
		perror("malloc for rule");
		return false;
	}
	rule->head_parameter = NULL;
	rule->pre = NULL;
	rule->next= NULL;

	rule->next = conf_tmp->head_rule;
	conf_tmp->head_rule = rule;

	strncpy(rule->action_id, action_id, sizeof(rule->action_id));
	strncpy(rule->des, description, sizeof(rule->des));
	rule->res = res;
	rule->id = id;

	//TODO: if there no more para??
	//TODO: if the variable is shorter than reading string?
	while (fscanf(fp,"%[^;\n];%[^;\n];",key, value) == 2){
		struct Para_node *para = malloc (sizeof(struct Para_node));
		if (para == NULL) {
			perror("Error in para_create");
			return false;
		}
		strncpy(para->key, key, sizeof(para->key));
		strncpy(para->value, value, sizeof(para->value));

		para->next = rule->head_parameter;
		rule->head_parameter = para;
	}
	getc(fp);
	return true;
}

// add the reverse link
static bool add_reverse_link(struct Conf *cf){
	struct Simau_Rule *p = cf->head_rule;
	p->pre = NULL;
	while (p->next){
		p->next->pre = p;
		p=p->next;
	}
	cf->tail_rule = p;
	return true;
}

// print a rule to file
void simaud_print_rule(struct Simau_Rule *p, FILE *f){
	fprintf(f,"%d;%s;%s;%d;",p->id, p->action_id, p->des, p->res);
	if (p->head_parameter){
		struct Para_node *t = p->head_parameter;
		while (t){
			fprintf(f,"%s;%s;",t->key,t->value);
			t=t->next;
		}
	}
	putc(10,f);
}

// print conf to file
void simaud_print_conf(FILE *f){
	struct Simau_Rule *p = conf->head_rule;
	while (p){
		simaud_print_rule(p,f);
		p=p->next;
	}
}

// print a rule to str
static int simaud_print_rule_tostr(struct Simau_Rule *p, char *s){
	int len = 0, r;
	r = sprintf(s+len,"%d;%s;%s;%d;",p->id, p->action_id, p->des, p->res);
	len = len + r;
	if (p->head_parameter){
		struct Para_node *t = p->head_parameter;
		while (t){
			r = sprintf(s+len,"%s;%s;",t->key,t->value);
			len = len + r;
			t=t->next;
		}
	}
	r = sprintf(s+len,"\n");
	len = len + r;
	return len;
}

// print conf to str
void simaud_print_conf_tostr(char *s){
	struct Simau_Rule *p = conf->head_rule;
	int len = 0, r;
	while (p){
		r = simaud_print_rule_tostr(p,s+len);
		len = len + r;
		p=p->next;
	}
}

// set struct conf
bool simaud_set_conf(){
	FILE *fp;
	struct Conf *cf;

	fp = fopen(PATH_OF_RULE, "r");
	if (fp == NULL){
		terminate("Error in simaud_read_conf when fopen()");
	}

	cf = conf_create();
	while (!feof(fp)){
		if (!simaud_read_rule(fp, cf)){
			conf_destroy(cf);
			goto close;
		}
	}
	add_reverse_link(cf);

	//lock
	conf_lock = true;
	if (conf != NULL)
		conf_destroy(conf);

	conf = cf;
	//unlock
	conf_lock = false;

close:
	fclose(fp);
	return true;
}

// watch the dir
int simaud_create_inotify_fd(){
	int inotify_fd, wd;

	inotify_fd = inotify_init();
	if (inotify_fd == -1){
		perror ("inotify_init");
		return -1;
	}

	wd = inotify_add_watch(inotify_fd, PATH_OF_RULE, IN_CLOSE_WRITE);
	if (wd == -1){
		perror("inotify_add_watch");
		return -1;
	}
	return inotify_fd;

}
