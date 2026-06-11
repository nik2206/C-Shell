#include "reveal.h"

extern char home_dir[4096];
extern char current_dir[4096];
extern char previous_dir[4096];

// For lexographical sorting 
int compare_names(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// to print directory contents
void print_directory_contents(const char *path, int show_all, int line_format) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        printf("No such directory!\n");
        return;
    }
    
    struct dirent *entry;
    char **entries = malloc(1024 * sizeof(char *));
    int entry_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }
        
        entries[entry_count] = malloc(strlen(entry->d_name) + 1);
        strcpy(entries[entry_count], entry->d_name);
        entry_count++;
    }
    closedir(dir);
    
    // Sort entries lexicographically
    qsort(entries, entry_count, sizeof(char *), compare_names);
    
    // Print entries
    if (line_format) {
        for (int i = 0; i < entry_count; i++) {
            printf("%s\n", entries[i]);
        }
    } else {
        for (int i = 0; i < entry_count; i++) {
            printf("%s", entries[i]);
            if (i < entry_count - 1) {
                printf(" ");
            }
        }
        if (entry_count > 0) {
            printf("\n");
        }
    }
    
    for (int i = 0; i < entry_count; i++) {
        free(entries[i]);
    }
    free(entries);
}

// Execute reveal command
int execute_reveal(char **args, int arg_count) {
    int show_all = 0;
    int line_format = 0;
    int path_index = -1;
    int flag_count = 0;
    
    for (int i = 0; i < arg_count; i++) {
        if (args[i][0] == '-' && args[i][1] != '\0') {
            flag_count++;
            
            for (int j = 1; args[i][j] != '\0'; j++) {
                if (args[i][j] == 'a') {
                    show_all = 1;
                } else if (args[i][j] == 'l') {
                    line_format = 1;
                }
            }
        } else if (args[i][0] != '-' || (args[i][0] == '-' && args[i][1] == '\0')) {
            if (path_index != -1) {
                printf("reveal: Invalid Syntax!\n");
                return 1;
            }
            path_index = i;
        }
    }
    
    char target_path[4096];
    
    if (path_index == -1) {
        strcpy(target_path, current_dir);
    } else {
        char *path_arg = args[path_index];
        
        if (strcmp(path_arg, "~") == 0) {
            strcpy(target_path, home_dir);
        } else if (strcmp(path_arg, ".") == 0) {
            strcpy(target_path, current_dir);
        } else if (strcmp(path_arg, "..") == 0) {
            char temp[4096];
            strcpy(temp, current_dir);
            
            char *last_slash = strrchr(temp, '/');
            if (last_slash == NULL || last_slash == temp) {
                strcpy(target_path, "/");
            } else {
                *last_slash = '\0';
                strcpy(target_path, temp);
            }
        } else if (strcmp(path_arg, "-") == 0) {
            if (strlen(previous_dir) == 0) {
                printf("No such directory!\n");
                return 1;
            }
            strcpy(target_path, previous_dir);
        } else {
            if (path_arg[0] == '/') {
                strcpy(target_path, path_arg);
            } else {
                strcpy(target_path, current_dir);
                if (target_path[strlen(target_path) - 1] != '/') {
                    strcat(target_path, "/");
                }
                strcat(target_path, path_arg);
            }
        }
    }
    
    struct stat st;
    if (stat(target_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("No such directory!\n");
        return 1;
    }
    
    print_directory_contents(target_path, show_all, line_format);
    
    return 0;
}
