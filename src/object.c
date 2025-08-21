#include "object.h"

#include <ctype.h>
#include <string.h>

#include "arena.h"

void ol_append(object_list *ol, object *obj) {
    object *ln = new_obj(OBJECT_TYPE_LIST_NODE, 1);
    ln->value = obj;

    if (ol->size == 0) {
        ol->head = ln;
    } else {
        ol->tail->next = ln;
    }
    ol->tail = ln;
    ++ol->size;
}

ol_iterator ol_start(const object_list *ol) {
    object *ln = ol->head;
    object *obj = ln ? ln->value : NULL;
    return (ol_iterator){
        .obj = obj,
        .ln = ln,
    };
}

void oli_next(ol_iterator *oli) {
    if (oli_is_end(oli))
        return;

    object *next_ln = oli->ln->next;

    oli->ln = next_ln;
    oli->obj = next_ln ? next_ln->value : NULL;
}

bool oli_is_end(const ol_iterator *oli) {
    return oli->ln == NULL;
}

// inspecting (printing) objects

#define CHECK_LEN(len) \
    if (buf + len >= end) return 0;

typedef void inspect_fn(const object *obj, String_Builder *sb, bool from_print);

static inspect_fn inspect_null;
static inspect_fn inspect_unit;
static inspect_fn inspect_char;
static inspect_fn inspect_int;
static inspect_fn inspect_float;
static inspect_fn inspect_function;
static inspect_fn inspect_list;
static inspect_fn inspect_lvalue;

static inspect_fn *const inspect_fns[] = {
    [OBJECT_TYPE_NULL] = inspect_null,
    [OBJECT_TYPE_UNIT] = inspect_unit,
    [OBJECT_TYPE_CHAR] = inspect_char,
    [OBJECT_TYPE_INT] = inspect_int,
    [OBJECT_TYPE_FLOAT] = inspect_float,
    [OBJECT_TYPE_FUNCTION] = inspect_function,
    [OBJECT_TYPE_LIST] = inspect_list,
    [OBJECT_TYPE_LVALUE] = inspect_lvalue,
};

void inspect(const object *obj, String_Builder *sb, bool from_print) {
    inspect_fn *inspect2 = inspect_fns[obj->type];
    inspect2(obj, sb, from_print);
}

static void inspect_null(const object *obj, String_Builder *sb, bool from_print) {
    (void)obj;
    (void)from_print;
    sb_append_buf(sb, "null", 4);
}

static void inspect_unit(const object *obj, String_Builder *sb, bool from_print) {
    (void)obj;
    (void)from_print;
    sb_append_buf(sb, "()", 2);
}

static void inspect_char(const object *obj, String_Builder *sb, bool from_print) {
    char c = obj->char_value;

    if (isprint(c)) {
        if (from_print) {
            sb_appendf(sb, "%c", c);
        } else {
            sb_appendf(sb, "'%c'", c);
        }
    } else {
        char *symbol;
        switch (c) {
            case '\n': {
                symbol = from_print ? "\\n" : "'\\n'";
            } break;
            case '\r': {
                symbol = from_print ? "\\r" : "'\\r'";
            } break;
            case '\t': {
                symbol = from_print ? "\\t" : "'\\t'";
            } break;
            case '\b': {
                symbol = from_print ? "\\b" : "'\\b'";
            } break;
            case '\f': {
                symbol = from_print ? "\\f" : "'\\f'";
            } break;
            case '\v': {
                symbol = from_print ? "\\f" : "'\\v'";
            } break;
            default: {
                if (from_print) {
                    sb_appendf(sb, "%X", c);
                } else {
                    sb_appendf(sb, "'%X'", c);
                }
                return;
            }
        }
        sb_append_buf(sb, symbol, from_print ? 2 : 4);
    }
}

static void inspect_int(const object *obj, String_Builder *sb, bool from_print) {
    (void)from_print;
    sb_appendf(sb, "%lld", obj->int_value);
}

static void inspect_float(const object *obj, String_Builder *sb, bool from_print) {
    (void)from_print;
    sb_appendf(sb, "%g", obj->float_value);
}

static void inspect_function(const object *obj, String_Builder *sb, bool from_print) {
    (void)from_print;
    size_t param_count = obj->builtin ? obj->builtin_param_count : obj->params.size;
    sb_appendf(sb, "function(%zu)", param_count);
}

static bool check_str(const object_list *values) {
    ol_iterator it = ol_start(values);

    for (; !oli_is_end(&it); oli_next(&it)) {
        if (it.obj->type != OBJECT_TYPE_CHAR) {
            return false;
        }
    }

    return true;
}

static void inspect_str(const object_list *values, String_Builder *sb, bool from_print) {
    if (!from_print)
        sb_append_buf(sb, "\"", 1);

    ol_iterator it = ol_start(values);

    for (; !oli_is_end(&it); oli_next(&it)) {
        sb_appendf(sb, "%c", it.obj->char_value);
    }

    if (!from_print)
        sb_append_buf(sb, "\"", 1);
}

static void inspect_list(const object *obj, String_Builder *sb, bool from_print) {
    const object_list *values = &obj->values;

    if (values->size != 0 && check_str(values)) {
        inspect_str(values, sb, from_print);
        return;
    }

    ol_iterator it = ol_start(values);

    sb_append_buf(sb, "[", 1);

    size_t size = values->size;

    if (size > 0) {
        for (size_t i = 0; i < size - 1; oli_next(&it), ++i) {
            inspect(it.obj, sb, false);
            sb_append_buf(sb, ", ", 2);
        }

        inspect(it.obj, sb, false);
    }

    sb_append_buf(sb, "]", 1);
}

static void inspect_lvalue(const object *obj, String_Builder *sb, bool from_print) {
    inspect(obj->ref, sb, from_print);
}
