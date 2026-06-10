#include "shell.h"

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
        }
        
        free_parsed_command(&cmd);
    }
}
