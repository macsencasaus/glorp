#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 512
#define RED_ERROR "\033[0;31merror:\033[0m"

static expression_reference parse_expression(parser *p,
                                             expression_precedence precedence);

static void next_token(parser *p);
static inline bool cur_token_is(parser *p, token_type tt);
static inline bool peek_token_is(parser *p, token_type tt);
static void expect_peek(parser *p, token_type tt);
static inline expression_precedence peek_precedence(parser *p);

static inline void no_prefix_parse_fn_error(parser *p, token tok);
static void print_error_line(token tok);

static prefix_parse_fn parse_identifier;
static prefix_parse_fn parse_int_literal;
static prefix_parse_fn parse_function_literal;
static prefix_parse_fn parse_list_literal;

static prefix_parse_fn parse_block_expression;
static prefix_parse_fn parse_prefix_expression;
static prefix_parse_fn parse_group_expression;
static infix_parse_fn parse_assign_expression;
static infix_parse_fn parse_infix_expression;
static infix_parse_fn parse_ternary_expression;
static infix_parse_fn parse_call_expression;
static infix_parse_fn parse_index_expression;

static prefix_parse_fn *const prefix_parse_fns[TOKEN_TYPE_ENUM_LENGTH] = {
    NULL,  // ILLEGAL
    NULL,  // EOF

    parse_identifier,   // IDENT
    NULL,               // CHAR
    parse_int_literal,  // INT
    NULL,               // FLOAT
    NULL,               // STRING

    NULL,                     // ASSIGN
    NULL,                     // PLUS
    parse_prefix_expression,  // MINUS
    parse_prefix_expression,  // BANG
    NULL,                     // ASTERISK
    NULL,                     // SLASH
    NULL,                     // PERCENT

    parse_prefix_expression,  // PLUS PLUS
    parse_prefix_expression,  // MINUS MINUS

    parse_function_literal,  // BACK SLASH

    NULL,  // LT
    NULL,  // GT
    NULL,  // LT EQ
    NULL,  // GT EQ
    NULL,  // EQ
    NULL,  // NOT EQ

    NULL,  // COMMA
    NULL,  // COLON
    NULL,  // COLON COLON
    NULL,  // SEMICOLON
    NULL,  // DOT

    parse_group_expression,  // LPAREN
    NULL,                    // RPAREN
    parse_block_expression,  // LBRACE
    NULL,                    // RBRACE
    parse_list_literal,      // LBRACKET
    NULL,                    // RBRACKET

    NULL,  // QUESTION
    NULL,  // RIGHT ARROW
};

static infix_parse_fn *const infix_parse_fns[TOKEN_TYPE_ENUM_LENGTH] = {
    NULL,  // ILLEGAL
    NULL,  // EOF

    NULL,  // IDENT
    NULL,  // CHAR
    NULL,  // INT
    NULL,  // FLOAT
    NULL,  // STRING

    parse_assign_expression,  // ASSIGN
    parse_infix_expression,   // PLUS
    parse_infix_expression,   // MINUS
    NULL,                     // BANG
    parse_infix_expression,   // ASTERISK
    parse_infix_expression,   // SLASH
    parse_infix_expression,   // PERCENT

    parse_infix_expression,  // PLUS PLUS
    NULL,                    // MINUS MINUS

    NULL,  // BACK SLASH

    parse_infix_expression,  // LT
    parse_infix_expression,  // GT
    parse_infix_expression,  // LT EQ
    parse_infix_expression,  // GT EQ
    parse_infix_expression,  // EQ
    parse_infix_expression,  // NOT EQ

    NULL,                     // COMMA
    NULL,                     // COLON
    parse_assign_expression,  // COLON COLON
    NULL,                     // SEMICOLON
    parse_infix_expression,   // DOT

    parse_call_expression,   // LPAREN
    NULL,                    // RPAREN
    NULL,                    // LBRACE
    NULL,                    // RBRACE
    parse_index_expression,  // LBRACKET
    NULL,                    // RBRACKET

    parse_ternary_expression,  // QUESTION
    NULL,                      // RIGHT ARROW
};

parser new_parser(lexer *l) {
    parser p = {
        .a = new_arena(),
        .l = l,
    };
    next_token(&p);
    next_token(&p);
    return p;
}

void reset_parser(parser *p, lexer *l) {
    p->l = l;
    next_token(p);
    next_token(p);
}

expression_reference parse_program(parser *p) {
    expression_list el = new_expression_list();

    while (!cur_token_is(p, TOKEN_TYPE_EOF)) {
        el_append(&p->a, &el, parse_expression(p, PRECEDENCE_LOWEST));
        if (peek_token_is(p, TOKEN_TYPE_SEMICOLON)) {
            next_token(p);
        }
        next_token(p);
    }

    expression program = new_expression(EXP_TYPE_PROGRAM);
    program.program.expressions = el;
    return arena_alloc(&p->a, program);
}

static expression_reference parse_expression(parser *p,
                                             expression_precedence precedence) {
    prefix_parse_fn *prefix = prefix_parse_fns[p->cur_token.type];

    if (prefix == NULL) {
        no_prefix_parse_fn_error(p, p->cur_token);
    }

    expression_reference left = prefix(p);

    while (peek_precedence(p) > precedence) {
        infix_parse_fn *infix = infix_parse_fns[p->peek_token.type];

        if (infix == NULL) {
            return left;
        }

        next_token(p);
        left = infix(p, left);
    }

    return left;
}

static expression_reference parse_identifier(parser *p) {
    expression identifier = new_expression(EXP_TYPE_IDENTIFIER);
    token tok = p->cur_token;

    identifier.identifier.value = tok.literal;
    identifier.identifier.length = tok.length;

    return arena_alloc(&p->a, identifier);
}

static expression_reference parse_int_literal(parser *p) {
    expression int_literal = new_expression(EXP_TYPE_INT_LITERAL);

    token tok = p->cur_token;
    int64_t value = 0;

    int64_t mod = 1;
    for (int64_t i = tok.length - 1; i >= 0; --i, mod *= 10) {
        value += (tok.literal[i] - '0') * mod;
    }

    int_literal.int_literal.value = value;

    return arena_alloc(&p->a, int_literal);
}

static expression_reference parse_function_literal(parser *p) {
    expression function_literal = new_expression(EXP_TYPE_FUNCTION_LITERAL);

    expression_list arguments = new_expression_list();

    expect_peek(p, TOKEN_TYPE_LPAREN);
    next_token(p);

    token tok;
    expression_reference argument;
    expression *argument_exp;
    while (!cur_token_is(p, TOKEN_TYPE_RPAREN)) {
        tok = p->cur_token;
        argument = parse_expression(p, PRECEDENCE_LOWEST);

        argument_exp = get_expression(&p->a, argument);
        if (argument_exp->type != EXP_TYPE_IDENTIFIER) {
            fprintf(stderr,
                    "%s:%lu:%lu: %s expected identifier for function literal "
                    "arguments\n",
                    p->l->filename, tok.line_number, tok.col_number, RED_ERROR);
            print_error_line(tok);
            exit(1);
        }

        el_append(&p->a, &arguments, argument);

        if (peek_token_is(p, TOKEN_TYPE_RPAREN)) {
            next_token(p);
            break;
        }

        expect_peek(p, TOKEN_TYPE_COMMA);
        next_token(p);
    }

    expect_peek(p, TOKEN_TYPE_RIGHT_ARROW);
    next_token(p);

    expression_reference body = parse_expression(p, PRECEDENCE_LOWEST);

    function_literal.function_literal.arguments = arguments;
    function_literal.function_literal.body = body;

    return arena_alloc(&p->a, function_literal);
}

static expression_reference parse_list_literal(parser *p) {
    expression list_literal = new_expression(EXP_TYPE_LIST_LITERAL);

    expression_list values = new_expression_list();

    next_token(p);

    // TODO: get expect peek to work
    while (!cur_token_is(p, TOKEN_TYPE_RBRACKET)) {
        el_append(&p->a, &values, parse_expression(p, PRECEDENCE_LOWEST));

        if (peek_token_is(p, TOKEN_TYPE_RBRACKET)) {
            next_token(p);
            break;
        }

        expect_peek(p, TOKEN_TYPE_COMMA);
        next_token(p);
    }

    // expect_peek(p, TOKEN_TYPE_RBRACKET);

    list_literal.list_literal.values = values;

    return arena_alloc(&p->a, list_literal);
}

static expression_reference parse_block_expression(parser *p) {
    expression block_expression = new_expression(EXP_TYPE_BLOCK_EXPRESSION);

    expression_list expressions = new_expression_list();

    next_token(p);
    while (!cur_token_is(p, TOKEN_TYPE_RBRACE)) {
        el_append(&p->a, &expressions, parse_expression(p, PRECEDENCE_LOWEST));
        if (peek_token_is(p, TOKEN_TYPE_SEMICOLON)) {
            next_token(p);
        }
        next_token(p);
    }

    // expect_peek(p, TOKEN_TYPE_RBRACE);

    block_expression.block_expression.expressions = expressions;

    return arena_alloc(&p->a, block_expression);
}

static expression_reference parse_prefix_expression(parser *p) {
    expression prefix_expression = new_expression(EXP_TYPE_PREFIX_EXPRESSION);

    token op = p->cur_token;
    next_token(p);

    expression_reference right = parse_expression(p, PRECEDENCE_PREFIX);

    prefix_expression.prefix_expression.right = right;
    prefix_expression.prefix_expression.op = op;

    return arena_alloc(&p->a, prefix_expression);
}

static expression_reference parse_group_expression(parser *p) {
    next_token(p);

    expression_reference exp = parse_expression(p, PRECEDENCE_LOWEST);

    expect_peek(p, TOKEN_TYPE_RPAREN);

    return exp;
}

static expression_reference parse_assign_expression(parser *p,
                                                    expression_reference left) {
    expression assign_expression = new_expression(EXP_TYPE_ASSIGN_EXPRESSION);

    if (cur_token_is(p, TOKEN_TYPE_COLON_COLON)) {
        assign_expression.assign_expression.constant = true;
    }

    next_token(p);

    assign_expression.assign_expression.left = left;
    assign_expression.assign_expression.right =
        parse_expression(p, PRECEDENCE_ASSIGN);

    return arena_alloc(&p->a, assign_expression);
}

static expression_reference parse_infix_expression(parser *p,
                                                   expression_reference left) {
    expression infix_expression = new_expression(EXP_TYPE_INFIX_EXPRESSION);

    token op = p->cur_token;

    next_token(p);

    infix_expression.infix_expression.op = op;
    infix_expression.infix_expression.left = left;

    expression_precedence precedence = precedence_lookup[op.type];
    infix_expression.infix_expression.right = parse_expression(p, precedence);

    return arena_alloc(&p->a, infix_expression);
}

static expression_reference parse_ternary_expression(
    parser *p, expression_reference left) {
    expression ternary_expression = new_expression(EXP_TYPE_TERNARY_EXPRESSION);

    next_token(p);

    ternary_expression.ternary_expression.condition = left;
    ternary_expression.ternary_expression.consequence =
        parse_expression(p, PRECEDENCE_TERNARY);

    expect_peek(p, TOKEN_TYPE_COLON);

    next_token(p);

    ternary_expression.ternary_expression.alternative =
        parse_expression(p, PRECEDENCE_LOWEST);

    return arena_alloc(&p->a, ternary_expression);
}

static expression_reference parse_call_expression(parser *p,
                                                  expression_reference left) {
    expression call_expression = new_expression(EXP_TYPE_CALL_EXPRESSION);
    expression_list arguments = new_expression_list();

    call_expression.call_expression.function = left;

    next_token(p);

    while (!cur_token_is(p, TOKEN_TYPE_RPAREN)) {
        el_append(&p->a, &arguments, parse_expression(p, PRECEDENCE_LOWEST));

        if (peek_token_is(p, TOKEN_TYPE_RPAREN)) {
            next_token(p);
            break;
        }

        expect_peek(p, TOKEN_TYPE_COMMA);
        next_token(p);
    }

    // expect_peek(p, TOKEN_TYPE_RPAREN);

    call_expression.call_expression.arguments = arguments;

    return arena_alloc(&p->a, call_expression);
}

static expression_reference parse_index_expression(parser *p,
                                                   expression_reference left) {
    expression index_expression = new_expression(EXP_TYPE_INDEX_EXPRESSION);

    index_expression.index_expression.list = left;

    next_token(p);

    expression_reference index = parse_expression(p, PRECEDENCE_LOWEST);

    expect_peek(p, TOKEN_TYPE_RBRACKET);

    index_expression.index_expression.index = index;

    return arena_alloc(&p->a, index_expression);
}

static void next_token(parser *p) {
    p->cur_token = p->peek_token;
    p->peek_token = lexer_next_token(p->l);
}

static inline bool cur_token_is(parser *p, token_type tt) {
    return p->cur_token.type == tt;
}

static inline bool peek_token_is(parser *p, token_type tt) {
    return p->peek_token.type == tt;
}

static void expect_peek(parser *p, token_type tt) {
    if (peek_token_is(p, tt)) {
        next_token(p);
        return;
    }

    token peek_token = p->peek_token;

    fprintf(stderr, "%s:%lu:%lu: %s expected %s\n", p->l->filename,
            peek_token.line_number, peek_token.col_number, RED_ERROR,
            token_type_literals[tt]);

    print_error_line(peek_token);

    exit(1);
}

static inline expression_precedence peek_precedence(parser *p) {
    return precedence_lookup[p->peek_token.type];
}

static inline void no_prefix_parse_fn_error(parser *p, token tok) {
    fprintf(stderr, "%s:%lu:%lu: %s no prefix parse function for %s\n",
            p->l->filename, tok.line_number, tok.col_number, RED_ERROR,
            token_type_literals[tok.type]);
    print_error_line(tok);
    exit(1);
}

// TODO: fix error message
static void print_error_line(token tok) {
    const char *RED_START = "\033[0;31m";
    const char *RED_END = "\033[0m";

    char *line = tok.literal - tok.col_number + 1;

    size_t line_length;
    for (char *p = line; *p != '\n'; ++p) ++line_length;

    char line_buf[line_length + 1];

    line_buf[line_length] = 0;

    strncpy(line_buf, line, line_length);

    printf("%4lu | %s\n", tok.line_number, line_buf);
    printf("     | %s%*c%s\n", RED_START, (int)tok.col_number, '^', RED_END);
}
