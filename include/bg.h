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

// Current foreground process (-1 if none)
extern pid_t fg_pid;
extern char fg_cmd_name[BG_CMD_LEN];

// Add a background job and print [job_number] pid
int add_bg_job(pid_t pid, const char *cmd_name);

// Add a stopped foreground process to the background list
int add_bg_job_stopped(pid_t pid, const char *cmd_name);

// Check for completed background jobs and print exit status
void check_bg_jobs(void);

// Kill all active background jobs (for Ctrl-D exit)
void kill_all_bg_jobs(void);

// List all running/stopped background processes
int execute_activities(void);

// Send a signal to a process
int execute_ping(char **args, int arg_count);

// Bring a background/stopped job to the foreground
int execute_fg(char **args, int arg_count);

// Resume a stopped background job
int execute_bg_cmd(char **args, int arg_count);

#endif
