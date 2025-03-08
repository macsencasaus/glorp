#include "interpreter.h"

#include <string.h>

#include "lexer.h"
#include "parser.h"

void interpret(const char *input, glorp_options *options) {
    const char *filename = options->filename;
    const size_t n = strlen(input);

    lexer l;
    lexer_init(&l, filename, input, n);

    if (options->flags & LEX_FLAG) {
        print_lexer_output(filename, input, n);
    }
    if (options->flags & ONLY_LEX_FLAG) return;

    arena a;
    arena_init(&a);

    parser p;
    parser_init(&p, &l, &a);

    expression_reference program = parse_program(&p);

    if (options->flags & AST_FLAG) {
        print_ast(p.a, program);
    }
    if (options->flags & ONLY_AST_FLAG) return;

#ifdef DEBUG
    print_debug_info(p.a);
#endif  // DEBUG
}
