#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "object.h"

WARN_UNUSED_RESULT
bool eval(const expr *program, environment *env, object *obj);

void add_cmdline_args(char **args, size_t argc, environment *env);

void add_builtins(environment *env);

#endif  // EVAULATOR_H
