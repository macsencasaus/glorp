#ifndef PARSER_HELPER_H
#define PARSER_HELPER_H

typedef expression_reference prefix_parse_fn(parser *p);
typedef expression_reference infix_parse_fn(parser *p, expression_reference left);

typedef enum {
    PRECEDENCE_LOWEST,
    PRECEDENCE_TERNARY,
    PRECEDENCE_ASSIGN,
    PRECEDENCE_EQUALS,
    PRECEDENCE_APPEND,
    PRECEDENCE_SUM,
    PRECEDENCE_PRODUCT,
    PRECEDENCE_PREFIX,
    PRECEDENCE_INDEX,
    PRECEDENCE_COMPOSE,
    PRECEDENCE_CALL,
} expression_precedence;

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

static const expression_precedence precedence_lookup[TOKEN_TYPE_ENUM_LENGTH] = {
    PRECEDENCE_LOWEST,      // ILLEGAL
    PRECEDENCE_LOWEST,      // EOF

    PRECEDENCE_LOWEST,      // IDENT
    PRECEDENCE_LOWEST,      // CHAR
    PRECEDENCE_LOWEST,      // INT
    PRECEDENCE_LOWEST,      // FLOAT
    PRECEDENCE_LOWEST,      // STRING

    PRECEDENCE_ASSIGN,      // ASSIGN
    PRECEDENCE_SUM,         // PLUS
    PRECEDENCE_SUM,         // MINUS
    PRECEDENCE_LOWEST,      // BANG
    PRECEDENCE_PRODUCT,     // ASTERISK
    PRECEDENCE_PRODUCT,     // SLASH
    PRECEDENCE_PRODUCT,     // PERCENT

    PRECEDENCE_APPEND,      // PLUS PLUS
    PRECEDENCE_LOWEST,      // MINUS MINUS

    PRECEDENCE_LOWEST,      // BACK SLASH

    PRECEDENCE_EQUALS,      // LT
    PRECEDENCE_EQUALS,      // GT
    PRECEDENCE_EQUALS,      // LT EQ
    PRECEDENCE_EQUALS,      // GT EQ
    PRECEDENCE_EQUALS,      // EQ
    PRECEDENCE_EQUALS,      // NOT EQ

    PRECEDENCE_LOWEST,      // COMMA
    PRECEDENCE_LOWEST,      // COLON
    PRECEDENCE_ASSIGN,      // COLON COLON
    PRECEDENCE_LOWEST,      // SEMICOLON
    PRECEDENCE_COMPOSE,     // DOT

    PRECEDENCE_CALL,        // LPAREN
    PRECEDENCE_LOWEST,      // RPAREN
    PRECEDENCE_LOWEST,      // LBRACE
    PRECEDENCE_LOWEST,      // RBRACE
    PRECEDENCE_INDEX,       // LBRACKET
    PRECEDENCE_LOWEST,      // RBRACKET

    PRECEDENCE_TERNARY,     // QUESTION
    PRECEDENCE_LOWEST,      // RIGHT ARROW
};

static prefix_parse_fn *const prefix_parse_fns[TOKEN_TYPE_ENUM_LENGTH] = {
    NULL,                       // ILLEGAL 
    NULL,                       // EOF
    
    parse_identifier,           // IDENT
    NULL,                       // CHAR
    parse_int_literal,          // INT
    NULL,                       // FLOAT
    NULL,                       // STRING
    
    NULL,                       // ASSIGN
    NULL,                       // PLUS
    parse_prefix_expression,    // MINUS
    parse_prefix_expression,    // BANG
    NULL,                       // ASTERISK
    NULL,                       // SLASH
    NULL,                       // PERCENT

    parse_prefix_expression,    // PLUS PLUS
    parse_prefix_expression,    // MINUS MINUS

    parse_function_literal,     // BACK SLASH

    NULL,                       // LT
    NULL,                       // GT
    NULL,                       // LT EQ
    NULL,                       // GT EQ
    NULL,                       // EQ
    NULL,                       // NOT EQ

    NULL,                       // COMMA
    NULL,                       // COLON
    NULL,                       // COLON COLON
    NULL,                       // SEMICOLON
    NULL,                       // DOT

    parse_group_expression,     // LPAREN
    NULL,                       // RPAREN
    parse_block_expression,     // LBRACE
    NULL,                       // RBRACE
    parse_list_literal,         // LBRACKET
    NULL,                       // RBRACKET

    NULL,                       // QUESTION
    NULL,                       // RIGHT ARROW
};

static infix_parse_fn *const infix_parse_fns[TOKEN_TYPE_ENUM_LENGTH] = {
    NULL,                       // ILLEGAL
    NULL,                       // EOF

    NULL,                       // IDENT
    NULL,                       // CHAR
    NULL,                       // INT
    NULL,                       // FLOAT
    NULL,                       // STRING

    parse_assign_expression,    // ASSIGN
    parse_infix_expression,     // PLUS
    parse_infix_expression,     // MINUS
    NULL,                       // BANG
    parse_infix_expression,     // ASTERISK
    parse_infix_expression,     // SLASH
    parse_infix_expression,     // PERCENT

    parse_infix_expression,     // PLUS PLUS
    NULL,                       // MINUS MINUS

    NULL,                       // BACK SLASH

    parse_infix_expression,     // LT
    parse_infix_expression,     // GT
    parse_infix_expression,     // LT EQ
    parse_infix_expression,     // GT EQ
    parse_infix_expression,     // EQ
    parse_infix_expression,     // NOT EQ

    NULL,                       // COMMA
    NULL,                       // COLON
    parse_assign_expression,    // COLON COLON
    NULL,                       // SEMICOLON
    parse_infix_expression,     // DOT

    parse_call_expression,      // LPAREN
    NULL,                       // RPAREN
    NULL,                       // LBRACE
    NULL,                       // RBRACE
    parse_index_expression,     // LBRACKET
    NULL,                       // RBRACKET
    
    parse_ternary_expression,   // QUESTION
    NULL,                       // RIGHT ARROW
};

#endif // PARSER_HELPER_H
