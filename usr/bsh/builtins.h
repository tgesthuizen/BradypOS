#ifndef BRADYPOS_BSH_BUILTINS_H
#define BRADYPOS_BSH_BUILTINS_H

#include <stdbool.h>

struct bsh_builtin
{
    const char *cmd;              ///< Name of the builtin
    void (*handler)(char **args); ///< Handler for parsing the remaining args
};

/**
 * @brief Contains all builtin commands of the shell.
 *
 * The last element of the array is @c NULL and terminates the array.
 */
extern const struct bsh_builtin *builtins[];

#endif
