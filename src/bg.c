#include "bg.h"

typedef struct {
    pid_t pid;
    char cmd_name[BG_CMD_LEN];
    int active;
    int stopped;     // 0 = running, 1 = stopped
    int job_number;
} BgJob;

static BgJob bg_jobs[MAX_BG_JOBS];
static int bg_job_count = 0;
static int next_job_number = 1;

// Add a background job and print [job_number] pid
int add_bg_job(pid_t pid, const char *cmd_name) {
    if (bg_job_count >= MAX_BG_JOBS) {
        return -1;
    }

    int idx = bg_job_count++;
    bg_jobs[idx].pid = pid;
    strncpy(bg_jobs[idx].cmd_name, cmd_name, BG_CMD_LEN - 1);
    bg_jobs[idx].cmd_name[BG_CMD_LEN - 1] = '\0';
    bg_jobs[idx].active = 1;
    bg_jobs[idx].stopped = 0;
    bg_jobs[idx].job_number = next_job_number++;

    printf("[%d] %d\n", bg_jobs[idx].job_number, bg_jobs[idx].pid);

    return bg_jobs[idx].job_number;
}

// Check for completed background jobs and print exit status
void check_bg_jobs(void) {
    for (int i = 0; i < bg_job_count; i++) {
        if (!bg_jobs[i].active) {
            continue;
        }

        int status;
        pid_t result = waitpid(bg_jobs[i].pid, &status,
                               WNOHANG | WUNTRACED | WCONTINUED);

        if (result > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                // Process terminated
                bg_jobs[i].active = 0;

                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    printf("%s with pid %d exited normally\n",
                           bg_jobs[i].cmd_name, bg_jobs[i].pid);
                } else {
                    printf("%s with pid %d exited abnormally\n",
                           bg_jobs[i].cmd_name, bg_jobs[i].pid);
                }
            } else if (WIFSTOPPED(status)) {
                bg_jobs[i].stopped = 1;
            } else if (WIFCONTINUED(status)) {
                bg_jobs[i].stopped = 0;
            }
        }
    }
}

// List all running/stopped background processes sorted by command name
int execute_activities(void) {
    // Collect indices of active jobs
    int active_indices[MAX_BG_JOBS];
    int active_count = 0;

    for (int i = 0; i < bg_job_count; i++) {
        if (!bg_jobs[i].active) {
            continue;
        }

        // Verify process is still alive
        if (kill(bg_jobs[i].pid, 0) < 0) {
            bg_jobs[i].active = 0;
            continue;
        }

        active_indices[active_count++] = i;
    }

    // Sort lexicographically by command name (bubble sort — small list)
    for (int a = 0; a < active_count - 1; a++) {
        for (int b = 0; b < active_count - a - 1; b++) {
            if (strcmp(bg_jobs[active_indices[b]].cmd_name,
                       bg_jobs[active_indices[b + 1]].cmd_name) > 0) {
                int tmp = active_indices[b];
                active_indices[b] = active_indices[b + 1];
                active_indices[b + 1] = tmp;
            }
        }
    }

    // Print in required format: [pid] : command_name - State
    for (int a = 0; a < active_count; a++) {
        int idx = active_indices[a];
        printf("[%d] : %s - %s\n",
               bg_jobs[idx].pid,
               bg_jobs[idx].cmd_name,
               bg_jobs[idx].stopped ? "Stopped" : "Running");
    }

    return 0;
}

// Send a signal to a process: ping <pid> <signal_number>
int execute_ping(char **args, int arg_count) {
    if (arg_count != 2) {
        printf("Invalid syntax!\n");
        return 1;
    }

    // Parse pid
    char *endptr;
    long pid_val = strtol(args[0], &endptr, 10);
    if (*endptr != '\0') {
        printf("Invalid syntax!\n");
        return 1;
    }

    // Parse signal number
    long sig_val = strtol(args[1], &endptr, 10);
    if (*endptr != '\0') {
        printf("Invalid syntax!\n");
        return 1;
    }

    int actual_signal = (int)(sig_val % 32);
    pid_t pid = (pid_t)pid_val;

    // Check if process exists
    if (kill(pid, 0) < 0) {
        printf("No such process found\n");
        return 1;
    }

    // Send signal
    if (kill(pid, actual_signal) < 0) {
        printf("No such process found\n");
        return 1;
    }

    printf("Sent signal %d to process with pid %d\n", actual_signal, (int)pid);

    // Update state in bg_jobs if the signal stops or resumes the process
    for (int i = 0; i < bg_job_count; i++) {
        if (bg_jobs[i].active && bg_jobs[i].pid == pid) {
            if (actual_signal == SIGSTOP || actual_signal == SIGTSTP) {
                bg_jobs[i].stopped = 1;
            } else if (actual_signal == SIGCONT) {
                bg_jobs[i].stopped = 0;
            }
            break;
        }
    }

    return 0;
}
