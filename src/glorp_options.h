#ifndef GLORP_OPTIONS_H
#define GLORP_OPTIONS_H

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t glorp_flags;

#define HELP_FLAG (1 << 0)
#define LEX_FLAG  (1 << 1)
#define AST_FLAG  (1 << 2)
#define REPL_FLAG (1 << 3)

typedef struct {
    char *filename;

    char **args;
    size_t argc;

    glorp_flags flags;
} glorp_options;

#endif // OPTIONS_H
