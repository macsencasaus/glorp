#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "glorpoptions.h"
#include "interpreter.h"
#include "repl.h"
#include "utils.h"

#define ARGPARSE_IMPLEMENTATION
#include "argparse.h"

#define STDIN_FILENAME "<stdin>"

arena a;

int main(int argc, char *argv[]) {
    argp_init(argc, argv, "An interpreted scripting language!", /* default_help */ true);

    bool *lex = argp_flag_bool("l", "lex", "print lexer output then exit");
    bool *ast = argp_flag_bool("a", "ast", "print ast then exit");
    bool *repl = argp_flag_bool("r", "repl", "start interactive repl");
    bool *verbose = argp_flag_bool("V", "verbose", "verbose mode");

    char **file = argp_pos_str("file", "", ARGP_OPT_OPTIONAL, "File to interpret, use '-' for stdin or repl when '-r' is specified to supply arguments");
    Argp_List *args = argp_pos_list("args", ARGP_OPT_OPTIONAL, "Arguments for program");

    if (!argp_parse_args()) {
        argp_print_error(stderr);
        return 1;
    }

    glorp_options options = {
        .file = *file,
        .args = args,
        .lex = *lex,
        .ast = *ast,
        .repl = *repl,
        .verbose = *verbose,
    };

    options.repl |= options.file[0] == 0;

    if (options.repl) {
        start_repl(&options);
        return 0;
    }

    char *input;
    if (strcmp(options.file, "-") == 0) {
        input = read_stdin();
    } else {
        input = read_file(options.file);
    }

    interpret(input, &options);

    free(input);
    argp_free_list(args);

    return 0;
}
