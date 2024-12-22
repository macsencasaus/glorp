#include "lexer.h"

#include "stdlib.h"

static void read_char(lexer *l);
static char peek_char(lexer *l);

static inline void read_ident(lexer *l);
static inline void read_number(lexer *l);
static inline void eat_whitespace(lexer *l);
static inline bool is_valid_starting_ident_char(char c);
static inline bool is_digit(char c);
static inline bool is_whitespace(char c);

lexer new_lexer(char *filename, char *input, size_t n) {
    lexer l = {
        .filename = filename,
        .input = input,
        .input_length = n,

        .line_number = 1,
    };

    read_char(&l);
    // read_char(&l);
    
    return l;
}

token lexer_next_token(lexer *l) {
    eat_whitespace(l);

    token tok = {
        .literal = l->input + l->position,
        .length = 1,

        .line_number = l->line_number,
        .col_number = l->col_number,
    };

    switch (l->ch) {
    case '=': {
        if (peek_char(l) == '=') {
            tok.type = TOKEN_TYPE_EQ;
            tok.length = 2;
            read_char(l);
        } else {
            tok.type = TOKEN_TYPE_ASSIGN;
        }
    } break;
    case '+': {
        if (peek_char(l) == '+') {
            tok.type = TOKEN_TYPE_PLUS_PLUS;
            tok.length = 2;
            read_char(l);
        } else {
            tok.type = TOKEN_TYPE_PLUS;
        }
    } break;
    case '-': {
        if (peek_char(l) == '-') {
            tok.type = TOKEN_TYPE_MINUS_MINUS;
            tok.length = 2;
            read_char(l);
        } else if (peek_char(l) == '>') {
            tok.type = TOKEN_TYPE_RIGHT_ARROW;
            tok.length = 2;
            read_char(l);
        } else {
            tok.type = TOKEN_TYPE_MINUS;
        }
    } break;
    case '*': {
        tok.type = TOKEN_TYPE_ASTERISK;
    } break;
    case '/': {
        tok.type = TOKEN_TYPE_SLASH;
    } break;
    case '!': {
        if (peek_char(l) == '=') {
            tok.type = TOKEN_TYPE_NOT_EQ;
            tok.length = 2;
            read_char(l);
        } else {
            tok.type = TOKEN_TYPE_BANG;
        }
    } break;
    case '\\': {
        tok.type = TOKEN_TYPE_BACK_SLASH; 
    } break;
    case '>': {
        if (peek_char(l) == '=') {
            tok.type = TOKEN_TYPE_GT_EQ;
            tok.length = 2;
            read_char(l);
        } else {
            tok.type = TOKEN_TYPE_GT;
        }
    } break;
    case '<': {
        if (peek_char(l) == '=') {
            tok.type = TOKEN_TYPE_LT_EQ;
            tok.length = 2;
            read_char(l);
        } else {
            tok.type = TOKEN_TYPE_LT;
        }
    } break;
    case ',': {
        tok.type = TOKEN_TYPE_COMMA;
    } break;
    case ':': {
        if (peek_char(l) == ':') {
            tok.type = TOKEN_TYPE_COLON_COLON;
            tok.length = 2;
            read_char(l);
        } else {
            tok.type = TOKEN_TYPE_COLON;
        }
    } break;
    case ';': {
        tok.type = TOKEN_TYPE_SEMICOLON;
    } break;
    case '.': {
        tok.type = TOKEN_TYPE_DOT;
    } break;
    case '(': {
        tok.type = TOKEN_TYPE_LPAREN;
    } break;
    case ')': {
        tok.type = TOKEN_TYPE_RPAREN;
    } break;
    case '{': {
        tok.type = TOKEN_TYPE_LBRACE;
    } break;
    case '}': {
        tok.type = TOKEN_TYPE_RBRACE;
    } break;
    case '[': {
        tok.type = TOKEN_TYPE_LBRACKET;
    } break;
    case ']': {
        tok.type = TOKEN_TYPE_RBRACKET;
    } break;
    case '?': {
        tok.type = TOKEN_TYPE_QUESTION;
    } break;
    case '\0': {
        tok.type = TOKEN_TYPE_EOF;
    } break;

    default: {
        size_t pos = l->position;
        if (is_valid_starting_ident_char(l->ch)) {
            read_ident(l);
            tok.type = TOKEN_TYPE_IDENT;
            tok.length = l->position - pos;
            return tok;
        } else if (is_digit(l->ch)) {
            read_number(l);
            tok.type = TOKEN_TYPE_INT;
            tok.length = l->position - pos;
            return tok;
        } else {
            tok.type = TOKEN_TYPE_ILLEGAL;
        }
    }

    }

    read_char(l);
    return tok;
}

static void read_char(lexer *l) {
    if (l->read_position >= l->input_length) {
        l->ch = 0;
    } else {
        l->ch = l->input[l->read_position];
    }
    
    if (l->ch == '\n') {
        ++l->line_number;
        l->col_number = 0;
    } else {
        ++l->col_number;
    }

    l->position = l->read_position;
    ++l->read_position;
}

static inline char peek_char(lexer *l) {
    return l->input[l->read_position];
}

static inline void read_ident(lexer *l) {
    for(; is_valid_starting_ident_char(l->ch) || is_digit(l->ch); read_char(l));
}

static inline void read_number(lexer *l) {
    for (; is_digit(l->ch); read_char(l));
}

static inline void eat_whitespace(lexer *l) {
    for (; is_whitespace(l->ch); read_char(l));
}

static inline bool is_valid_starting_ident_char(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static inline bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
