#ifndef BRADYPOS_BSH_PARSER_H
#define BRADYPOS_BSH_PARSER_H

#include <stdbool.h>

/**
 * @file parser.h
 * Interface for the shell command parser of bsh.
 */

enum
{
    EOF = -1,
    MAX_COMMAND_LINE_LEN = 128,
    MAX_COMMAND_ARGS = 8,
};

int getch();
void ungetch(int ch);
void exec_command(char **args);

bool parse();

#endif
