#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "ast.h"
#include "object.h"

object eval(expression_reference ref, environment *env); 

#endif  // EVAULATOR_H
