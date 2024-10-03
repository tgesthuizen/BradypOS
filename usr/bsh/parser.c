#include "parser.h"
#include <stdbool.h>

static char cmd_buffer[MAX_COMMAND_LINE_LEN + 1];
static char *cmd_arg[MAX_COMMAND_ARGS + 1];

static char *cmd_buffer_pos;
static char **cmd_arg_pos;

enum parse_result
{
    accepted,
    rejected,
    failed,
};

static enum parse_result parse_word()
{
    enum word_state
    {
        init,
        word,
        word_escape,

        quote_start,
        quote,
        quote_escape,
        quote_end,

        space,
    };

    enum word_state state = init;
    while (1)
    {
        int ch = getch();
        switch (state)
        {
        case init:
            switch (ch)
            {
            case ' ':
            case '\t':
                state = space;
                break;
            case '\n':
                ungetch(ch);
                return rejected;
            case '"':
                state = quote_start;
                break;
            case '\\':
                *cmd_arg_pos++ = cmd_buffer_pos;
                state = word_escape;
                break;
            default:
                *cmd_arg_pos++ = cmd_buffer_pos;
                *cmd_buffer_pos++ = ch;
                state = word;
                break;
            }
            break;
        case word:
            switch (ch)
            {
            case '\n':
                ungetch(ch);
                // fall-through
            case ' ':
            case '\t':
                *cmd_buffer_pos++ = '\0';
                return accepted;
            case '"':
                state = quote_start;
                break;
            case '\\':
                state = word_escape;
                break;
            default:
                *cmd_buffer_pos++ = ch;
            }
            break;
        case word_escape:
            switch (ch)
            {
            case '\\':
                *cmd_buffer_pos++ = '\\';
                break;
            case ' ':
                *cmd_buffer_pos++ = ' ';
                break;
            case '\n':
            case EOF:
                return failed;
            default:
                // TODO: Print error message about unrecognized escape sequence.
                return failed;
            }
            state = word;
            break;
        case quote_start:
            *cmd_arg_pos++ = cmd_buffer_pos;
            state = quote;
            // falll-through
        case quote:
            switch (ch)
            {
            case '\\':
                state = quote_escape;
                break;
            case '"':
                state = quote_end;
                break;
            default:
                *cmd_buffer_pos++ = ch;
                break;
            }
            break;
        case quote_escape:
            switch (ch)
            {
            case 'n':
                *cmd_buffer_pos++ = '\n';
                break;
            case 't':
                *cmd_buffer_pos++ = '\t';
                break;
            default:
                *cmd_buffer_pos++ = ch;
                break;
            }
            state = quote;
            break;
        case quote_end:
            switch (ch)
            {
            case ' ':
            case '\t':
                *cmd_buffer_pos++ = '\0';
                return accepted;
            case '\\':
                state = word_escape;
                break;
            case '"':
                state = quote_start;
                break;
            default:
                *cmd_buffer_pos++ = ch;
                state = word;
                break;
            }
            break;
        case space:
            switch (ch)
            {
            case ' ':
            case '\t':
                break;
            case '\n':
                return rejected;
            case '"':
                state = quote_start;
                break;
            default:
                *cmd_arg_pos++ = cmd_buffer_pos;
                *cmd_buffer_pos++ = ch;
                break;
            }
            break;
        }
    }
}

bool parse()
{
    cmd_buffer_pos = cmd_buffer;
    cmd_arg_pos = cmd_arg;
    bool keep_parsing = true;
    while (keep_parsing)
    {
        switch (parse_word())
        {
        case accepted:
            break;
        case rejected:
            keep_parsing = false;
            break;
        case failed:
            return false;
        }
    }

    exec_command(cmd_arg);

    return true;
}
