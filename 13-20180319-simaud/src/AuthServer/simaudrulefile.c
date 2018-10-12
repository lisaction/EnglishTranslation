#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "simaudrulefile.h"
#include "simaudruleinterpret.h"

/* FILE *fp; */

static const char *get_rule_path(){
	/* LOW: improve the function */
	const char *path = "/home/lin/git/simau-repository/13-20180319-simaud/src/AuthServer/rule";
	return path;
}

/* Open file to check rules when there is a request */
FILE *simaud_open_rule_file(){
	FILE *fp;
	fp = fopen(get_rule_path(), "r");
	if (fp == NULL)
		fprintf (stderr, "Error: %s\n", strerror(errno));
	return fp;
}

/* Open file to write */
FILE *simaud_open_rule_for_write(){
	FILE *fp;
	fp = fopen(get_rule_path(), "w");
	if (fp == NULL)
		fprintf (stderr, "Error: %s\n", strerror(errno));
	return fp;
}

/* Write a line to file. */
int simaud_write_line(char *line, FILE *fp){

	/* End of the file */
	if (feof(fp)){
		return 0;
	}

	/* clear error before read */
	clearerr(fp);
	/* Rule regulator will verify the rule for us */
	fputs(line, fp);
	/* error when reading */
	if (ferror(fp)){
		fprintf (stderr, "Error: Write rule to file.\n");
		return -1;
	}
	return 1;
}

/* Read a line from rule file. Return:
 * 1 - read a line from file successfully
 * 0 - end of file
 * -1 - reading error */
int simaud_read_line(char *line, FILE *fp){

	/* End of the file */
	if (feof(fp)){
		return 0;
	}

	/* clear error before read */
	clearerr(fp);
	/* Rule regulator will verify the rule for us */
	fgets(line, RULE_LEN, fp);
	/* error when reading */
	if (ferror(fp)){
		fprintf (stderr, "Error: Read rule from file.\n");
		return -1;
	}

	return 1;
}

void simaud_close_file(FILE *fp){
	fclose(fp);
}
