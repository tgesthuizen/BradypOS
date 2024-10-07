#ifndef BRADYPOS_BSH_CMD_H
#define BRADYPOS_BSH_CMD_H

enum
{
    CMD_MAX_ARGS = 8,
    CMD_BUF_SIZE = 128,
};

extern char *args[];
int getch();
void ungetch(int ch);

void parse_cmd();

#endif
