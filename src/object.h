#ifndef OBJECT_H
#define OBJECT_H

#define JSMN_HEADER
#include "jsmn.h"

#include "common.h"

enum {
    OBJECT_UNDEFINED,
    OBJECT_NULL,
    OBJECT_BOOL,
    OBJECT_NUMBER,
    OBJECT_STRING,
    OBJECT_OBJECT,
    OBJECT_ARRAY,
    OBJECT_TOTAL
};

struct object {
    int type;
    char *k;
    union {
        char *v_string;
        f64 v_num;
        int v_bool;
    };
    struct object *next;
    struct object *child;
    struct object *parent;
};

// object_ss: sets the type on 'object' to string, and sets v_string to 's'
void object_ss(struct object *object, char *s);

// object_ss: sets the type on 'object' to number, and sets v_num to 'n'
void object_sn(struct object *object, f64 n);

// object_sb: sets the type on 'object' to bool and sets v_bool to 'b'
void object_sb(struct object *object, int b);

// object_so: sets the type on 'object' to object and sets child to 'o'
void object_so(struct object *object, struct object *o);

// object_sa: sets the type on 'object' to array and sets child to 'a'
void object_sa(struct object *object, struct object *a);

// object_gt: locates the object with 'path', returns the type
int object_gt(struct object *object, char *path);

// object_gs: locates the object with 'path', returns the string
char *object_gs(struct object *object, char *path);

// object_gn: locates the object with 'path', returns the number
double object_gn(struct object *object, char *path);

// object_gb: locates the object with 'path', returns the boolean
int object_gb(struct object *object, char *path);

// object_go: finds the object with 'path', returns that object (NULL if it isn't an object)
struct object *object_go(struct object *object, char *path);

// object_ga: finds the object with 'path', returns that object (NULL if it's an array)
struct object *object_ga(struct object *object, char *path);

// object_deref: locates the object with 'path'
struct object *object_deref(struct object *root, char *path);

// object_from_json: creates an object from a list of json tokens
struct object *object_from_json(char *s, size_t len);

// object_from_tokens: recursive function building up the 'object' tree
struct object *object_from_tokens(char *s, jsmntok_t *tokens, size_t len);

// object_free: frees the object
void object_free(struct object *object);

#endif // OBJECT_H

