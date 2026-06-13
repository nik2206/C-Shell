#include "log.h"

extern char home_dir[4096];

static char log_history[LOG_MAX_COMMANDS][LOG_MAX_CMD_LEN];
static int  log_count = 0;

static void get_log_path(char *buf, int bufsize) {
    snprintf(buf, bufsize, "%s/.shell_log", home_dir);
}

// Save log history 
static void save_log(void) {
    char path[4096];
    get_log_path(path, 4096);

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("save_log: fopen");
        return;
    }

    for (int i = 0; i < log_count; i++) {
        fprintf(fp, "%s\n", log_history[i]);// writing the log_history to the file
    }

    fclose(fp);
}

// Initialize log
void initialize_log(void) {
    char path[4096];
    get_log_path(path, 4096);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        log_count = 0;
        return;
    }

    log_count = 0;
    char line[LOG_MAX_CMD_LEN];
    while (fgets(line, LOG_MAX_CMD_LEN, fp) != NULL && log_count < LOG_MAX_COMMANDS) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (strlen(line) == 0) {
            continue;
        }
        strncpy(log_history[log_count], line, LOG_MAX_CMD_LEN - 1);
        log_history[log_count][LOG_MAX_CMD_LEN - 1] = '\0';
        log_count++;
    }

    fclose(fp);
}

// Add command to log history
void add_to_log(const char *cmd_str) {
    if (cmd_str == NULL || strlen(cmd_str) == 0) {
        return;
    }

    // Don't store if identical to the most recent entry
    if (log_count > 0 && strcmp(log_history[log_count - 1], cmd_str) == 0) {
        return;
    }

    // If at capacity, shift everything left to drop the oldest
    if (log_count == LOG_MAX_COMMANDS) {
        for (int i = 1; i < LOG_MAX_COMMANDS; i++) {
            strcpy(log_history[i - 1], log_history[i]);
        }
        log_count--;
    }

    strncpy(log_history[log_count], cmd_str, LOG_MAX_CMD_LEN - 1);
    log_history[log_count][LOG_MAX_CMD_LEN - 1] = '\0';
    log_count++;

    save_log();
}

// Check if input starts with the log command
int is_log_command(const char *raw_input) {
    if (raw_input == NULL) return 0;

    const char *p = raw_input;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }

    if (strncmp(p, "log", 3) == 0) {
        char c = p[3]; // to ensure that the 4th character is a valid separtor for the command  
        if (c == '\0' || c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
            c == '|'  || c == ';' || c == '&'  || c == '<'  || c == '>') {
            return 1;
        }
    }
    return 0;
}

// Execute log command
int execute_log(char **args, int arg_count, char *reexec_buf) {
    // No arguments - print history oldest to newest
    if (arg_count == 0) {
        for (int i = 0; i < log_count; i++) {
            printf("%s\n", log_history[i]);
        }
        return 0;
    }

    // clears the existing history
    if (arg_count == 1 && strcmp(args[0], "purge") == 0) {
        log_count = 0;
        save_log();
        return 0;
    }

    // execute <index> - re-execute command at index (1-indexed, newest first)
    if (arg_count == 2 && strcmp(args[0], "execute") == 0) {
        char *endptr;
        long idx = strtol(args[1], &endptr, 10);

        if (*endptr != '\0' || idx < 1 || idx > log_count) {
            printf("log: Invalid Syntax!\n");
            return 0;
        }

        int real_idx = log_count - (int)idx;
        strcpy(reexec_buf, log_history[real_idx]);
        return 1;
    }

    // Anything else is invalid
    printf("log: Invalid Syntax!\n");
    return 0;
}
