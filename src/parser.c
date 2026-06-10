#include "parser.h"

// Check if a string is a special operator
int is_operator(const char *str) {
    if (str == NULL) return 0;
    
    if (strcmp(str, PIPE_OP) == 0)      return 1;  
    if (strcmp(str, SEMICOLON_OP) == 0) return 1;  
    if (strcmp(str, AMPERSAND_OP) == 0) return 1;  
    if (strcmp(str, INPUT_OP) == 0)     return 1;  // <
    if (strcmp(str, OUTPUT_OP) == 0)    return 1;  // >
    if (strcmp(str, APPEND_OP) == 0)    return 1;  // >>
    
    return 0;
}


int validate_syntax(ParsedCommand *cmd) {
    if (cmd->token_count == 0) {
        return 1;  // Empty is valid as user can just press Enter
    }
    
    if (is_operator(cmd->tokens[0]) && strcmp(cmd->tokens[0], AMPERSAND_OP) != 0) {
        return 0;
    }
    
    // Last token cannot be an operator (except &)
    if (is_operator(cmd->tokens[cmd->token_count - 1]) && 
        strcmp(cmd->tokens[cmd->token_count - 1], AMPERSAND_OP) != 0) {
        return 0;
    }
    
    // Check for invalid operator sequences
    for (int i = 0; i < cmd->token_count - 1; i++) {
        char *current = cmd->tokens[i];
        char *next = cmd->tokens[i + 1];
        
        if ((strcmp(current, PIPE_OP) == 0 || strcmp(current, SEMICOLON_OP) == 0)) {
            if (is_operator(next) && strcmp(next, AMPERSAND_OP) != 0) {
                return 0;  
            }
        }
        
        // & or < or > cannot be followed by & or ; or |
        if ((strcmp(current, AMPERSAND_OP) == 0 || 
             strcmp(current, INPUT_OP) == 0 || 
             strcmp(current, OUTPUT_OP) == 0 ||
             strcmp(current, APPEND_OP) == 0)) {
            if (strcmp(current, AMPERSAND_OP) == 0 && 
                (strcmp(next, SEMICOLON_OP) == 0 || strcmp(next, AMPERSAND_OP) == 0)) {
                return 0;
            }
            if ((strcmp(current, INPUT_OP) == 0 || 
                 strcmp(current, OUTPUT_OP) == 0 ||
                 strcmp(current, APPEND_OP) == 0) && is_operator(next)) {
                return 0;  // Redirection must be followed by filename
            }
        }
    }
    
    return 1; 
}

// Tokenize input - split by spaces AND special operators
// >> must be kept together as one token
ParsedCommand parse_input(char *input) {
    ParsedCommand cmd;
    cmd.tokens = NULL;
    cmd.token_count = 0;
    cmd.valid = 1;
    
    if (input == NULL || strlen(input) == 0) {
        cmd.valid = 1;  
        return cmd;
    }
    
    char *input_copy = malloc(strlen(input) + 1);
    strcpy(input_copy, input);
    
    cmd.tokens = malloc((strlen(input) + 1) * sizeof(char *));
    
    char *ptr = input_copy;
    
    while (*ptr != '\0') {
        // Skip whitespace
        while (*ptr != '\0' && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) {
            ptr++;
        }
        
        if (*ptr == '\0') break;
        
        // Check for special operators
        if (*ptr == '>' && *(ptr + 1) == '>') {
            cmd.tokens[cmd.token_count] = malloc(3);
            strcpy(cmd.tokens[cmd.token_count], ">>");
            cmd.token_count++;
            ptr += 2;
        } else if (*ptr == '<') {
            cmd.tokens[cmd.token_count] = malloc(2);
            strcpy(cmd.tokens[cmd.token_count], "<");
            cmd.token_count++;
            ptr++;
        } else if (*ptr == '>') {
            cmd.tokens[cmd.token_count] = malloc(2);
            strcpy(cmd.tokens[cmd.token_count], ">");
            cmd.token_count++;
            ptr++;
        } else if (*ptr == '|') {
            cmd.tokens[cmd.token_count] = malloc(2);
            strcpy(cmd.tokens[cmd.token_count], "|");
            cmd.token_count++;
            ptr++;
        } else if (*ptr == ';') {
            cmd.tokens[cmd.token_count] = malloc(2);
            strcpy(cmd.tokens[cmd.token_count], ";");
            cmd.token_count++;
            ptr++;
        } else if (*ptr == '&') {
            cmd.tokens[cmd.token_count] = malloc(2);
            strcpy(cmd.tokens[cmd.token_count], "&");
            cmd.token_count++;
            ptr++;
        } else {
            char *token_start = ptr;
            
            // Read until whitespace or operator
            while (*ptr != '\0' && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\r' &&
                   *ptr != '<' && *ptr != '>' && *ptr != '|' && *ptr != ';' && *ptr != '&') {
                ptr++;
            }
            
            int token_len = ptr - token_start;
            cmd.tokens[cmd.token_count] = malloc(token_len + 1);
            strncpy(cmd.tokens[cmd.token_count], token_start, token_len);
            cmd.tokens[cmd.token_count][token_len] = '\0';
            cmd.token_count++;
        }
    }
    
    free(input_copy);
    
    // Validate the syntax
    if (!validate_syntax(&cmd)) {
        cmd.valid = 0;
    }
    
    return cmd;
}

void free_parsed_command(ParsedCommand *cmd) {
    if (cmd == NULL) return;
    
    for (int i = 0; i < cmd->token_count; i++) {
        free(cmd->tokens[i]);
    }
    free(cmd->tokens);
    
    cmd->token_count = 0;
    cmd->tokens = NULL;
}
