#include <stdbool.h>

bool simaud_open_rule_file(FILE *fp);
void simaud_close_file(FILE *fp);
char *simaud_read_line(FILE *fp);
