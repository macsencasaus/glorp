#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glorp_options.h"
#include "interpreter.h"
#include "repl.h"

typedef struct {
    char *long_name;
    char *short_name;
    char *desc;
} option;

static char *program_name;

static void help(void);
static void try_help(void);
static void invalid_long_option(char *arg);
static void invalid_short_option(char arg);
static void parse_arguments(int argc, char *argv[], glorp_options *options);
static void parse_long_arg(char *arg, glorp_options *options);
static void parse_short_arg(char *arg, glorp_options *options);
static char *read_file(glorp_options *options);

static const option options[] = {
    {"help", "h", "give this help"},
    {"lex", "l", "print lexer output"},
    {"only-lex", "", "process up to lexer"},
    {"ast", "a", "print ast"},
    {"only-ast", "", "process up to ast"},
    {"repl", "r", "start interactive repl"},
    {0},
};

int main(int argc, char *argv[]) {
    program_name = argv[0];

    glorp_options options = {0};

    parse_arguments(argc, argv, &options);

    if (options.flags & HELP_FLAG) {
        help();
    }

    if (options.flags & REPL_FLAG) {
        start_repl(&options);
        return 0;
    }

    char *input = read_file(&options);

    interpret(input, &options);

    free(input);

    return 0;
}

static void help(void) {
    static const char *const help_header =
        "usage: %s [option] ... [file | -] [arg] ...\n"
        "repl starts if no file is specified\n"
        "\n"
        "options:\n";

    printf(help_header, program_name);

    for (size_t i = 0; *(char *)(options + i); ++i) {
        if (*options[i].short_name == 0) {
            printf("      --%-10s %s\n", options[i].long_name, options[i].desc);
            continue;
        }
        printf("  -%s, --%-10s %s\n", options[i].short_name,
               options[i].long_name, options[i].desc);
    }

    exit(0);
}

static void try_help(void) {
    fprintf(stderr, "Try `%s --help` for more information.\n", program_name);
    exit(1);
}

static void invalid_long_option(char *arg) {
    const char *const invalid_long_option_message =
        "%s: invalid option -- '%s'\n";

    fprintf(stderr, invalid_long_option_message, program_name, arg);
    try_help();
}

static void invalid_short_option(char arg) {
    const char *const invalid_long_option_message =
        "%s: invalid option -- '%c'\n";

    fprintf(stderr, invalid_long_option_message, program_name, arg);
    try_help();
}

static void parse_arguments(int argc, char *argv[], glorp_options *options) {
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
                    options->args = argv + i;
                    options->argc = (size_t)(argc - i);
                    leave_loop = true;
                    break;
                }

                if (!arg[1]) {
                    options->filename = arg;
                    options->flags |= REPL_FLAG;
                    filename_seen = true;
                    break;
                }

                switch (arg[1]) {
                    case '-': {
                        parse_long_arg(arg, options);
                    } break;
                    default: {
                        parse_short_arg(arg, options);
                    } break;
                }
            } break;
            default: {
                if (!filename_seen) {
                    options->filename = arg;
                    filename_seen = true;
                } else {
                    options->args = argv + i;
                    options->argc = (size_t)(argc - i);
                    leave_loop = true;
                }
            } break;
        }
    }
    if (options->filename == NULL) {
        options->flags |= REPL_FLAG;
    }
}

static void parse_long_arg(char *arg, glorp_options *options) {
    if (!arg[2]) {
        fprintf(stderr, "%s: empty long option name '--'\n", program_name);
        try_help();
    }

    arg = arg + 2;
    if (strcmp(arg, "help") == 0) {
        options->flags |= HELP_FLAG;
    } else if (strcmp(arg, "lex") == 0) {
        options->flags |= LEX_FLAG;
    } else if (strcmp(arg, "only-lex") == 0) {
        options->flags |= ONLY_LEX_FLAG;
    } else if (strcmp(arg, "ast") == 0) {
        options->flags |= AST_FLAG;
    } else if (strcmp(arg, "only-ast") == 0) {
        options->flags |= ONLY_AST_FLAG;
    } else if (strcmp(arg, "repl") == 0) {
        options->flags |= REPL_FLAG;
    } else {
        invalid_long_option(arg);
    }
}

static void parse_short_arg(char *arg, glorp_options *options) {
    for (size_t i = 1; arg[i]; ++i) {
        switch (arg[i]) {
            case 'h': {
                options->flags |= HELP_FLAG;
            } break;
            case 'l': {
                options->flags |= LEX_FLAG;
            } break;
            case 'a': {
                options->flags |= AST_FLAG;
            } break;
            case 'r': {
                options->flags |= REPL_FLAG;
            } break;
            default: {
                invalid_short_option(arg[i]);
            } break;
        }
    }
}

static char *read_file(glorp_options *options) {
    const char *filename = options->filename;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "%s: %s: No such file or directory\n", program_name,
                filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if ((long)bytes_read != file_size) {
        fprintf(stderr, "%s: %s: Failed to read file\n", program_name,
                filename);
        free(buffer);
        fclose(file);
        exit(1);
    }

    buffer[file_size] = 0;

    fclose(file);
    return buffer;
}
