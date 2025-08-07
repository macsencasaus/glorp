#ifndef GLORP_OPTIONS_H
#define GLORP_OPTIONS_H

#include <stdint.h>
#include <stdlib.h>

typedef uint32_t glorp_flags;

// clang-format off
#define HELP_FLAG       (1 << 0)
#define LEX_FLAG        (1 << 1)
#define ONLY_LEX_FLAG   (1 << 2)
#define AST_FLAG        (1 << 3)
#define ONLY_AST_FLAG   (1 << 4)
#define REPL_FLAG       (1 << 5)
#define STDIN_FLAG      (1 << 6)
#define VERBOSE_FLAG    (1 << 7)
// clang-format on

typedef struct {
    const char *file_name;

    char **args;
    size_t argc;

    glorp_flags flags;
} glorp_options;

#endif  // OPTIONS_H
