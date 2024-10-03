#include "../parser.h"
#include <stddef.h>
#include <stdio.h>

int getch(void) { return getchar(); }

void ungetch(int ch) { ungetc(ch, stdin); }

void exec_command(char **args)
{
    size_t len = 0;
    for (; args[len]; ++len)
        printf("\t\"%s\"\n", args[len]);
}

int main()
{
    const bool success = parse();
    printf("%s\n", success ? "success" : "failed");
}
