#include "parser.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "error.h"

// whether the parser is allowed to parse these infix expressions
#define COLON_FLAG (1 << 0)
#define TUPLE_FLAG (1 << 1)

static expression_reference parse_expression(parser *p,
                                             expression_precedence precedence);

static inline void next_token(parser *p);
static inline bool cur_token_is(parser *p, token_type tt);
static inline bool peek_token_is(parser *p, token_type tt);
static inline void expect_peek(parser *p, token_type tt);
static inline void expect_cur(parser *p, token_type tt);
static inline void no_prefix_parse_fn_error(parser *p, token tok);
static inline expression_precedence peek_precedence(parser *p);

static prefix_parse_fn parse_identifier;
static prefix_parse_fn parse_int_literal;
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

    parse_infix_expression,  // LT
    parse_infix_expression,  // GT
    parse_infix_expression,  // LT EQ
    parse_infix_expression,  // GT EQ
    parse_infix_expression,  // EQ
    parse_infix_expression,  // NOT EQ

    parse_infix_expression,   // COMMA
    parse_infix_expression,   // COLON
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
    parse_infix_expression,    // RIGHT ARROW
};

void parser_init(parser *p, lexer *l, arena *a) {
    *p = (parser){
        .a = a,
        .l = l,
    };
    next_token(p);
    next_token(p);
}

void parser_reset_lexer(parser *p, lexer *l) {
    p->l = l;
    next_token(p);
    next_token(p);
}

expression_reference parse_program(parser *p) {
    expression_reference program_ref =
        arena_alloc_expression(p->a, (expression){0});
    expression *program = get_expression(p->a, program_ref);

    expression_list el = new_expression_list(p->a);

    while (!cur_token_is(p, TOKEN_TYPE_EOF)) {
        el_append(&el, parse_expression(p, PRECEDENCE_LOWEST));
        if (peek_token_is(p, TOKEN_TYPE_SEMICOLON)) {
            next_token(p);
        }
        next_token(p);
    }

    *program = new_expression(EXP_TYPE_PROGRAM, (token){0});
    program->program.expressions = el;

    return program_ref;
}

static expression_reference parse_expression(parser *p,
                                             expression_precedence precedence) {
    prefix_parse_fn *prefix = prefix_parse_fns[p->cur_token.type];

    if (prefix == NULL) {
        no_prefix_parse_fn_error(p, p->cur_token);
    }

    expression_reference left = prefix(p);

    expression_associativity assoc = assoc_lookup[p->peek_token.type];
    expression_precedence peek_prec = peek_precedence(p);

    bool consume_cond = (assoc == ASSOC_LEFT && peek_prec > precedence) ||
                        (assoc == ASSOC_RIGHT && peek_prec >= precedence);
    while (consume_cond) {
        infix_parse_fn *infix = infix_parse_fns[p->peek_token.type];

        if (infix == NULL) {
            return left;
        }

        next_token(p);
        left = infix(p, left);

        expression_precedence peek_prec = peek_precedence(p);
        consume_cond = (assoc == ASSOC_LEFT && peek_prec > precedence) ||
                       (assoc == ASSOC_RIGHT && peek_prec >= precedence);
    }

    return left;
}

static expression_reference parse_identifier(parser *p) {
    expression identifier = new_expression(EXP_TYPE_IDENTIFIER, p->cur_token);
    token tok = p->cur_token;

    identifier.identifier.value = tok.literal;
    identifier.identifier.length = tok.length;

    return arena_alloc_expression(p->a, identifier);
}

static expression_reference parse_int_literal(parser *p) {
    expression int_literal = new_expression(EXP_TYPE_INT_LITERAL, p->cur_token);

    token tok = p->cur_token;
    int64_t value = 0;

    int64_t mod = 1;
    for (int64_t i = tok.length - 1; i >= 0; --i, mod *= 10) {
        value += (tok.literal[i] - '0') * mod;
    }

    int_literal.int_literal.value = value;

    return arena_alloc_expression(p->a, int_literal);
}

static expression_reference parse_list_literal(parser *p) {
    expression list_literal =
        new_expression(EXP_TYPE_LIST_LITERAL, p->cur_token);

    expression_list values = new_expression_list(p->a);

    next_token(p);

    while (!cur_token_is(p, TOKEN_TYPE_RBRACKET)) {
        parser_flags old_flags = p->flags;
        p->flags |= TUPLE_FLAG;
        el_append(&values, parse_expression(p, PRECEDENCE_LOWEST));
        p->flags = old_flags;

        if (peek_token_is(p, TOKEN_TYPE_RBRACKET)) {
            next_token(p);
            break;
        }

        expect_peek(p, TOKEN_TYPE_COMMA);
        next_token(p);
    }

    expect_cur(p, TOKEN_TYPE_RBRACKET);

    list_literal.list_literal.values = values;

    return arena_alloc_expression(p->a, list_literal);
}

static expression_reference parse_block_expression(parser *p) {
    parser_flags old_flags = p->flags;
    p->flags = 0;

    expression block_expression =
        new_expression(EXP_TYPE_BLOCK_EXPRESSION, p->cur_token);

    expression_list expressions = new_expression_list(p->a);

    next_token(p);
    while (!cur_token_is(p, TOKEN_TYPE_RBRACE)) {
        el_append(&expressions, parse_expression(p, PRECEDENCE_LOWEST));
        if (peek_token_is(p, TOKEN_TYPE_SEMICOLON)) {
            next_token(p);
        }
        next_token(p);
    }

    expect_cur(p, TOKEN_TYPE_RBRACE);

    block_expression.block_expression.expressions = expressions;

    p->flags = old_flags;
    return arena_alloc_expression(p->a, block_expression);
}

static expression_reference parse_prefix_expression(parser *p) {
    expression prefix_expression =
        new_expression(EXP_TYPE_PREFIX_EXPRESSION, p->cur_token);

    token op = p->cur_token;
    next_token(p);

    expression_reference right = parse_expression(p, PRECEDENCE_PREFIX);

    prefix_expression.prefix_expression.right = right;
    prefix_expression.prefix_expression.op = op;

    return arena_alloc_expression(p->a, prefix_expression);
}

static expression_reference parse_group_expression(parser *p) {
    parser_flags old_flags = p->flags;
    p->flags = 0;

    next_token(p);
    expression_reference exp = parse_expression(p, PRECEDENCE_LOWEST);
    expect_peek(p, TOKEN_TYPE_RPAREN);

    p->flags = old_flags;
    return exp;
}

static expression_reference parse_assign_expression(parser *p,
                                                    expression_reference left) {
    expression assign_expression =
        new_expression(EXP_TYPE_ASSIGN_EXPRESSION, p->cur_token);

    if (cur_token_is(p, TOKEN_TYPE_COLON_COLON)) {
        assign_expression.assign_expression.constant = true;
    }

    next_token(p);

    assign_expression.assign_expression.left = left;
    assign_expression.assign_expression.right =
        parse_expression(p, PRECEDENCE_ASSIGN);

    return arena_alloc_expression(p->a, assign_expression);
}

static expression_reference parse_infix_expression(parser *p,
                                                   expression_reference left) {
    expression infix_expression =
        new_expression(EXP_TYPE_INFIX_EXPRESSION, p->cur_token);

    token op = p->cur_token;

    next_token(p);

    infix_expression.infix_expression.op = op;
    infix_expression.infix_expression.left = left;

    expression_precedence precedence = precedence_lookup[op.type];
    infix_expression.infix_expression.right = parse_expression(p, precedence);

    return arena_alloc_expression(p->a, infix_expression);
}

static expression_reference parse_ternary_expression(
    parser *p, expression_reference left) {
    expression ternary_expression =
        new_expression(EXP_TYPE_TERNARY_EXPRESSION, p->cur_token);

    next_token(p);

    ternary_expression.ternary_expression.condition = left;

    parser_flags old_flags = p->flags;
    p->flags |= COLON_FLAG;
    ternary_expression.ternary_expression.consequence =
        parse_expression(p, PRECEDENCE_LOWEST);
    p->flags = old_flags;

    expect_peek(p, TOKEN_TYPE_COLON);

    next_token(p);

    ternary_expression.ternary_expression.alternative =
        parse_expression(p, PRECEDENCE_LOWEST);

    return arena_alloc_expression(p->a, ternary_expression);
}

static expression_reference parse_call_expression(parser *p,
                                                  expression_reference left) {
    expression call_expression =
        new_expression(EXP_TYPE_CALL_EXPRESSION, p->cur_token);
    expression_list arguments = new_expression_list(p->a);

    call_expression.call_expression.function = left;

    next_token(p);

    while (!cur_token_is(p, TOKEN_TYPE_RPAREN)) {
        el_append(&arguments, parse_expression(p, PRECEDENCE_LOWEST));

        if (peek_token_is(p, TOKEN_TYPE_RPAREN)) {
            next_token(p);
            break;
        }

        expect_peek(p, TOKEN_TYPE_COMMA);
        next_token(p);
    }

    expect_cur(p, TOKEN_TYPE_RPAREN);

    call_expression.call_expression.arguments = arguments;

    return arena_alloc_expression(p->a, call_expression);
}

static expression_reference parse_index_expression(parser *p,
                                                   expression_reference left) {
    expression index_expression =
        new_expression(EXP_TYPE_INDEX_EXPRESSION, p->cur_token);

    index_expression.index_expression.list = left;

    next_token(p);

    expression_reference index = parse_expression(p, PRECEDENCE_LOWEST);

    expect_peek(p, TOKEN_TYPE_RBRACKET);

    index_expression.index_expression.index = index;

    return arena_alloc_expression(p->a, index_expression);
}

static inline void next_token(parser *p) {
    p->cur_token = p->peek_token;
    p->peek_token = lexer_next_token(p->l);
}

static inline bool cur_token_is(parser *p, token_type tt) {
    return p->cur_token.type == tt;
}

static inline bool peek_token_is(parser *p, token_type tt) {
    return p->peek_token.type == tt;
}

static inline void expect_peek(parser *p, token_type tt) {
    match_token_type_error(p->l->filename, p->peek_token, tt);
    next_token(p);
}

static inline void expect_cur(parser *p, token_type tt) {
    match_token_type_error(p->l->filename, p->cur_token, tt);
}

static inline expression_precedence peek_precedence(parser *p) {
    if ((p->flags & COLON_FLAG) && (p->peek_token.type == TOKEN_TYPE_COLON)) {
        return PRECEDENCE_STOP;
    }
    if ((p->flags & TUPLE_FLAG) && (p->peek_token.type == TOKEN_TYPE_COMMA)) {
        return PRECEDENCE_STOP;
    }
    return precedence_lookup[p->peek_token.type];
}

static inline void no_prefix_parse_fn_error(parser *p, token tok) {
    unexpected_token_error(p->l->filename, tok);
    exit(1);
}
