#ifndef BG_H
#define BG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define MAX_BG_JOBS 64
#define BG_CMD_LEN 256

// Add a background job and print [job_number] pid
int add_bg_job(pid_t pid, const char *cmd_name);

// Check for completed background jobs and print exit status
void check_bg_jobs(void);

// List all running/stopped background processes
int execute_activities(void);

// Send a signal to a process
int execute_ping(char **args, int arg_count);

#endif
