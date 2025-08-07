#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "glorpoptions.h"
#include "environment.h"

void interpret(const char *input, const glorp_options *selected_options);

// evaluate imported files
bool interpret_with_env(const char *input, const glorp_options *selected_options, environment *env);

#endif  // INTERPRETER_H
