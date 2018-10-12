#ifndef RULEFILE_H
#define RULEFILE_H

#include <stdio.h>

FILE *simaud_open_rule_file();
FILE *simaud_open_rule_for_write();
void simaud_close_file(FILE *fp);
int simaud_read_line(char *line, FILE *fp);
int simaud_write_line(char *line, FILE *fp);

#endif
