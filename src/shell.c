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
    
    // clearing the redirections and their filenames and passing the remaining tokens to the execvp function
    char *argv[SHELL_MAX_INPUT];
    int argc = 0;
    
    for (int i = 0; i < atomic_end; i++) {
        if (strcmp(cmd->tokens[i], "<") == 0 ||
            strcmp(cmd->tokens[i], ">") == 0 ||
            strcmp(cmd->tokens[i], ">>") == 0) {
            i++; // skip the filename after the operator
            continue;
        }
        argv[argc++] = cmd->tokens[i];
    }
    argv[argc] = NULL;

    if (argc == 0) {
        return 0;
    }

    // Fork and exec
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    
    if (pid == 0) {
        // Child process
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
