#include <stdio.h>
#include <string.h>

#include "evaluator.h"
#include "glorp_options.h"
#include "lexer.h"
#include "parser.h"

#define BUF_SIZE 1024

#define PROMPT ">> "
#define QUIT ":q"
#define REPL_FILENAME "interactive"

void start_repl(glorp_options *options) {
    printf("Welcome to glorp!\n");

    char in[BUF_SIZE] = {0};

    lexer l;

    arena a;
    arena_init(&a);

    parser p = {.a = &a};

    expression_reference program;

    environment env;
    environment_init(&env, p.a, NULL);

    for (;;) {
        memset(in, 0, sizeof(in));
        arena_clear_expressions(p.a);

        printf(PROMPT);
        fgets(in, sizeof(in), stdin);

        if (strncmp(in, QUIT, sizeof(QUIT) - 1) == 0) {
            break;
        }

        lexer_init(&l, REPL_FILENAME, in, sizeof(in));

        if (options->flags & LEX_FLAG) {
            print_lexer_output(REPL_FILENAME, in, sizeof(in));
        }
        if (options->flags & ONLY_LEX_FLAG) continue;

        parser_reset_lexer(&p, &l);

        program = parse_program(&p);

        if (options->flags & AST_FLAG) {
            print_ast(p.a, program);
        }
        if (options->flags & ONLY_AST_FLAG) continue;

        object obj = eval(program, &env);
        inspect(&obj);

#ifdef DEBUG
        print_debug_info(p.a);
#endif  // DEBUG
    }
}
