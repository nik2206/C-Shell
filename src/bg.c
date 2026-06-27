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

// Globals for tracking the current foreground process
pid_t fg_pid = -1;
char fg_cmd_name[BG_CMD_LEN] = "";

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

// Add a stopped foreground process to the background list (Ctrl-Z)
int add_bg_job_stopped(pid_t pid, const char *cmd_name) {
    if (bg_job_count >= MAX_BG_JOBS) {
        return -1;
    }

    int idx = bg_job_count++;
    bg_jobs[idx].pid = pid;
    strncpy(bg_jobs[idx].cmd_name, cmd_name, BG_CMD_LEN - 1);
    bg_jobs[idx].cmd_name[BG_CMD_LEN - 1] = '\0';
    bg_jobs[idx].active = 1;
    bg_jobs[idx].stopped = 1;
    bg_jobs[idx].job_number = next_job_number++;

    printf("\n[%d] Stopped %s\n", bg_jobs[idx].job_number, bg_jobs[idx].cmd_name);

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

// Kill all active background jobs (for Ctrl-D / exit)
void kill_all_bg_jobs(void) {
    for (int i = 0; i < bg_job_count; i++) {
        if (bg_jobs[i].active) {
            kill(bg_jobs[i].pid, SIGKILL);
            bg_jobs[i].active = 0;
        }
    }
}

// List all running/stopped background processes sorted by command name
int execute_activities(void) {
    int active_indices[MAX_BG_JOBS];
    int active_count = 0;

    for (int i = 0; i < bg_job_count; i++) {
        if (!bg_jobs[i].active) {
            continue;
        }

        if (kill(bg_jobs[i].pid, 0) < 0) {
            bg_jobs[i].active = 0;
            continue;
        }

        active_indices[active_count++] = i;
    }

    // Sort lexicographically by command name
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

    char *endptr;
    long pid_val = strtol(args[0], &endptr, 10);
    if (*endptr != '\0') {
        printf("Invalid syntax!\n");
        return 1;
    }

    long sig_val = strtol(args[1], &endptr, 10);
    if (*endptr != '\0') {
        printf("Invalid syntax!\n");
        return 1;
    }

    int actual_signal = (int)(sig_val % 32);
    pid_t pid = (pid_t)pid_val;

    if (kill(pid, 0) < 0) {
        printf("No such process found\n");
        return 1;
    }

    if (kill(pid, actual_signal) < 0) {
        printf("No such process found\n");
        return 1;
    }

    printf("Sent signal %d to process with pid %d\n", actual_signal, (int)pid);

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

// Helper: find a job by job_number, or most recent active job if job_num == -1
static int find_bg_job(int job_num) {
    if (job_num == -1) {
        // Find the most recently created active job
        for (int i = bg_job_count - 1; i >= 0; i--) {
            if (bg_jobs[i].active) {
                return i;
            }
        }
        return -1;
    }

    for (int i = 0; i < bg_job_count; i++) {
        if (bg_jobs[i].active && bg_jobs[i].job_number == job_num) {
            return i;
        }
    }
    return -1;
}

// Bring a background/stopped job to the foreground: fg [job_number]
int execute_fg(char **args, int arg_count) {
    int job_num = -1; // default: most recent

    if (arg_count > 0) {
        char *endptr;
        long val = strtol(args[0], &endptr, 10);
        if (*endptr != '\0') {
            printf("No such job\n");
            return 1;
        }
        job_num = (int)val;
    }

    int idx = find_bg_job(job_num);
    if (idx < 0) {
        printf("No such job\n");
        return 1;
    }

    // Print the command being brought to foreground
    printf("%s\n", bg_jobs[idx].cmd_name);

    pid_t pid = bg_jobs[idx].pid;

    // Set as foreground process
    fg_pid = pid;
    strncpy(fg_cmd_name, bg_jobs[idx].cmd_name, BG_CMD_LEN - 1);
    fg_cmd_name[BG_CMD_LEN - 1] = '\0';

    // If stopped, resume it
    if (bg_jobs[idx].stopped) {
        kill(pid, SIGCONT);
        bg_jobs[idx].stopped = 0;
    }

    // Remove from background list (it's now foreground)
    bg_jobs[idx].active = 0;

    // Wait for the process to finish or stop again
    int status;
    waitpid(pid, &status, WUNTRACED);

    fg_pid = -1;
    fg_cmd_name[0] = '\0';

    if (WIFSTOPPED(status)) {
        // Process was stopped again (Ctrl-Z) — move back to bg
        add_bg_job_stopped(pid, bg_jobs[idx].cmd_name);
        return 0;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 1;
}

// Resume a stopped background job: bg [job_number]
int execute_bg_cmd(char **args, int arg_count) {
    int job_num = -1; // default: most recent

    if (arg_count > 0) {
        char *endptr;
        long val = strtol(args[0], &endptr, 10);
        if (*endptr != '\0') {
            printf("No such job\n");
            return 1;
        }
        job_num = (int)val;
    }

    int idx = find_bg_job(job_num);
    if (idx < 0) {
        printf("No such job\n");
        return 1;
    }

    if (!bg_jobs[idx].stopped) {
        printf("Job already running\n");
        return 1;
    }

    // Resume the stopped job
    kill(bg_jobs[idx].pid, SIGCONT);
    bg_jobs[idx].stopped = 0;

    printf("[%d] %s &\n", bg_jobs[idx].job_number, bg_jobs[idx].cmd_name);

    return 0;
}
