#include "hop.h"

extern char home_dir[4096];
extern char current_dir[4096];
extern char previous_dir[4096];

// Execute hop command
int execute_hop(char **args, int arg_count) {
    if (arg_count == 0) {
        if (chdir(home_dir) != 0) {
            printf("No such directory!\n");
            return 1;
        }

        strcpy(previous_dir, current_dir);
        strcpy(current_dir, home_dir);
        return 0;
    }

    for (int i = 0; i < arg_count; i++) {
        char target_path[4096];

        if (strcmp(args[i], "~") == 0) {
            if (chdir(home_dir) != 0) {
                printf("No such directory!\n");
                return 1;
            }

            strcpy(previous_dir, current_dir);
            strcpy(current_dir, home_dir);

        } else if (strcmp(args[i], ".") == 0) {
            continue;

        } else if (strcmp(args[i], "..") == 0) {
            char *last_slash = strrchr(current_dir, '/');

            if (last_slash == NULL || last_slash == current_dir) {
                continue;
            }

            char temp_path[4096];
            strcpy(temp_path, current_dir);
            temp_path[last_slash - current_dir] = '\0';

            if (chdir(temp_path) != 0) {
                printf("No such directory!\n");
                return 1;
            }

            strcpy(previous_dir, current_dir);
            strcpy(current_dir, temp_path);

        } else if (strcmp(args[i], "-") == 0) {
            if (strlen(previous_dir) == 0) {
                continue;
            }

            if (chdir(previous_dir) != 0) {
                printf("No such directory!\n");
                return 1;
            }

            char temp[4096];
            strcpy(temp, current_dir);
            strcpy(current_dir, previous_dir);
            strcpy(previous_dir, temp);

        } else {
            if (args[i][0] == '/') {
                strcpy(target_path, args[i]);
            } else {
                strcpy(target_path, current_dir);

                if (target_path[strlen(target_path) - 1] != '/') {
                    strcat(target_path, "/");
                }

                strcat(target_path, args[i]);
            }

            struct stat st;

            if (stat(target_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
                printf("No such directory!\n");
                return 1;
            }

            if (chdir(target_path) != 0) {
                printf("No such directory!\n");
                return 1;
            }

            strcpy(previous_dir, current_dir);
            strcpy(current_dir, target_path);
        }
    }

    return 0;
}