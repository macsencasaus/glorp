#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "bumpalloc.h"
#include "evaluator.h"
#include "glorpoptions.h"
#include "lexer.h"
#include "parser.h"
#include "readline/history.h"
#include "readline/readline.h"

#define SB_IMPLEMENTATION
#include "sb.h"

#define BUF_SIZE 1024
#define MAX_HISTORY_DEPTH 64

#define PROMPT ">> "
#define DOT_PROMPT ".. "

#define QUIT ":q"
#define REPL_FILENAME "<interactive>"

extern arena a;

extern eval_error eval_err;
extern parser_error parser_err;

static inline bool parser_err_is_unexpected_eof(void) {
    return parser_err.tok.type == TOKEN_TYPE_EOF && parser_err.type == PARSER_ERROR_UNEXPECTED;
}

void start_repl(glorp_options *options) {
    printf("Welcome to glorp!\n");

    rl_bind_key('\t', rl_insert);

    lexer l;

    arena_init(&a);

    parser p = {0};

    expr *program;

    hash_table ht;
    ht_init(&ht);

    environment env;
    environment_init(&env, NULL, &ht, 0);
    env.selected_options = options;

    add_cmdline_args(options->args->items, options->args->size, &env);
    add_builtins(&env);

    size_t line_number = 0;

    bump_alloc line_alloc;
    ba_init(&line_alloc);

    String_Builder in = {0};
    String_Builder out = {0};

    char *cur_prompt = PROMPT;

    for (;;) {
        ++line_number;
        sb_reset(&out);

        char *line = readline(cur_prompt);

        if (line == NULL) {
            printf("Leaving glorp.\n");
            return;
        }

        size_t line_len = strlen(line);
        sb_append_buf(&in, line, line_len);

        if (wait_for_more(line, line_len)) {
            cur_prompt = DOT_PROMPT;
            continue;
        }

        cur_prompt = PROMPT;
        if (in.store == NULL) {
            continue;
        }

        free(line);

        sb_append_null(&in);
        add_history(in.store);

        size_t n = in.size;
        char *cur_in = (char *)ba_malloc(&line_alloc, n);
        memcpy(cur_in, in.store, in.size);

        lexer_init(&l, REPL_FILENAME, cur_in, n);
        l.line_number = line_number;

        if (options->lex) {
            print_lexer_output(&l);
            continue;
        }

        parser_reset_lexer(&p, &l);

        program = parse_program(&p);
        if (program == NULL) {
            if (parser_err_is_unexpected_eof() && line_len > 0) {
                // TODO: free old program

                sb_pop_last(&in);
                ba_free(&line_alloc, n);
                cur_prompt = DOT_PROMPT;

                continue;
            }
            sb_reset(&in);
            inspect_parser_error(REPL_FILENAME, &parser_err);
            continue;
        }

        sb_reset(&in);

        if (options->verbose)
            arena_print_exprs();

        if (options->ast) {
            print_ast(program);
            continue;
        }

        if (program->expressions.size == 0)
            continue;

        object obj;
        if (eval(program, &env, &obj)) {
            if (obj.type != OBJECT_TYPE_UNIT) {
                inspect(&obj, &out, false);
                printf("%.*s\n", (int)out.size, out.store);
            }

            temp_cleanup(&obj);
        } else {
            inspect_eval_error(REPL_FILENAME, &eval_err);
        }

        if (options->verbose) {
            print_debug_info();
            print_ht_info(&ht);
        }
    }

    clear_history();
    sb_free(&in);
    ba_destroy(&line_alloc);
    arena_destroy(&a);
    ht_destroy(&ht);
}
