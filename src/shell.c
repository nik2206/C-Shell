#include "shell.h"

#define MAX_PIPE_CMDS 64

// Execute a pipeline of commands connected by pipes
static int execute_pipeline(char **tokens, int group_end) {
    // Find pipe positions
    int pipe_pos[MAX_PIPE_CMDS];
    int pipe_count = 0;

    for (int i = 0; i < group_end; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            pipe_pos[pipe_count++] = i;
        }
    }

    int cmd_count = pipe_count + 1;

    // Find start and end index for each command's tokens
    int cmd_start[MAX_PIPE_CMDS];
    int cmd_end[MAX_PIPE_CMDS];

    cmd_start[0] = 0;
    for (int i = 0; i < pipe_count; i++) {
        cmd_end[i] = pipe_pos[i];
        cmd_start[i + 1] = pipe_pos[i] + 1;
    }
    cmd_end[cmd_count - 1] = group_end;

    // Create pipes between adjacent commands
    int pipe_fds[MAX_PIPE_CMDS][2];
    for (int i = 0; i < pipe_count; i++) {
        if (pipe(pipe_fds[i]) < 0) {
            perror("pipe");
            return 1;
        }
    }

    // Fork a child for each command in the pipeline
    pid_t pids[MAX_PIPE_CMDS];
    for (int j = 0; j < cmd_count; j++) {
        pids[j] = -1;
    }

    for (int j = 0; j < cmd_count; j++) {
        // Extract argv, input_file, output_file for this atomic
        char *argv[SHELL_MAX_INPUT];
        int argc = 0;
        char *input_file = NULL;
        char *output_file = NULL;
        int output_append = 0;

        for (int i = cmd_start[j]; i < cmd_end[j]; i++) {
            if (strcmp(tokens[i], "<") == 0) {
                if (i + 1 < cmd_end[j]) {
                    input_file = tokens[i + 1];
                    i++;
                }
                continue;
            }
            if (strcmp(tokens[i], ">>") == 0) {
                if (i + 1 < cmd_end[j]) {
                    output_file = tokens[i + 1];
                    output_append = 1;
                    i++;
                }
                continue;
            }
            if (strcmp(tokens[i], ">") == 0) {
                if (i + 1 < cmd_end[j]) {
                    output_file = tokens[i + 1];
                    output_append = 0;
                    i++;
                }
                continue;
            }
            argv[argc++] = tokens[i];
        }
        argv[argc] = NULL;

        if (argc == 0) {
            continue;
        }

        pids[j] = fork();
        if (pids[j] < 0) {
            perror("fork");
            continue;
        }

        if (pids[j] == 0) {
            // Connect pipe input
            if (j > 0) {
                dup2(pipe_fds[j - 1][0], STDIN_FILENO);
            }

            // Connect pipe output
            if (j < cmd_count - 1) {
                dup2(pipe_fds[j][1], STDOUT_FILENO);
            }

            for (int i = 0; i < pipe_count; i++) {
                close(pipe_fds[i][0]);
                close(pipe_fds[i][1]);
            }

            // File redirections override pipe redirections
            if (input_file != NULL) {
                int fd_in = open(input_file, O_RDONLY);
                if (fd_in < 0) {
                    printf("No such file or directory\n");
                    _exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            if (output_file != NULL) {
                int flags = O_WRONLY | O_CREAT;
                flags |= output_append ? O_APPEND : O_TRUNC;
                int fd_out = open(output_file, flags, 0644);
                if (fd_out < 0) {
                    printf("Unable to create file for writing\n");
                    _exit(1);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            execvp(argv[0], argv);
            printf("Command not found!\n");
            _exit(1);
        }
    }

    // Parent: close all pipe fds
    for (int i = 0; i < pipe_count; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }

    // Wait for all children to finish
    for (int j = 0; j < cmd_count; j++) {
        if (pids[j] > 0) {
            int status;
            waitpid(pids[j], &status, 0);
        }
    }

    return 0;
}

static int execute_cmd_group(char **tokens, int count) {
    if (count == 0) {
        return 0;
    }

    // Check for pipes in this cmd_group
    int has_pipes = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            has_pipes = 1;
            break;
        }
    }

    // If pipes present use pipeline execution
    if (has_pipes) {
        return execute_pipeline(tokens, count);
    }

    // Single command 
    char *command = tokens[0];

    if (strcmp(command, "hop") == 0) {
        char **args = &tokens[1];
        int arg_count = count - 1;
        return execute_hop(args, arg_count);
    }

    if (strcmp(command, "reveal") == 0) {
        char **args = &tokens[1];
        int arg_count = count - 1;
        return execute_reveal(args, arg_count);
    }

    if (strcmp(command, "log") == 0) {
        char **args = &tokens[1];
        int arg_count = count - 1;
        char reexec_buf[SHELL_MAX_INPUT];
        reexec_buf[0] = '\0';

        int needs_reexec = execute_log(args, arg_count, reexec_buf);

        // Re-parse and execute the recalled command
        if (needs_reexec && strlen(reexec_buf) > 0) {
            ParsedCommand recmd = parse_input(reexec_buf);
            if (recmd.token_count > 0 && !recmd.valid) {
                printf("Invalid Syntax!\n");
            } else if (recmd.token_count > 0) {
                execute_command(&recmd);
            }
            free_parsed_command(&recmd);
        }
        return 0;
    }

    // External single command - build argv with redirections
    char *argv[SHELL_MAX_INPUT];
    int argc = 0;
    char *input_file = NULL;
    char *output_file = NULL;
    int output_append = 0;

    for (int i = 0; i < count; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 < count) {
                input_file = tokens[i + 1];
                i++;
            }
            continue;
        }
        if (strcmp(tokens[i], ">>") == 0) {
            if (i + 1 < count) {
                output_file = tokens[i + 1];
                output_append = 1;
                i++;
            }
            continue;
        }
        if (strcmp(tokens[i], ">") == 0) {
            if (i + 1 < count) {
                output_file = tokens[i + 1];
                output_append = 0;
                i++;
            }
            continue;
        }
        argv[argc++] = tokens[i];
    }
    argv[argc] = NULL;

    if (argc == 0) {
        return 0;
    }

    // Validate input file before forking
    if (input_file != NULL) {
        int test_fd = open(input_file, O_RDONLY);
        if (test_fd < 0) {
            printf("No such file or directory\n");
            return 1;
        }
        close(test_fd);
    }

    // Validate output file before forking
    if (output_file != NULL) {
        int flags = O_WRONLY | O_CREAT;
        flags |= output_append ? O_APPEND : O_TRUNC;
        int test_fd = open(output_file, flags, 0644);
        if (test_fd < 0) {
            printf("Unable to create file for writing\n");
            return 1;
        }
        close(test_fd);
    }

    // Fork and exec
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child process - set up input redirection
        if (input_file != NULL) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in < 0) {
                printf("No such file or directory\n");
                _exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        // Child process - set up output redirection
        if (output_file != NULL) {
            int flags = O_WRONLY | O_CREAT;
            flags |= output_append ? O_APPEND : O_TRUNC;
            int fd_out = open(output_file, flags, 0644);
            if (fd_out < 0) {
                printf("Unable to create file for writing\n");
                _exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        execvp(argv[0], argv);
        printf("Command not found!\n");
        _exit(1);
    }

    // Parent process - wait for child to finish
    int status;
    waitpid(pid, &status, 0);

    return 0;
}

// Execute a parsed command - loops through cmd_groups separated by ; or &
int execute_command(ParsedCommand *cmd) {
    if (cmd->token_count == 0) {
        return 0;
    }

    int i = 0;
    while (i < cmd->token_count) {
        // Find end of current cmd_group 
        int group_end = cmd->token_count;
        for (int j = i; j < cmd->token_count; j++) {
            if (strcmp(cmd->tokens[j], ";") == 0 || strcmp(cmd->tokens[j], "&") == 0) {
                group_end = j;
                break;
            }
        }

        // Execute cmd_group
        int group_len = group_end - i;
        if (group_len > 0) {
            execute_cmd_group(&cmd->tokens[i], group_len);
        }

        // Move to next cmd_group
        if (group_end < cmd->token_count) {
            i = group_end + 1;
        } else {
            break;
        }
    }

    return 0;
}

void run_shell(void) {
    char input[SHELL_MAX_INPUT];

    // Load persistent log history at startup
    initialize_log();

    while (1) { // infinite loop as we want the shell to keep running until user exits
        display_prompt();

        if (fgets(input, SHELL_MAX_INPUT, stdin) == NULL) {
            printf("logout\n");
            break;
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        if (strlen(input) == 0) {
            continue;
        }

        // Keep a copy of the raw input before parsing for the log
        char raw_input[SHELL_MAX_INPUT];
        strncpy(raw_input, input, SHELL_MAX_INPUT - 1);
        raw_input[SHELL_MAX_INPUT - 1] = '\0';

        ParsedCommand cmd = parse_input(input);

        if (cmd.token_count > 0 && !cmd.valid) {
            printf("Invalid Syntax!\n");
        } else if (cmd.token_count > 0) {
            // Store in log before executing, but skip if command is "log"
            if (!is_log_command(raw_input)) {
                add_to_log(raw_input);
            }

            execute_command(&cmd);
        }

        free_parsed_command(&cmd);
    }
}
