#include "bg.h"

typedef struct {
    pid_t pid;
    char cmd_name[BG_CMD_LEN];
    int active;
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
        pid_t result = waitpid(bg_jobs[i].pid, &status, WNOHANG);

        if (result > 0) {
            bg_jobs[i].active = 0;

            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                printf("%s with pid %d exited normally\n",
                       bg_jobs[i].cmd_name, bg_jobs[i].pid);
            } else {
                printf("%s with pid %d exited abnormally\n",
                       bg_jobs[i].cmd_name, bg_jobs[i].pid);
            }
        }
    }
}
