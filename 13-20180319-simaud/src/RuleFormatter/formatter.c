#include <stdlib.h>
#include <string.h>

#include "formatter.h"
#include "src/AuthServer/simaudruleinterpret.h"
#include "src/AuthServer/simaudrulefile.h"

#define MAX_RULE_NUM 100

/* a is a sorted array; n is the element number in a[] */
/* 1 - insert
 * -1 - find */
int find_or_insert(int n, int a[], int v){
	int i=0, j=n-1;
	int mid = (i+j)/2;
	int fc;

	if (n == 0){
		a[0] = v;
		return 1;
	}

	while (i <= j){
		fc = mid;
		if (a[mid] == v) { // find the value
			return -1;
		}
		else if (a[mid] > v){
			j = mid;
		}
		else if (a[mid] < v){
			i = mid;
		}
		mid = (i+j)/2;
		// a[fc] has been compared.
		if (mid == fc ){ // can't find
			break;
		}
	}
	// didn't find the value
	i = n;
	if (a[mid] < v) {// insert after mid
		while (i > mid){
			a[i+1]=a[i];
			i--;
		}//i==mid
	}
	else if (a[mid] > v){// insert before mid
		while (i >= mid){
			a[i+1]=a[i];
			i--;
		}//i==mid-1
	}
	a[i+1] = v;
	return 1;
}

/* >0 rid of rule
 * <0 error */
int formatter_is_validstr(char *str){
	int rid;
	Rule *r = NULL;
	r = simaud_create_rule(str);
	if (!r)
		return -1;
	if (r->id == 0) // abandoned rule
		return -1;
	if (strlen(r->action_id) == LEN_OF_UNIT)
		fprintf(stderr, "Warning: Rule %d action_id may exceed.\n", r->id);
	if (strlen(r->des) == LEN_OF_DES)
		fprintf(stderr, "Warning: Rule %d action_id may exceed.\n", r->id);
	if (r->res != 0 && r->res != 1){
		fprintf(stderr, "Error: Rule %d has unknown result.\n", r->id);
		return -1;
	}
	rid = r->id;
	simaud_delete_rule(r);
	return rid;
}

void formatter_write_2newfile(){
	char line[RULE_LEN];
	FILE *fp, *fpt;
	int rv, rnew, n, i;
	int rid[MAX_RULE_NUM+1];

	fpt = simaud_open_tmprule_for_write();
	fp = simaud_open_rule_for_write();

	memset(line, 0, sizeof(line));

	n = 0;
	rv = simaud_read_line(line, fp);
	while (rv>0){
		rnew = formatter_is_validstr(line);
		if (rnew > 0) { //valid rule
			// check if there is another rule with same id
			if (find_or_insert(n, rid, rnew) > 0){
				// valid rule and save its rid
				simaud_write_line(line, fpt);
				n++;
			}
		}
		rv = simaud_read_line(line, fp);
	}

	simaud_close_file(fp);
	simaud_close_file(fpt);
	simaud_remove_file();
	simaud_rename_tmpfile();

	printf("%d rules has been recorded.\n", n);
}

