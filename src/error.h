#ifndef ERROR_H
#define ERROR_H

#include "token.h"

void generic_error(const char *filename, token tok, const char *msg);
void match_token_type_error(const char *filename, token tok, token_type tt);
void unexpected_token_error(const char *filename, token tok);

void print_error_line(token tok);

#endif  // ERROR_H
