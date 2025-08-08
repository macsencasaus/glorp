#ifndef GLORP_OPTIONS_H
#define GLORP_OPTIONS_H

#include <stdbool.h>
#include <stdint.h>

#include "argparse.h"

typedef struct {
    const char *file;
    Argp_List *args;

    bool lex : 1;
    bool ast : 1;
    bool repl : 1;
    bool verbose : 1;
} glorp_options;

#endif  // OPTIONS_H
