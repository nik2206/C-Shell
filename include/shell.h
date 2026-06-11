#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "prompt.h"
#include "parser.h"
#include "hop.h"
#include "reveal.h"

#define SHELL_MAX_INPUT 1024
#define SHELL_MAX_PATH 4096

// Global variables to track shell state
extern char home_dir[SHELL_MAX_PATH];        
extern char current_dir[SHELL_MAX_PATH];     
extern char previous_dir[SHELL_MAX_PATH];    

void run_shell(void);

int execute_command(ParsedCommand *cmd);

#endif
