#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MAX_COMMANDS 15
#define LOG_MAX_CMD_LEN 1024

void initialize_log(void);

void add_to_log(const char *cmd_str);

int is_log_command(const char *raw_input);

int execute_log(char **args, int arg_count, char *reexec_buf);

#endif
