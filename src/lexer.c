#include "lexer.h"

#include <string.h>

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

static void read_comment(lexer *l);
static void read_char(lexer *l);
static char peek_char(lexer *l);

static inline void read_ident(lexer *l);
static inline token_type read_number(lexer *l);
static inline void read_char_literal(lexer *l);
static inline void read_string_literal(lexer *l);
static inline void eat_whitespace(lexer *l);
static inline bool is_valid_starting_ident_char(char c);
static inline bool is_digit(char c);
static inline bool is_whitespace(char c);
static inline bool is_exec(token *t);

void lexer_init(lexer *l, const char *filename, const char *input, size_t n) {
    *l = (lexer){
        .filename = filename,
        .input = input,
        .input_length = n,
        .line_number = 1,
    };

    read_char(l);
}

token lexer_next_token(lexer *l) {
start:
    eat_whitespace(l);

    while (l->ch == '#') {
        read_comment(l);
        eat_whitespace(l);
    }

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
            } else if (peek_char(l) == '>') {
                tok.type = TOKEN_TYPE_FAT_RIGHT_ARROW;
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
        case '%': {
            tok.type = TOKEN_TYPE_PERCENT;
        } break;
        case '&': {
            if (peek_char(l) == '&') {
                tok.type = TOKEN_TYPE_LAND;
                tok.length = 2;
                read_char(l);
            } else {
                tok.type = TOKEN_TYPE_BAND;
            }
        } break;
        case '|': {
            if (peek_char(l) == '|') {
                tok.type = TOKEN_TYPE_LOR;
                tok.length = 2;
                read_char(l);
            } else if (peek_char(l) == '>') {
                tok.type = TOKEN_TYPE_RIGHT_PIPE;
                tok.length = 2;
                read_char(l);
            } else {
                tok.type = TOKEN_TYPE_BOR;
            }
        } break;
        case '~': {
            tok.type = TOKEN_TYPE_NOT;
        } break;
        case '^': {
            tok.type = TOKEN_TYPE_XOR;
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
        case '>': {
            char p = peek_char(l);
            switch (p) {
                case '=': {
                    tok.type = TOKEN_TYPE_GT_EQ;
                    tok.length = 2;
                    read_char(l);
                } break;
                case '>': {
                    read_char(l);
                    if (peek_char(l) == '>') {
                        tok.type = TOKEN_TYPE_RIGHT_COMPOSE;
                        tok.length = 3;
                        read_char(l);
                    } else {
                        tok.type = TOKEN_TYPE_RIGHT_SHIFT;
                        tok.length = 2;
                    }
                } break;
                default: {
                    tok.type = TOKEN_TYPE_GT;
                }
            }
        } break;
        case '<': {
            char p = peek_char(l);
            switch (p) {
                case '<': {
                    read_char(l);
                    if (peek_char(l) == '<') {
                        tok.type = TOKEN_TYPE_LEFT_COMPOSE;
                        tok.length = 3;
                        read_char(l);
                    } else {
                        tok.type = TOKEN_TYPE_LEFT_SHIFT;
                        tok.length = 2;
                    }
                } break;
                case '|': {
                    tok.type = TOKEN_TYPE_LEFT_PIPE;
                    tok.length = 2;
                    read_char(l);
                } break;
                case '=': {
                    tok.type = TOKEN_TYPE_LT_EQ;
                    tok.length = 2;
                    read_char(l);
                } break;
                default: {
                    tok.type = TOKEN_TYPE_LT;
                }
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
        case '\'': {
            size_t pos = l->position;
            tok.type = TOKEN_TYPE_CHAR;
            read_char_literal(l);
            tok.length = l->position - pos;
            return tok;
        } break;
        case '"': {
            size_t pos = l->position;
            tok.type = TOKEN_TYPE_STRING;
            read_string_literal(l);
            tok.length = l->position - pos;
            return tok;
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
                if (is_exec(&tok)) {
                    read_comment(l);
                    goto start;
                }
                return tok;
            } else if (is_digit(l->ch)) {
                tok.type = read_number(l);
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

void print_lexer_output(const char *filename, const char *input, size_t n) {
    lexer l;
    lexer_init(&l, filename, input, n);

    token tok = lexer_next_token(&l);
    while (tok.type != TOKEN_TYPE_EOF) {
        printf("TOKEN type: %-10s literal: %-10.*s length: %lu\n",
               token_type_literals[tok.type], (int)tok.length, tok.literal,
               tok.length);

        tok = lexer_next_token(&l);
    }
}

static void read_comment(lexer *l) {
    for (; l->ch != '\n' && l->ch != 0 && l->ch != EOF; read_char(l));
    eat_whitespace(l);
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
    if (l->read_position >= l->input_length) return 0;
    return l->input[l->read_position];
}

static inline void read_ident(lexer *l) {
    for (; is_valid_starting_ident_char(l->ch) || is_digit(l->ch);
         read_char(l));
}

static inline token_type read_number(lexer *l) {
    bool seen_dot = false;
    token_type tt = TOKEN_TYPE_INT;
    for (; is_digit(l->ch); read_char(l)) {
        if (!seen_dot && peek_char(l) == '.') {
            read_char(l);
            if (!is_digit(peek_char(l)))
                return TOKEN_TYPE_INT;
            read_char(l);
            tt = TOKEN_TYPE_FLOAT;
            seen_dot = true;
        }
    }
    return tt;
}

static inline void read_char_literal(lexer *l) {
    read_char(l);  // first single quote
    for (; l->ch != '\'' && l->ch != 0; read_char(l)) {
        if (l->ch == '\\' && peek_char(l) == '\'') read_char(l);
    }
    read_char(l);  // second single quote
}

static inline void read_string_literal(lexer *l) {
    read_char(l);  // first quote
    for (; l->ch != '"' && l->ch != 0; read_char(l)) {
        if (l->ch == '\\' && peek_char(l) == '"') read_char(l);
    }
    read_char(l);  // second quote
}

static inline void eat_whitespace(lexer *l) {
    for (; is_whitespace(l->ch); read_char(l));
}

static inline bool is_valid_starting_ident_char(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

static inline bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static inline bool is_exec(token *t) {
    if (t->line_number != 2)
        return false;
    return strncmp(t->literal, "exec", 4) == 0;
}

#ifndef MAX_STACK_SIZE
#define MAX_STACK_SIZE 256
#endif

static token_type tok_stack[MAX_STACK_SIZE];
static size_t stack_size = 0;

static void populate_stack(const char *input, size_t n) {
    lexer l = {
        .input = input,
        .input_length = n,
        .line_number = 1,
    };

    read_char(&l);

    token tok;
    do {
        tok = lexer_next_token(&l);

        switch (tok.type) {
            case TOKEN_TYPE_LPAREN:
            case TOKEN_TYPE_LBRACE:
            case TOKEN_TYPE_LBRACKET: {
                assert(stack_size < MAX_STACK_SIZE && "Too many open parens/brackets/braces");

                tok_stack[stack_size++] = tok.type;
            } break;
            case TOKEN_TYPE_RPAREN:
            case TOKEN_TYPE_RBRACE:
            case TOKEN_TYPE_RBRACKET: {
                if (stack_size == 0)
                    continue;

                if (tok_stack[stack_size - 1] == tok.type - 1)
                    --stack_size;
            } break;
            default: {
            }
        }
    } while (tok.type != TOKEN_TYPE_EOF);
}

bool wait_for_more(const char *input, size_t n) {
    if (n == 0 || input[0] == 0) {
        stack_size = 0;
        return false;
    }

    populate_stack(input, n);

    return stack_size > 0;
}
