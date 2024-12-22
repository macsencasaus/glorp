#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <stdio.h>

typedef size_t object_reference;

typedef enum {
    OBJECT_TYPE_INT,
    OBJECT_TYPE_FLOAT,
    OBJECT_TYPE_CHAR,
    OBJECT_TYPE_LIST,
} object_type;

typedef struct {
    int64_t value;
} int_object;

typedef struct {
    double value;
} float_object;

typedef struct {
    char value;
} char_object;

typedef struct {
    char value;
} list_object;

typedef struct {
    size_t rc;
    object_type type;
} object;

#endif  // OBJECT_H
