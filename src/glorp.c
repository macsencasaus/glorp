#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "glorpoptions.h"
#include "interpreter.h"
#include "repl.h"
#include "utils.h"

#define STDIN_FILENAME "<stdin>"

arena a;

typedef struct {
    char *long_name;
    char short_name;
    char *desc;
    glorp_flags flag;
} option;

const char *program_name;

static void help(void);
static void try_help(void);
static void invalid_long_option(char *arg);
static void invalid_short_option(char arg);
static void parse_arguments(int argc, char *argv[], glorp_options *options);
static void parse_long_arg(char *arg, glorp_options *options);
static void parse_short_arg(char *arg, glorp_options *options);
/* static char *read_file(glorp_options *options); */
/* static char *read_stdin(void); */

// clang-format off
static const option options[] = {
    {"help",     'h',  "give this help",         HELP_FLAG},
    {"lex",      'l',  "print lexer output",     LEX_FLAG},
    {"only-lex", '\0', "process up to lexer",    ONLY_LEX_FLAG},
    {"ast",      'a',  "print ast",              AST_FLAG},
    {"only-ast", '\0', "process up to ast",      ONLY_AST_FLAG},
    {"repl",     'r',  "start interactive repl", REPL_FLAG},
    {"verbose",  'v',  "verbose mode",           VERBOSE_FLAG},
};
// clang-format on

static const size_t option_count = sizeof(options) / sizeof(option);

int main(int argc, char *argv[]) {
    program_name = argv[0];

    glorp_options selected_options = {0};

    parse_arguments(argc, argv, &selected_options);

    if (selected_options.flags & HELP_FLAG) {
        help();
    }

    if (selected_options.flags & REPL_FLAG) {
        start_repl(&selected_options);
        return 0;
    }

    char *input;
    if (selected_options.flags & STDIN_FLAG) {
        input = read_stdin();
    } else {
        input = read_file(selected_options.file_name);
    }

    interpret(input, &selected_options);

    free(input);

    return 0;
}

static void help(void) {
    static const char help_header[] =
        "usage: %s [option] ... [file | -] [arg] ...\n"
        "repl starts if no file is specified\n"
        "\n"
        "options:\n";

    printf(help_header, program_name);

    for (size_t i = 0; i < option_count; ++i) {
        if (options[i].short_name == 0) {
            printf("      --%-10s %s\n", options[i].long_name, options[i].desc);
            continue;
        }
        printf("  -%c, --%-10s %s\n", options[i].short_name,
               options[i].long_name, options[i].desc);
    }

    exit(0);
}

static void try_help(void) {
    fprintf(stderr, "Try `%s --help` for more information.\n", program_name);
    exit(1);
}

static void invalid_long_option(char *arg) {
    const char invalid_long_option_message[] = "%s: invalid option -- '%s'\n";

    fprintf(stderr, invalid_long_option_message, program_name, arg);
    try_help();
}

static void invalid_short_option(char arg) {
    const char invalid_short_option_message[] = "%s: invalid option -- '%c'\n";

    fprintf(stderr, invalid_short_option_message, program_name, arg);
    try_help();
}

static void parse_arguments(int argc, char *argv[], glorp_options *selected_options) {
    bool filename_seen = false;
    bool leave_loop = false;

    char *arg;
    for (int i = 1; i < argc && !leave_loop; ++i) {
        arg = argv[i];

        if (!*arg) {
            continue;
        }

        switch (arg[0]) {
            case '-': {
                if (filename_seen) {
                    selected_options->args = argv + i;
                    selected_options->argc = (size_t)(argc - i);
                    leave_loop = true;
                    break;
                }

                if (!arg[1]) {
                    selected_options->file_name = STDIN_FILENAME;
                    selected_options->flags |= STDIN_FLAG;
                    filename_seen = true;
                    break;
                }

                switch (arg[1]) {
                    case '-': {
                        parse_long_arg(arg, selected_options);
                    } break;
                    default: {
                        parse_short_arg(arg, selected_options);
                    } break;
                }
            } break;
            default: {
                if (!filename_seen) {
                    selected_options->file_name = arg;
                    filename_seen = true;
                } else {
                    selected_options->args = argv + i;
                    selected_options->argc = (size_t)(argc - i);
                    leave_loop = true;
                }
            } break;
        }
    }
    if (selected_options->file_name == NULL) {
        selected_options->flags |= REPL_FLAG;
    }
}

static void parse_long_arg(char *arg, glorp_options *selected_options) {
    if (!arg[2]) {
        fprintf(stderr, "%s: empty long option name '--'\n", program_name);
        try_help();
    }

    arg = arg + 2;

    for (size_t i = 0; i < option_count; ++i) {
        const option *o = options + i;
        if (strcmp(arg, o->long_name) == 0) {
            selected_options->flags |= o->flag;
            return;
        }
    }

    invalid_long_option(arg);
}

static void parse_short_arg(char *arg, glorp_options *selected_options) {
    for (size_t i = 1; arg[i]; ++i) {
        bool found = false;
        for (size_t j = 0; j < option_count; ++j) {
            const option *o = options + j;
            if (o->short_name == arg[i]) {
                selected_options->flags |= o->flag;
                found = true;
                break;
            }
        }
        if (!found)
            invalid_short_option(arg[i]);
    }
}
