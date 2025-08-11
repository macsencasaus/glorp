#ifndef GLORP_H
#define GLORP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    TOKEN_TYPE_ILLEGAL,
    TOKEN_TYPE_EOF,

    TOKEN_TYPE_IDENT,
    TOKEN_TYPE_CHAR,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_STRING,

    TOKEN_TYPE_ASSIGN,    // =
    TOKEN_TYPE_PLUS,      // +
    TOKEN_TYPE_MINUS,     // -
    TOKEN_TYPE_BANG,      // !
    TOKEN_TYPE_ASTERISK,  // *
    TOKEN_TYPE_SLASH,     // /
    TOKEN_TYPE_PERCENT,   // %

    TOKEN_TYPE_PLUS_PLUS,    // ++
    TOKEN_TYPE_MINUS_MINUS,  // --

    TOKEN_TYPE_LT,      // <
    TOKEN_TYPE_GT,      // >
    TOKEN_TYPE_LT_EQ,   // <=
    TOKEN_TYPE_GT_EQ,   // >=
    TOKEN_TYPE_EQ,      // ==
    TOKEN_TYPE_NOT_EQ,  // !=

    TOKEN_TYPE_LAND,  // &&
    TOKEN_TYPE_LOR,   // ||
    TOKEN_TYPE_BAND,  // &
    TOKEN_TYPE_BOR,   // |
    TOKEN_TYPE_NOT,   // ~
    TOKEN_TYPE_XOR,   // ^

    TOKEN_TYPE_LEFT_SHIFT,   // <<
    TOKEN_TYPE_RIGHT_SHIFT,  // >>

    TOKEN_TYPE_LEFT_COMPOSE,   // <<<
    TOKEN_TYPE_RIGHT_COMPOSE,  // >>>
    TOKEN_TYPE_LEFT_PIPE,      // <|
    TOKEN_TYPE_RIGHT_PIPE,     // |>

    // Delimiters
    TOKEN_TYPE_COMMA,        // ,
    TOKEN_TYPE_COLON,        // :
    TOKEN_TYPE_COLON_COLON,  // ::
    TOKEN_TYPE_SEMICOLON,    // ;
    TOKEN_TYPE_DOT,          // .

    TOKEN_TYPE_LPAREN,    // (
    TOKEN_TYPE_RPAREN,    // )
    TOKEN_TYPE_LBRACE,    // {
    TOKEN_TYPE_RBRACE,    // }
    TOKEN_TYPE_LBRACKET,  // [
    TOKEN_TYPE_RBRACKET,  // ]

    TOKEN_TYPE_QUESTION,     // ?
    TOKEN_TYPE_RIGHT_ARROW,  // ->
    TOKEN_TYPE_LEFT_ARROW,   // <-

    TOKEN_TYPE_FAT_RIGHT_ARROW, // =>

    TOKEN_TYPE_ENUM_LENGTH,
} token_type;

typedef struct {
    token_type type;

    const char *literal;
    size_t length;

    uint32_t line_number;
    uint32_t col_number;
} token;

typedef struct expr_list expr_list;
typedef struct expr expr;

struct expr_list {
    expr *head;
    expr *tail;
    size_t size;
};

typedef enum {
    EXPR_TYPE_NONE,

    EXPR_TYPE_PROGRAM,

    EXPR_TYPE_UNIT,

    EXPR_TYPE_IDENTIFIER,
    EXPR_TYPE_CHAR_LITERAL,
    EXPR_TYPE_INT_LITERAL,
    EXPR_TYPE_FLOAT_LITERAL,
    EXPR_TYPE_STRING_LITERAL,
    EXPR_TYPE_LIST_LITERAL,

    EXPR_TYPE_BLOCK_EXPRESSION,
    EXPR_TYPE_PREFIX_EXPRESSION,
    EXPR_TYPE_INFIX_EXPRESSION,
    EXPR_TYPE_TERNARY_EXPRESSION,
    EXPR_TYPE_CALL_EXPRESSION,
    EXPR_TYPE_INDEX_EXPRESSION,
    EXPR_TYPE_CASE_EXPRESSION,

    EXPR_TYPE_IMPORT_EXPRESSION,

    EXPR_ENUM_LENGTH,
} expr_type;

struct expr {
    token start_tok;
    token end_tok;

    expr *next;  // only used in expression list

    expr_type type;
    union {
        // program
        // list literal
        // block
        expr_list expressions;

        // identifier
        // string literal
        // import
        struct {
            const char *literal;
            size_t length;
        };

        // char literal
        char char_value;

        // int literal
        int64_t int_value;

        // float literal
        double float_value;

        // prefix
        // assign
        // infix
        struct {
            token op;
            expr *right;
            expr *left;
        };

        // ternary
        struct {
            expr *condition;
            expr *consequence;
            expr *alternative;
        };

        // call
        struct {
            expr *function;
            expr_list params;
        };

        // index
        struct {
            expr *list;
            expr *index;
        };

        // case
        struct {
            expr_list conditions;
            expr_list results;
        };
    };
};

typedef struct environment environment;
typedef struct object_list object_list;
typedef struct object object;

struct object_list {
    object *head;
    object *tail;
    size_t size;
};

typedef enum {
    OBJECT_TYPE_NULL,

    OBJECT_TYPE_UNIT,

    OBJECT_TYPE_CHAR,
    OBJECT_TYPE_INT,
    OBJECT_TYPE_FLOAT,

    OBJECT_TYPE_FUNCTION,
    OBJECT_TYPE_LIST,

    OBJECT_TYPE_LVALUE,

    // doesn't get evaluated
    OBJECT_TYPE_LIST_NODE,
    OBJECT_TYPE_ENVIRONMENT,

    OBJECT_TYPE_ENUM_LENGTH,
} object_type;


typedef bool builtin_fn_t(const expr_list *params, const expr *call, environment *env, object *result);

struct object {
    size_t rc;

    object_type type;
    union {
        // char
        char char_value;

        // int
        int64_t int_value;

        // float
        double float_value;

        // function
        struct {
            bool builtin;
            union {
                // normal functions
                struct {
                    expr_list params;
                    const expr *body;
                };

                // builtin functions
                struct {
                    size_t builtin_param_count;
                    builtin_fn_t *builtin_fn;
                };
            };

            environment *outer_env;
        };

        // list
        object_list values;

        // lvalue
        object *ref;  // reference to heap allocated object

        // list node
        struct {
            object *value;
            object *next;
        };

        // environment
        // environment env;
    };
};

typedef struct {
    object *obj;
    object *ln;
} ol_iterator;

void ol_append(object_list *ol, object *obj);
ol_iterator ol_start(const object_list *ol);

void oli_next(ol_iterator *oli);
bool oli_is_end(const ol_iterator *oli);

typedef struct {
    char *name;
    builtin_fn_t *fn;
    size_t param_count;
} builtin_entry;

bool eval(const expr *program, environment *env, object *obj);

#endif // GLORP_H
