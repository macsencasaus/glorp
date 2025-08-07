#include "interpreter.h"

#include <string.h>

#include "arena.h"
#include "environment.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"

extern arena a;

extern eval_error eval_err;
extern parser_error parser_err;

#define BUF_SIZE 1024

void interpret(const char *input, const glorp_options *selected_options) {
    const char *filename = selected_options->file_name;
    const size_t n = strlen(input);

    lexer l;
    lexer_init(&l, filename, input, n);

    if (selected_options->flags & LEX_FLAG) {
        print_lexer_output(filename, input, n);
    }
    if (selected_options->flags & ONLY_LEX_FLAG) return;

    arena_init(&a);

    parser p;
    parser_init(&p, &l);

    expr *program = parse_program(&p);
    if (program == NULL) {
        inspect_parser_error(filename, &parser_err);
        arena_destroy(&a);
        return;
    }

    if (selected_options->flags & AST_FLAG) {
        print_ast(program);
    }
    if (selected_options->flags & ONLY_AST_FLAG) {
        if (selected_options->flags & VERBOSE_FLAG)
            arena_print_exprs();
        return;
    }

    hash_table ht;
    ht_init(&ht);

    environment env;
    environment_init(&env, NULL, &ht, 0);
    env.selected_options = selected_options;

    add_cmdline_args(selected_options->args, selected_options->argc, &env);
    add_builtins(&env);

    object obj;
    if (eval(program, &env, &obj)) {
        temp_cleanup(&obj);
    } else {
        inspect_eval_error(filename, &eval_err);
    }

    if (selected_options->flags & VERBOSE_FLAG) {
        print_debug_info();
        print_ht_info(&ht);
    }

    arena_destroy(&a);
    ht_destroy(&ht);
}

bool interpret_with_env(const char *input, const glorp_options *selected_options, 
                        environment *env) {
    const char *filename = selected_options->file_name;
    const size_t n = strlen(input);

    lexer l;
    lexer_init(&l, filename, input, n);

    parser p;
    parser_init(&p, &l);

    expr *program = parse_program(&p);
    if (program == NULL) {
        inspect_parser_error(filename, &parser_err);
        return false;
    }
    object obj;
    if (eval(program, env, &obj)) {
        temp_cleanup(&obj);
    } else {
        inspect_eval_error(filename, &eval_err);
        return false;
    }

    return true;
}
