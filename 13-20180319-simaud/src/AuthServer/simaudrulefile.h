#ifndef RULEFILE_H
#define RULEFILE_H

#include <stdio.h>

const char *get_rule_path();

FILE *simaud_open_rule_file();
FILE *simaud_open_rule_for_write();
FILE *simaud_open_tmprule_for_write();

int simaud_read_line(char *line, FILE *fp);
int simaud_write_line(char *line, FILE *fp);

void simaud_close_file(FILE *fp);
void simaud_remove_file();
void simaud_rename_tmpfile();

#endif
