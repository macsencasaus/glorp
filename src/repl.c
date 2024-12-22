#include "glorp_options.h"

#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>

#define BUF_SIZE 1024

#define PROMPT ">> "
#define QUIT ":q"
#define REPL_FILENAME "interactive"

void start_repl(glorp_options *options) {
    printf("Welcome to glorp!\n");

    char in[BUF_SIZE] = {0};
    char out[BUF_SIZE] = {0};
    
    lexer l;
    arena a = new_arena();
    parser p = { .a = a };
    expression_reference program;

    // printing
    token tok;
    lexer l2;

    for (;;) {

        printf(PROMPT);
        fgets(in, sizeof(in), stdin);

        if (strncmp(in, QUIT, 2) == 0) {
            break;
        }

        l = new_lexer(REPL_FILENAME, in, strlen(in));

        if (options->flags & LEX_FLAG) {
            l2 = new_lexer(REPL_FILENAME, in, strlen(in));
            tok = lexer_next_token(&l2);
            while (tok.type != TOKEN_TYPE_EOF) {
                strncpy(out, tok.literal, tok.length);
                printf("TOKEN type: %-10s literal: %-10s length: %lu\n",
                       token_type_literals[tok.type],
                       out,
                       tok.length
                );
                
                tok = lexer_next_token(&l2);
                memset(out, 0, sizeof(out));
            }
        }

        reset_parser(&p, &l);

        program = parse_program(&p);

        if (options->flags & AST_FLAG) {
            print_ast(&a, program);
        }

        memset(in, 0, sizeof(in));
    }
}
