#include "prompt.h"
#include "shell.h"
#include <pwd.h>

// Initialize shell state getting home directory and current directory
void initialize_shell(void) {
    // Gets home directory 
    if (getcwd(home_dir, SHELL_MAX_PATH) == NULL) {
        perror("getcwd failed");
        exit(1);
    }
    
    // Initialize current directory same as home
    strcpy(current_dir, home_dir);
    
    // Prev directory empty as no previous directory exists at start
    strcpy(previous_dir, "");
}

// Display the shell prompt
void display_prompt(void) {
    char username[SHELL_MAX_PATH];
    char hostname[SHELL_MAX_PATH];
    char display_dir[SHELL_MAX_PATH];
    
    // Get username
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        strcpy(username, "user");
    } else {
        strcpy(username, pw->pw_name);
    }
    
    // Get hostname
    if (gethostname(hostname, SHELL_MAX_PATH) == -1) {
        strcpy(hostname, "host");
    }
    

    if (strcmp(current_dir, home_dir) == 0) {
        strcpy(display_dir, "~");
    } else if (strncmp(current_dir, home_dir, strlen(home_dir)) == 0 && 
               current_dir[strlen(home_dir)] == '/') {

        strcpy(display_dir, "~");
        strcat(display_dir, &current_dir[strlen(home_dir)]);
    } else {

        strcpy(display_dir, current_dir);
    }

    printf("[%s@%s:%s]$ ", username, hostname, display_dir);
    fflush(stdout);
}
