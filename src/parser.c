#include "parser.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"

#define CHECK_PARSE(parse_res) \
    if ((parse_res) == NULL) return NULL

#define SET_ERR(_tok, _type, _expected)                          \
    parser_err = (parser_error) {                                \
        .tok = (_tok), .type = (_type), .expected = (_expected), \
    }

// whether the parser is allowed to parse these infix expressions
#define COLON_FLAG (1 << 0)
#define TUPLE_FLAG (1 << 1)
#define BOR_FLAG (1 << 2)

parser_error parser_err;

WARN_UNUSED_RESULT
static expr *parse_expression(parser *p, expression_precedence precedence);

static inline void next_token(parser *p);
static inline bool cur_token_is(parser *p, token_type tt);
static inline bool peek_token_is(parser *p, token_type tt);
static inline bool expect_peek(parser *p, token_type tt) WARN_UNUSED_RESULT;
static inline bool expect_cur(parser *p, token_type tt) WARN_UNUSED_RESULT;
static inline void no_prefix_parse_fn_error(parser *p);
static inline expression_precedence peek_precedence(parser *p);

static prefix_parse_fn parse_identifier;
static prefix_parse_fn parse_char_literal;
static prefix_parse_fn parse_int_literal;
static prefix_parse_fn parse_float_literal;
static prefix_parse_fn parse_string_literal;
static prefix_parse_fn parse_list_literal;

static prefix_parse_fn parse_block_expression;
static prefix_parse_fn parse_prefix_expression;
static prefix_parse_fn parse_group_expression;
static prefix_parse_fn parse_case_expression;
static prefix_parse_fn parse_import_expression;

static infix_parse_fn parse_infix_expression;
static infix_parse_fn parse_ternary_expression;
static infix_parse_fn parse_call_expression;
static infix_parse_fn parse_index_expression;

// clang-format off
static prefix_parse_fn *const prefix_parse_fns[TOKEN_TYPE_ENUM_LENGTH] = {
    [TOKEN_TYPE_ILLEGAL]         = NULL,
    [TOKEN_TYPE_EOF]             = NULL,

    [TOKEN_TYPE_IDENT]           = parse_identifier,
    [TOKEN_TYPE_CHAR]            = parse_char_literal,
    [TOKEN_TYPE_INT]             = parse_int_literal,
    [TOKEN_TYPE_FLOAT]           = parse_float_literal,
    [TOKEN_TYPE_STRING]          = parse_string_literal,

    [TOKEN_TYPE_ASSIGN]          = NULL,
    [TOKEN_TYPE_PLUS]            = parse_import_expression,
    [TOKEN_TYPE_MINUS]           = parse_prefix_expression,
    [TOKEN_TYPE_BANG]            = parse_prefix_expression,
    [TOKEN_TYPE_ASTERISK]        = NULL,
    [TOKEN_TYPE_SLASH]           = NULL,
    [TOKEN_TYPE_PERCENT]         = NULL,

    [TOKEN_TYPE_PLUS_PLUS]       = parse_prefix_expression,
    [TOKEN_TYPE_MINUS_MINUS]     = parse_prefix_expression,

    [TOKEN_TYPE_LT]              = NULL,
    [TOKEN_TYPE_GT]              = NULL,
    [TOKEN_TYPE_LT_EQ]           = NULL,
    [TOKEN_TYPE_GT_EQ]           = NULL,
    [TOKEN_TYPE_EQ]              = NULL,
    [TOKEN_TYPE_NOT_EQ]          = NULL,

    [TOKEN_TYPE_LAND]            = NULL,
    [TOKEN_TYPE_LOR]             = NULL,
    [TOKEN_TYPE_BAND]            = NULL,
    [TOKEN_TYPE_BOR]             = parse_case_expression,
    [TOKEN_TYPE_NOT]             = parse_prefix_expression,
    [TOKEN_TYPE_XOR]             = NULL,

    [TOKEN_TYPE_LEFT_SHIFT]      = NULL,
    [TOKEN_TYPE_RIGHT_SHIFT]     = NULL,

    [TOKEN_TYPE_LEFT_COMPOSE]    = NULL,
    [TOKEN_TYPE_RIGHT_COMPOSE]   = NULL,
    [TOKEN_TYPE_LEFT_PIPE]       = NULL,
    [TOKEN_TYPE_RIGHT_PIPE]      = NULL,

    [TOKEN_TYPE_COMMA]           = NULL,
    [TOKEN_TYPE_COLON]           = NULL,
    [TOKEN_TYPE_COLON_COLON]     = parse_prefix_expression,
    [TOKEN_TYPE_SEMICOLON]       = NULL,
    [TOKEN_TYPE_DOT]             = NULL,

    [TOKEN_TYPE_LPAREN]          = parse_group_expression,
    [TOKEN_TYPE_RPAREN]          = NULL,
    [TOKEN_TYPE_LBRACE]          = parse_block_expression,
    [TOKEN_TYPE_RBRACE]          = NULL,
    [TOKEN_TYPE_LBRACKET]        = parse_list_literal,
    [TOKEN_TYPE_RBRACKET]        = NULL,

    [TOKEN_TYPE_QUESTION]        = NULL,
    [TOKEN_TYPE_RIGHT_ARROW]     = NULL,
    [TOKEN_TYPE_LEFT_ARROW]      = NULL,
    [TOKEN_TYPE_FAT_RIGHT_ARROW] = NULL,
};
// clang-format on

// clang-format off
static infix_parse_fn *const infix_parse_fns[TOKEN_TYPE_ENUM_LENGTH] = {
    [TOKEN_TYPE_ILLEGAL]         = NULL,  
    [TOKEN_TYPE_EOF]             = NULL,  

    [TOKEN_TYPE_IDENT]           = NULL,  
    [TOKEN_TYPE_CHAR]            = NULL,  
    [TOKEN_TYPE_INT]             = NULL,  
    [TOKEN_TYPE_FLOAT]           = NULL,  
    [TOKEN_TYPE_STRING]          = NULL,  

    [TOKEN_TYPE_ASSIGN]          = parse_infix_expression,  
    [TOKEN_TYPE_PLUS]            = parse_infix_expression,   
    [TOKEN_TYPE_MINUS]           = parse_infix_expression,   
    [TOKEN_TYPE_BANG]            = NULL,                     
    [TOKEN_TYPE_ASTERISK]        = parse_infix_expression,   
    [TOKEN_TYPE_SLASH]           = parse_infix_expression,   
    [TOKEN_TYPE_PERCENT]         = parse_infix_expression,   

    [TOKEN_TYPE_PLUS_PLUS]       = NULL,  
    [TOKEN_TYPE_MINUS_MINUS]     = NULL,                    

    [TOKEN_TYPE_LT]              = parse_infix_expression,  
    [TOKEN_TYPE_GT]              = parse_infix_expression,  
    [TOKEN_TYPE_LT_EQ]           = parse_infix_expression,  
    [TOKEN_TYPE_GT_EQ]           = parse_infix_expression,  
    [TOKEN_TYPE_EQ]              = parse_infix_expression,  
    [TOKEN_TYPE_NOT_EQ]          = parse_infix_expression,  

    [TOKEN_TYPE_LAND]            = parse_infix_expression,
    [TOKEN_TYPE_LOR]             = parse_infix_expression,
    [TOKEN_TYPE_BAND]            = parse_infix_expression,
    [TOKEN_TYPE_BOR]             = parse_infix_expression,
    [TOKEN_TYPE_NOT]             = NULL,
    [TOKEN_TYPE_XOR]             = parse_infix_expression,

    [TOKEN_TYPE_LEFT_SHIFT]      = parse_infix_expression,
    [TOKEN_TYPE_RIGHT_SHIFT]     = parse_infix_expression,

    [TOKEN_TYPE_LEFT_COMPOSE]    = parse_infix_expression,
    [TOKEN_TYPE_RIGHT_COMPOSE]   = parse_infix_expression,
    [TOKEN_TYPE_LEFT_PIPE]       = parse_infix_expression,
    [TOKEN_TYPE_RIGHT_PIPE]      = parse_infix_expression,

    [TOKEN_TYPE_COMMA]           = parse_infix_expression,   
    [TOKEN_TYPE_COLON]           = parse_infix_expression,   
    [TOKEN_TYPE_COLON_COLON]     = parse_infix_expression,  
    [TOKEN_TYPE_SEMICOLON]       = NULL,                     
    [TOKEN_TYPE_DOT]             = parse_infix_expression,   

    [TOKEN_TYPE_LPAREN]          = parse_call_expression,   
    [TOKEN_TYPE_RPAREN]          = NULL,                    
    [TOKEN_TYPE_LBRACE]          = NULL,                    
    [TOKEN_TYPE_RBRACE]          = NULL,                    
    [TOKEN_TYPE_LBRACKET]        = parse_index_expression,  
    [TOKEN_TYPE_RBRACKET]        = NULL,                    

    [TOKEN_TYPE_QUESTION]        = parse_ternary_expression,  
    [TOKEN_TYPE_RIGHT_ARROW]     = parse_infix_expression,    
    [TOKEN_TYPE_LEFT_ARROW]      = NULL,
    [TOKEN_TYPE_FAT_RIGHT_ARROW] = NULL,
};
// clang-format on

void parser_init(parser *p, lexer *l) {
    *p = (parser){
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

expr *parse_program(parser *p) {
    expr *program = new_expr(EXPR_TYPE_PROGRAM, NULL);
    expr_list *el = &program->expressions;
    ;

    expr *program_expr;
    while (!cur_token_is(p, TOKEN_TYPE_EOF)) {
        program_expr = parse_expression(p, PRECEDENCE_LOWEST);
        CHECK_PARSE(program_expr);
        el_append(el, program_expr);

        if (peek_token_is(p, TOKEN_TYPE_SEMICOLON)) {
            next_token(p);
        }
        next_token(p);
    }
    return program;
}

static expr *parse_expression(parser *p, expression_precedence precedence) {
    prefix_parse_fn *prefix = prefix_parse_fns[p->cur_token.type];

    if (prefix == NULL) {
        no_prefix_parse_fn_error(p);
        return false;
    }

    expr *left;
    CHECK_PARSE(left = prefix(p));

#define CONSUME_COND                                        \
    (assoc_lookup[p->peek_token.type] == ASSOC_LEFT &&      \
     peek_precedence(p) > precedence) ||                    \
        (assoc_lookup[p->peek_token.type] == ASSOC_RIGHT && \
         peek_precedence(p) >= precedence)

    while (CONSUME_COND) {
        infix_parse_fn *infix = infix_parse_fns[p->peek_token.type];

        if (infix == NULL) {
            break;
        }

        next_token(p);
        CHECK_PARSE(left = infix(p, left));
    }

    return left;
}

static expr *parse_identifier(parser *p) {
    const token *tok = &p->cur_token;

    expr *identifier = new_expr2(EXPR_TYPE_IDENTIFIER, tok);

    identifier->literal = tok->literal;
    identifier->length = tok->length;

    return identifier;
}

static expr *parse_char_literal(parser *p) {
    expr *char_literal = new_expr2(EXPR_TYPE_CHAR_LITERAL, &p->cur_token);

    const char *c = p->cur_token.literal + 1;

    char *value = &char_literal->char_value;

    switch (*c) {
        case 0: {
            SET_ERR(p->cur_token, PARSER_ERROR_EXPECTED, TOKEN_TYPE_CHAR);
            return NULL;
        } break;
        case '\\': {
            ++c;
            if (*c == 'x') {
                // TODO: parse hex
            } else if (*c == 'b') {
                // TODO: parse binary
            } else if (isdigit(*c)) {
                // TODO: parse octal
            } else {
                switch (*c) {
                    case 'n':
                        *value = '\n';
                        break;
                    case 'r':
                        *value = '\r';
                        break;
                    case 't':
                        *value = '\t';
                        break;
                    case 'b':
                        *value = '\b';
                        break;
                    case 'f':
                        *value = '\f';
                        break;
                    case 'v':
                        *value = '\v';
                        break;
                    default: {
                        SET_ERR(p->cur_token, PARSER_ERROR_EXPECTED,
                                TOKEN_TYPE_CHAR);
                    }
                }
            }
        } break;
        default: {
            *value = *c;
            if (c[1] != '\'') {
                SET_ERR(p->cur_token, PARSER_ERROR_EXPECTED, TOKEN_TYPE_CHAR);
                return NULL;
            }
        }
    }

    ++c;
    if (*c != '\'') {
        SET_ERR(p->cur_token, PARSER_ERROR_EXPECTED, TOKEN_TYPE_CHAR);
        return false;
    }

    return char_literal;
}

static expr *parse_int_literal(parser *p) {
    expr *int_literal = new_expr2(EXPR_TYPE_INT_LITERAL, &p->cur_token);
    int_literal->int_value = atoll(p->cur_token.literal);
    return int_literal;
}

static expr *parse_float_literal(parser *p) {
    expr *float_literal = new_expr2(EXPR_TYPE_FLOAT_LITERAL, &p->cur_token);
    float_literal->float_value = atof(p->cur_token.literal);
    return float_literal;
}

static expr *parse_string_literal(parser *p) {
    expr *str_literal = new_expr2(EXPR_TYPE_STRING_LITERAL, &p->cur_token);

    str_literal->literal = p->cur_token.literal + 1;
    str_literal->length = p->cur_token.length - 2;

    return str_literal;
}

static expr *parse_list_literal(parser *p) {
    expr *list_literal = new_expr(EXPR_TYPE_LIST_LITERAL, &p->cur_token);

    expr_list *values = &list_literal->expressions;

    next_token(p);

    expr *list_elem;
    while (!cur_token_is(p, TOKEN_TYPE_RBRACKET)) {
        parser_flags old_flags = p->flags;
        p->flags |= TUPLE_FLAG;
        CHECK_PARSE(list_elem = parse_expression(p, PRECEDENCE_LOWEST));
        el_append(values, list_elem);
        p->flags = old_flags;

        if (peek_token_is(p, TOKEN_TYPE_RBRACKET)) {
            next_token(p);
            break;
        }

        if (!expect_peek(p, TOKEN_TYPE_COMMA)) {
            return NULL;
        }
        next_token(p);
    }

    if (!expect_cur(p, TOKEN_TYPE_RBRACKET)) {
        return NULL;
    }

    list_literal->end_tok = p->cur_token;

    return list_literal;
}

static expr *parse_block_expression(parser *p) {
    parser_flags old_flags = p->flags;
    p->flags = 0;

    expr *block_expr = new_expr(EXPR_TYPE_BLOCK_EXPRESSION, &p->cur_token);

    expr_list *expressions = &block_expr->expressions;

    next_token(p);

    expr *elem_expr;
    while (!cur_token_is(p, TOKEN_TYPE_RBRACE)) {
        CHECK_PARSE(elem_expr = parse_expression(p, PRECEDENCE_LOWEST));
        el_append(expressions, elem_expr);
        if (peek_token_is(p, TOKEN_TYPE_SEMICOLON)) {
            next_token(p);
        }
        next_token(p);
    }

    if (!expect_cur(p, TOKEN_TYPE_RBRACE)) {
        return NULL;
    }

    block_expr->end_tok = p->cur_token;

    p->flags = old_flags;

    return block_expr;
}

static expr *parse_prefix_expression(parser *p) {
    expr *prefix_expr = new_expr(EXPR_TYPE_PREFIX_EXPRESSION, &p->cur_token);

    next_token(p);

    expr *right;
    CHECK_PARSE(right = parse_expression(p, PRECEDENCE_PREFIX));

    prefix_expr->right = right;
    prefix_expr->op = prefix_expr->start_tok;
    prefix_expr->end_tok = right->end_tok;

    return prefix_expr;
}

static expr *parse_group_expression(parser *p) {
    if (peek_token_is(p, TOKEN_TYPE_RPAREN)) {
        expr *e = new_expr(EXPR_TYPE_UNIT, &p->cur_token);
        next_token(p);
        e->end_tok = p->cur_token;
        return e;
    }

    token start_tok = p->cur_token;

    parser_flags old_flags = p->flags;
    p->flags = 0;

    next_token(p);

    expr *e;
    CHECK_PARSE(e = parse_expression(p, PRECEDENCE_LOWEST));

    if (!expect_peek(p, TOKEN_TYPE_RPAREN)) {
        return NULL;
    }

    e->start_tok = start_tok;
    e->end_tok = p->cur_token;

    p->flags = old_flags;

    return e;
}

static expr *parse_case_expression(parser *p) {
    expr *e = new_expr(EXPR_TYPE_CASE_EXPRESSION, &p->cur_token);

    expr *condition, *result;
    while (cur_token_is(p, TOKEN_TYPE_BOR)) {
        next_token(p);
        CHECK_PARSE(condition = parse_expression(p, PRECEDENCE_LOWEST));

        el_append(&e->conditions, condition);

        if (!expect_peek(p, TOKEN_TYPE_FAT_RIGHT_ARROW)) {
            return NULL;
        }

        next_token(p);

        parser_flags old_flags = p->flags;
        p->flags |= BOR_FLAG;

        CHECK_PARSE(result = parse_expression(p, PRECEDENCE_LOWEST));

        p->flags = old_flags;

        el_append(&e->results, result);

        if (peek_token_is(p, TOKEN_TYPE_SEMICOLON))
            next_token(p);

        if (peek_token_is(p, TOKEN_TYPE_BOR))
            next_token(p);
    }

    e->end_tok = p->cur_token;
    return e;
}

static expr *parse_import_expression(parser *p) {
    expr *e = new_expr(EXPR_TYPE_IMPORT_EXPRESSION, &p->cur_token);
    
    if (!expect_peek(p, TOKEN_TYPE_STRING)) {
        return NULL;
    }

    e->literal = p->cur_token.literal + 1;
    e->length = p->cur_token.length - 2;

    e->end_tok = p->cur_token;
    
    return e;
}

static expr *parse_infix_expression(parser *p, expr *left) {
    const token *tok = &p->cur_token;
    expression_precedence precedence = precedence_lookup[tok->type];

    expr *infix_expr = new_expr(EXPR_TYPE_INFIX_EXPRESSION, &left->start_tok);
    infix_expr->op = *tok;
    infix_expr->left = left;

    next_token(p);

    expr *right;
    CHECK_PARSE(right = parse_expression(p, precedence));

    infix_expr->right = right;
    infix_expr->end_tok = right->end_tok;

    return infix_expr;
}

static expr *parse_ternary_expression(parser *p, expr *left) {
    expr *ternary_expr = new_expr(EXPR_TYPE_TERNARY_EXPRESSION, &left->start_tok);

    next_token(p);

    ternary_expr->condition = left;

    parser_flags old_flags = p->flags;
    p->flags |= COLON_FLAG;

    expr *consequence;
    CHECK_PARSE(consequence = parse_expression(p, PRECEDENCE_LOWEST));
    ternary_expr->consequence = consequence;

    p->flags = old_flags;

    if (!expect_peek(p, TOKEN_TYPE_COLON)) {
        return NULL;
    }

    next_token(p);

    expr *alternative;
    CHECK_PARSE(alternative = parse_expression(p, PRECEDENCE_LOWEST));
    ternary_expr->alternative = alternative;

    ternary_expr->end_tok = alternative->end_tok;

    return ternary_expr;
}

static expr *parse_call_expression(parser *p, expr *left) {
    expr *call_expr = new_expr(EXPR_TYPE_CALL_EXPRESSION, &left->start_tok);

    expr_list *params = &call_expr->params;

    call_expr->function = left;

    next_token(p);

    expr *param;
    while (!cur_token_is(p, TOKEN_TYPE_RPAREN)) {
        parser_flags old_flags = p->flags;
        p->flags |= TUPLE_FLAG;
        CHECK_PARSE(param = parse_expression(p, PRECEDENCE_LOWEST));
        el_append(params, param);
        p->flags = old_flags;

        if (peek_token_is(p, TOKEN_TYPE_RPAREN)) {
            next_token(p);
            break;
        }

        if (!expect_peek(p, TOKEN_TYPE_COMMA)) {
            return NULL;
        }
        next_token(p);
    }

    if (!expect_cur(p, TOKEN_TYPE_RPAREN)) {
        return NULL;
    }

    call_expr->end_tok = p->cur_token;

    return call_expr;
}

static expr *parse_index_expression(parser *p, expr *left) {
    expr *index_expr = new_expr(EXPR_TYPE_INDEX_EXPRESSION, &left->start_tok);

    index_expr->list = left;

    next_token(p);

    expr *index;
    CHECK_PARSE(index = parse_expression(p, PRECEDENCE_LOWEST));

    if (!expect_peek(p, TOKEN_TYPE_RBRACKET)) {
        return NULL;
    }

    index_expr->index = index;

    index_expr->end_tok = p->cur_token;

    return index_expr;
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

static inline bool expect_peek(parser *p, token_type tt) {
    if (p->peek_token.type != tt) {
        parser_err = (parser_error){
            .tok = p->peek_token,
            .type = PARSER_ERROR_EXPECTED,
            .expected = tt,
        };
        return false;
    }
    next_token(p);
    return true;
}

static inline bool expect_cur(parser *p, token_type tt) {
    if (p->cur_token.type != tt) {
        parser_err = (parser_error){
            .tok = p->cur_token,
            .type = PARSER_ERROR_EXPECTED,
            .expected = tt,
        };
        return false;
    }
    return true;
}

static inline expression_precedence peek_precedence(parser *p) {
    if ((p->flags & COLON_FLAG) && (p->peek_token.type == TOKEN_TYPE_COLON)) {
        return PRECEDENCE_STOP;
    }
    if ((p->flags & TUPLE_FLAG) && (p->peek_token.type == TOKEN_TYPE_COMMA)) {
        return PRECEDENCE_STOP;
    }
    if ((p->flags & BOR_FLAG) && (p->peek_token.type == TOKEN_TYPE_BOR)) {
        return PRECEDENCE_STOP;
    }
    return precedence_lookup[p->peek_token.type];
}

static inline void no_prefix_parse_fn_error(parser *p) {
    parser_err = (parser_error){
        .tok = p->cur_token,
        .type = PARSER_ERROR_UNEXPECTED,
    };
}
