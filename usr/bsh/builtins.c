#include "builtins.h"
#include <stddef.h>

extern struct bsh_builtin cd_builtin;
extern struct bsh_builtin pwd_builtin;
extern struct bsh_builtin ls_builtin;

const struct bsh_builtin *builtins[] = {&cd_builtin, &pwd_builtin, &ls_builtin,
                                        NULL};
