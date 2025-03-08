#include "object.h"

#include "arena.h"

object new_object(object_type ot) {
    return (object){
        .type = ot,
    };
}

void object_list_init(object_list *ol, arena *a) {
    *ol = (object_list){
        .a = a,
    };
}

void ol_append(object_list *ol, object_reference ref) {
    object ln = new_object(OBJECT_TYPE_LIST_NODE);
    ln.list_node.value = ref;
    object_reference ln_ref = arena_alloc_object(ol->a, ln);

    if (ol->size == 0) {
        ol->head = ln_ref;
    } else {
        ol->a->objects[ol->tail].list_node.next = ln_ref;
    }
    ol->tail = ln_ref;
    ++ol->size;
}

object_list_iterator ol_start(object_list *ol) {
    object_reference ln_ref = ol->head;
    if (ln_ref == NULL_OBJECT_REFERENCE) return ol_end(ol);
    object *ln = get_object(ol->a, ln_ref);
    return (object_list_iterator){
        .ref = ln->list_node.value,
        .obj = get_object(ol->a, ln->list_node.value),
        .ln_ref = ln_ref,
        .ol = ol,
    };
}

object_list_iterator ol_end(object_list *ol) {
    return (object_list_iterator){
        .ref = NULL_OBJECT_REFERENCE,
        .obj = NULL,
        .ol = ol,
    };
}

void oli_next(object_list_iterator *oli) {
    object *ln = get_object(oli->ol->a, oli->ln_ref);
    oli->ln_ref = ln->list_node.next;

    // TODO:

    /* oli->ref = oli->obj->next; */
    /* oli->obj = get_object(oli->ol->a, oli->obj->next); */
}

bool oli_eq(object_list_iterator *a, object_list_iterator *b) {
    return a->obj == b->obj;
}

// inspecting (printing) objects

typedef void inspect_fn(object *obj);

static inspect_fn inspect_null;
static inspect_fn inspect_int;
static inspect_fn inspect_function;
static inspect_fn inspect_list;

static inspect_fn *const inspect_fns[OBJECT_TYPE_ENUM_LENGTH] = {
    inspect_null,
    inspect_int,
    inspect_function,
    inspect_list,
};

void inspect(object *obj) {
    inspect_fn *inspect2 = inspect_fns[obj->type];
    inspect2(obj);
}

static void inspect_null(object *_) { printf("null\n"); }

static void inspect_int(object *obj) { printf("%ld\n", obj->int_object.value); }

static void inspect_function(object *_) {
    // TODO:
    printf("function\n");
}

static void inspect_list(object *_) {
    // TODO:
    printf("list\n");
}
