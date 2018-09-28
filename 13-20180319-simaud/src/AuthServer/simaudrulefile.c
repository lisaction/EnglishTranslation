#include <stdio.h>
#include <stdlib.h>
#include "simaudrulefile.h"

#define MAX_RULE_LINE 500

/* FILE *fp; */

static const char *get_rule_path(){
	/* LOW: improve the function */
	const char *path = ".rule";
	return path;
}

/* Open file to check rules when there is a request */
bool simaud_open_rule_file(FILE *fp){
	const char *path = get_rule_path();
	fp = fopen(path, "r");
	if (fp == NULL){
		fprintf (stderr, "Error: Open rule file.\n");
		return false;
	}
	return true;
}

void simaud_close_file(FILE *fp){
	fclose(fp);
}

/* Read a line from rule file. Return:
 * str - a line from file
 * NULL - end of file or reading error */
char *simaud_read_line(FILE *fp){

	/* End of the file */
	if (feof(fp)){
		return NULL;
	}

	/* clear error before read */
	clearerr(fp);
	/* Rule regulator will verify the rule for us */
	char line[MAX_RULE_LINE+2];
	fgets(line, sizeof(line), fp);
	/* error when reading */
	if (ferror(fp)){
		fprintf (stderr, "Error: Read rule from file.\n");
		return NULL;
	}

	/* return local variable warning. */
	return line;
}
