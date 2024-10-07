#include "cmd.h"

static char buffer[CMD_BUF_SIZE];
char *args[CMD_MAX_ARGS + 1];

enum parser_state
{
    ps_init,

    ps_word_start,
    ps_word,
    ps_word_escape,

    ps_open_quote_word_start,
    ps_open_quote,
    ps_close_quote,
    ps_quote_escape,

    ps_space,
    ps_done,
};

void parse_cmd()
{
    char *bufpos = buffer;
    char **arg_pos = args;
    enum parser_state state = ps_init;
    while (1)
    {
        int ch = getch();
        switch (state)
        {
        case ps_init:
            switch (ch)
            {
            case ' ':
            case '\t':
            case '\n':
                break;
            case '"':
                state = ps_open_quote_word_start;
                break;
            default:
                state = ps_word;
                break;
            }
            break;
        case ps_word_start:
            *arg_pos++ = bufpos;
        // fallthrough
        case ps_word:
            switch (ch)
            {
            case '"':
                state = ps_open_quote;
                break;
            case ' ':
            case '\t':
                state = ps_space;
                ++arg_pos;
                break;
            case '\n':
                state = ps_done;
                break;
            default:
                *bufpos++ = ch;
                break;
            }
            break;
        case ps_word_escape:
            switch (ch)
            {
            case ' ':
                *bufpos++ = ' ';
                break;
            case '\\':
                *bufpos++ = '\\';
                break;
            case '"':
                *bufpos++ = '"';
                break;
            default:
                // TODO: Warn about invalid escape sequence
                break;
            }
            break;
        case ps_open_quote_word_start:

        case ps_open_quote:
            switch (ch)
            {
            case '"':
                state = ps_close_quote;
            }
        }
    }
}
