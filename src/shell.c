#include "shell.h"

// Execute a parsed command by dispatching to the correct handler
int execute_command(ParsedCommand *cmd) {
    if (cmd->token_count == 0) {
        return 0;  
    }
    
    // Find end of first cmd_group (stop at ; or &)
    int group_end = cmd->token_count;
    for (int i = 0; i < cmd->token_count; i++) {
        if (strcmp(cmd->tokens[i], ";") == 0 || strcmp(cmd->tokens[i], "&") == 0) {
            group_end = i;
            break;
        }
    }

    // Find end of first atomic within the cmd_group (stop at |)
    int atomic_end = group_end;
    for (int i = 0; i < group_end; i++) {
        if (strcmp(cmd->tokens[i], "|") == 0) {
            atomic_end = i;
            break;
        }
    }

    if (atomic_end == 0) {
        return 0;
    }
    
    char *command = cmd->tokens[0];
    
    // Handle builtins
    if (strcmp(command, "hop") == 0) {
        char **args = &cmd->tokens[1];
        int arg_count = atomic_end - 1;
        return execute_hop(args, arg_count);
    }
    
    if (strcmp(command, "reveal") == 0) {
        char **args = &cmd->tokens[1];
        int arg_count = atomic_end - 1;
        return execute_reveal(args, arg_count);
    }
    
    if (strcmp(command, "log") == 0) {
        char **args = &cmd->tokens[1];
        int arg_count = atomic_end - 1;
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
    
    // Build argv, skipping redirection operators and their filenames
    char *argv[SHELL_MAX_INPUT];
    int argc = 0;
    
    // Find the last input/output redirection files 
    char *input_file = NULL;
    char *output_file = NULL;
    int output_append = 0; // 0 = truncate (>), 1 = append (>>)
    
    for (int i = 0; i < atomic_end; i++) {
        if (strcmp(cmd->tokens[i], "<") == 0) {
            if (i + 1 < atomic_end) {
                input_file = cmd->tokens[i + 1];
                i++; // skip the filename
            }
            continue;
        }
        if (strcmp(cmd->tokens[i], ">>") == 0) {
            if (i + 1 < atomic_end) {
                output_file = cmd->tokens[i + 1];
                output_append = 1;
                i++; // skip the filename
            }
            continue;
        }
        if (strcmp(cmd->tokens[i], ">") == 0) {
            if (i + 1 < atomic_end) {
                output_file = cmd->tokens[i + 1];
                output_append = 0;
                i++; // skip the filename
            }
            continue;
        }
        argv[argc++] = cmd->tokens[i];
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
        // If execvp returns, the command was not found
        printf("Command not found!\n");
        _exit(1);
    }
    
    // Parent process - wait for child to finish
    int status;
    waitpid(pid, &status, 0);
    
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
