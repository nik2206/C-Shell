#include "shell.h"

// Global variables
char home_dir[SHELL_MAX_PATH];
char current_dir[SHELL_MAX_PATH];
char previous_dir[SHELL_MAX_PATH];

int main(void) 
{
    // Initialize shell
    initialize_shell();
    
    // Run the main shell loop
    run_shell();
    
    return 0;
}
