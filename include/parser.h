#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define PIPE_OP      "|"
#define SEMICOLON_OP ";"
#define AMPERSAND_OP "&"
#define INPUT_OP     "<"
#define OUTPUT_OP    ">"
#define APPEND_OP    ">>"

typedef struct {
    char **tokens;      
    int token_count;    
    int valid;          
} ParsedCommand;

// Parse user input into tokens and validate syntax
ParsedCommand parse_input(char *input);

// Free allocated memory 
void free_parsed_command(ParsedCommand *cmd);

// Check if a string is a special operator
int is_operator(const char *str);

int validate_syntax(ParsedCommand *cmd);

#endif
