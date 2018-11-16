#include <stdlib.h>

#include "simaudruleinterpret.h"
#include "simaudrulefile.h"
#include "simaudauthif.h"

int simaud_compare_rule(Request *req){
	int res=100, rv;
	FILE *fp;
	char line[RULE_LEN+1];
	Rule *r;
	if (!(fp=simaud_open_rule_file()))
		return -1;

	/* End of the file */
	if (feof(fp)){
		return -1;
	}
	/* read line */
	if ((rv=simaud_read_line(line, fp))<0){
		simaud_close_file(fp);
		return -1;
	}
	while (rv>0){
		/* rule regulator */
		r=simaud_create_rule(line);
		if (req_match_rule(req,r))
			res=r->res<res?r->res:res; //select the small one
		free(r);
		rv=simaud_read_line(line, fp);
	}

	simaud_close_file(fp);
	if (res<100) //there is matched rule
		return res;
	else return 0; //no matched rule

}

int simaud_authorize_mqclient(char *buff){
	/* the "Buff" look like: action_id;p1;v1;p2;v2; */
	Request *req;
	req = simaud_create_req(buff);
	return simaud_compare_rule(req);
}
