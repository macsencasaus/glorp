#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"

typedef uint8_t parser_flags;

typedef struct {
    lexer *l;

    token cur_token;
    token peek_token;

    parser_flags flags;
} parser;

typedef expr *prefix_parse_fn(parser *p);
typedef expr *infix_parse_fn(parser *p, expr *left);

typedef enum {
    PRECEDENCE_STOP,
    PRECEDENCE_LOWEST,
    PRECEDENCE_ASSIGN,
    PRECEDENCE_PIPE,
    PRECEDENCE_FUNCTION,
    PRECEDENCE_TUPLE,
    PRECEDENCE_TERNARY,
    PRECEDENCE_LOR,
    PRECEDENCE_LAND,
    PRECEDENCE_BOR,
    PRECEDENCE_XOR,
    PRECEDENCE_BAND,
    PRECEDENCE_EQUALS,
    PRECEDENCE_SHIFT,
    PRECEDENCE_APPEND,
    PRECEDENCE_SUM,
    PRECEDENCE_PRODUCT,
    PRECEDENCE_PREFIX,
    PRECEDENCE_INDEX,
    PRECEDENCE_COMPOSE,
    PRECEDENCE_CALL,
    PRECEDENCE_FIELD,
} expression_precedence;

typedef enum {
    ASSOC_NONE,
    ASSOC_LEFT,
    ASSOC_RIGHT,
} expression_associativity;

void parser_init(parser *p, lexer *l);

// for repl (reuses same arena given a different lexer)
void parser_reset_lexer(parser *p, lexer *l);

WARN_UNUSED_RESULT
expr *parse_program(parser *p);

// clang-format off
static const expression_precedence precedence_lookup[TOKEN_TYPE_ENUM_LENGTH] = {
    [TOKEN_TYPE_ILLEGAL]         = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_EOF]             = PRECEDENCE_LOWEST,  
                                  
    [TOKEN_TYPE_IDENT]           = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_CHAR]            = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_INT]             = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_FLOAT]           = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_STRING]          = PRECEDENCE_LOWEST,  
                                   
    [TOKEN_TYPE_ASSIGN]          = PRECEDENCE_ASSIGN,   
    [TOKEN_TYPE_PLUS]            = PRECEDENCE_SUM,      
    [TOKEN_TYPE_MINUS]           = PRECEDENCE_SUM,      
    [TOKEN_TYPE_BANG]            = PRECEDENCE_LOWEST,   
    [TOKEN_TYPE_ASTERISK]        = PRECEDENCE_PRODUCT,  
    [TOKEN_TYPE_SLASH]           = PRECEDENCE_PRODUCT,  
    [TOKEN_TYPE_PERCENT]         = PRECEDENCE_PRODUCT,  
                                   
    [TOKEN_TYPE_PLUS_PLUS]       = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_MINUS_MINUS]     = PRECEDENCE_LOWEST,  
                                   
    [TOKEN_TYPE_LT]              = PRECEDENCE_EQUALS,  
    [TOKEN_TYPE_GT]              = PRECEDENCE_EQUALS,  
    [TOKEN_TYPE_LT_EQ]           = PRECEDENCE_EQUALS,  
    [TOKEN_TYPE_GT_EQ]           = PRECEDENCE_EQUALS,  
    [TOKEN_TYPE_EQ]              = PRECEDENCE_EQUALS,  
    [TOKEN_TYPE_NOT_EQ]          = PRECEDENCE_EQUALS,  

    [TOKEN_TYPE_LAND]            = PRECEDENCE_LAND,
    [TOKEN_TYPE_LOR]             = PRECEDENCE_LOR,
    [TOKEN_TYPE_BAND]            = PRECEDENCE_BAND,
    [TOKEN_TYPE_BOR]             = PRECEDENCE_BOR,
    [TOKEN_TYPE_NOT]             = PRECEDENCE_PREFIX,
    [TOKEN_TYPE_XOR]             = PRECEDENCE_XOR,

    [TOKEN_TYPE_LEFT_SHIFT]      = PRECEDENCE_SHIFT,
    [TOKEN_TYPE_RIGHT_SHIFT]     = PRECEDENCE_SHIFT,

    [TOKEN_TYPE_LEFT_COMPOSE]    = PRECEDENCE_COMPOSE,
    [TOKEN_TYPE_RIGHT_COMPOSE]   = PRECEDENCE_COMPOSE,
    [TOKEN_TYPE_LEFT_PIPE]       = PRECEDENCE_PIPE,
    [TOKEN_TYPE_RIGHT_PIPE]      = PRECEDENCE_PIPE,
                                 
    [TOKEN_TYPE_COMMA]           = PRECEDENCE_TUPLE,    
    [TOKEN_TYPE_COLON]           = PRECEDENCE_APPEND,   
    [TOKEN_TYPE_COLON_COLON]     = PRECEDENCE_ASSIGN,   
    [TOKEN_TYPE_SEMICOLON]       = PRECEDENCE_LOWEST,   
    [TOKEN_TYPE_DOT]             = PRECEDENCE_FIELD,  
                                   
    [TOKEN_TYPE_LPAREN]          = PRECEDENCE_CALL,    
    [TOKEN_TYPE_RPAREN]          = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_LBRACE]          = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_RBRACE]          = PRECEDENCE_LOWEST,  
    [TOKEN_TYPE_LBRACKET]        = PRECEDENCE_INDEX,   
    [TOKEN_TYPE_RBRACKET]        = PRECEDENCE_LOWEST,  
                                   
    [TOKEN_TYPE_QUESTION]        = PRECEDENCE_TERNARY,   
    [TOKEN_TYPE_RIGHT_ARROW]     = PRECEDENCE_FUNCTION,  
    [TOKEN_TYPE_FAT_RIGHT_ARROW] = PRECEDENCE_LOWEST,
};
// clang-format on

// clang-format off
static const expression_associativity assoc_lookup[TOKEN_TYPE_ENUM_LENGTH] = {
    [TOKEN_TYPE_ILLEGAL]         = ASSOC_NONE,
    [TOKEN_TYPE_EOF]             = ASSOC_NONE,
                                  
    [TOKEN_TYPE_IDENT]           = ASSOC_NONE,
    [TOKEN_TYPE_CHAR]            = ASSOC_NONE,
    [TOKEN_TYPE_INT]             = ASSOC_NONE,
    [TOKEN_TYPE_FLOAT]           = ASSOC_NONE,
    [TOKEN_TYPE_STRING]          = ASSOC_NONE,
                                   
    [TOKEN_TYPE_ASSIGN]          = ASSOC_RIGHT,
    [TOKEN_TYPE_PLUS]            = ASSOC_LEFT,
    [TOKEN_TYPE_MINUS]           = ASSOC_LEFT,
    [TOKEN_TYPE_BANG]            = ASSOC_NONE,
    [TOKEN_TYPE_ASTERISK]        = ASSOC_LEFT,
    [TOKEN_TYPE_SLASH]           = ASSOC_LEFT,
    [TOKEN_TYPE_PERCENT]         = ASSOC_LEFT,
                                   
    [TOKEN_TYPE_PLUS_PLUS]       = ASSOC_NONE,
    [TOKEN_TYPE_MINUS_MINUS]     = ASSOC_NONE,
                                   
    [TOKEN_TYPE_LT]              = ASSOC_LEFT,
    [TOKEN_TYPE_GT]              = ASSOC_LEFT,
    [TOKEN_TYPE_LT_EQ]           = ASSOC_LEFT,
    [TOKEN_TYPE_GT_EQ]           = ASSOC_LEFT,
    [TOKEN_TYPE_EQ]              = ASSOC_LEFT,
    [TOKEN_TYPE_NOT_EQ]          = ASSOC_LEFT,

    [TOKEN_TYPE_LAND]            = ASSOC_LEFT,
    [TOKEN_TYPE_LOR]             = ASSOC_LEFT,
    [TOKEN_TYPE_BAND]            = ASSOC_LEFT,
    [TOKEN_TYPE_BOR]             = ASSOC_LEFT,
    [TOKEN_TYPE_NOT]             = ASSOC_LEFT,
    [TOKEN_TYPE_XOR]             = ASSOC_LEFT,

    [TOKEN_TYPE_LEFT_SHIFT]      = ASSOC_LEFT,
    [TOKEN_TYPE_RIGHT_SHIFT]     = ASSOC_LEFT,

    [TOKEN_TYPE_LEFT_COMPOSE]    = ASSOC_LEFT,
    [TOKEN_TYPE_RIGHT_COMPOSE]   = ASSOC_RIGHT,
    [TOKEN_TYPE_LEFT_PIPE]       = ASSOC_LEFT,
    [TOKEN_TYPE_RIGHT_PIPE]      = ASSOC_RIGHT,
                                 
    [TOKEN_TYPE_COMMA]           = ASSOC_RIGHT,
    [TOKEN_TYPE_COLON]           = ASSOC_RIGHT,
    [TOKEN_TYPE_COLON_COLON]     = ASSOC_RIGHT,
    [TOKEN_TYPE_SEMICOLON]       = ASSOC_NONE,
    [TOKEN_TYPE_DOT]             = ASSOC_LEFT,
                                   
    [TOKEN_TYPE_LPAREN]          = ASSOC_LEFT,
    [TOKEN_TYPE_RPAREN]          = ASSOC_NONE,
    [TOKEN_TYPE_LBRACE]          = ASSOC_NONE,
    [TOKEN_TYPE_RBRACE]          = ASSOC_NONE,
    [TOKEN_TYPE_LBRACKET]        = ASSOC_LEFT, 
    [TOKEN_TYPE_RBRACKET]        = ASSOC_NONE,  
                                   
    [TOKEN_TYPE_QUESTION]        = ASSOC_LEFT,
    [TOKEN_TYPE_RIGHT_ARROW]     = ASSOC_RIGHT,  
    [TOKEN_TYPE_LEFT_ARROW]      = ASSOC_LEFT,
    [TOKEN_TYPE_FAT_RIGHT_ARROW] = ASSOC_NONE,
};
// clang-format on

#endif  // PARSER_H
