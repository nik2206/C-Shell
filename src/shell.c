#include "shell.h"

int execute_command(ParsedCommand *cmd) {
    if (cmd->token_count == 0) {
        return 0;  
    }
    
    char *command = cmd->tokens[0];
    
    if (strcmp(command, "hop") == 0) {
        char **args = &cmd->tokens[1];
        int arg_count = cmd->token_count - 1;
        return execute_hop(args, arg_count);
    }
    
    if (strcmp(command, "reveal") == 0) {
        char **args = &cmd->tokens[1];
        int arg_count = cmd->token_count - 1;
        return execute_reveal(args, arg_count);
    }
    
    
    return 0; 
}

void run_shell(void) {
    char input[SHELL_MAX_INPUT];
    
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
        
        ParsedCommand cmd = parse_input(input);
        
        if (cmd.token_count > 0 && !cmd.valid) {
            printf("Invalid Syntax!\n");
        } else if (cmd.token_count > 0) {
            execute_command(&cmd);
        }
        
        free_parsed_command(&cmd);
    }
}
