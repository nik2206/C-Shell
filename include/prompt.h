#ifndef PROMPT_H
#define PROMPT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define SHELL_MAX_PATH 4096

void display_prompt(void);

void initialize_shell(void);

#endif
