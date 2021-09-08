// Brian Chrzanowski
// 2021-09-08 13:01:19

#include <stdlib.h>

#include "jsmn.h"

#include "common.h"

#include "object.h"

// object_ss: sets the type on 'object' to string, and sets v_string to 's'
void object_ss(struct object *object, char *s)
{
	if (object == NULL)
		return;
	object->type = OBJECT_STRING;
	object->v_string = s;
}

// object_ss: sets the type on 'object' to number, and sets v_num to 'n'
void object_sn(struct object *object, f64 n)
{
	if (object == NULL)
		return;
	object->type = OBJECT_NUMBER;
	object->v_num = n;
}

// object_sb: sets the type on 'object' to bool and sets v_bool to 'b'
void object_sb(struct object *object, int b)
{
	if (object == NULL)
		return;
	object->type = OBJECT_BOOL;
	object->v_bool = b;
}

// object_so: sets the type on 'object' to object and sets child to 'o'
void object_so(struct object *object, struct object *o)
{
	if (object == NULL)
		return;
	object->type = OBJECT_OBJECT;
	object->child = o;
}

// object_sa: sets the type on 'object' to array and sets child to 'a'
void object_sa(struct object *object, struct object *a)
{
	if (object == NULL)
		return;
	object->type = OBJECT_ARRAY;
	object->child = a;
}

// object_gt: locates the object with 'path', returns the type
int object_gt(struct object *object, char *path)
{
    struct object *z;

    z = object_deref(object, path);

    return z == NULL ? OBJECT_UNDEFINED : z->type;
}

// object_gs: locates the object with 'path', returns the string
char *object_gs(struct object *object, char *path)
{
    struct object *z;

    z = object_deref(object, path);

    return z == NULL ? NULL : strdup(z->v_string);
}

// object_gn: locates the object with 'path', returns the number
double object_gn(struct object *object, char *path)
{
    struct object *z;

    z = object_deref(object, path);

    return z == NULL ? 0.0 : z->v_num;
}

// object_gb: locates the object with 'path', returns the boolean
int object_gb(struct object *object, char *path)
{
    struct object *z;

    z = object_deref(object, path);

    return z == NULL ? -1 : z->v_bool;
}

// object_go: finds the object with 'path', returns that object (NULL if it isn't an object)
struct object *object_go(struct object *object, char *path)
{
    struct object *z;

    z = object_deref(object, path);

    return z != NULL && z->type == OBJECT_OBJECT ? z : NULL;
}

// object_ga: finds the object with 'path', returns that object (NULL if it's an array)
struct object *object_ga(struct object *object, char *path)
{
    struct object *z;

    z = object_deref(object, path);

    return z != NULL && z->type == OBJECT_ARRAY ? z : NULL;
}

// object_deref: locates the object with 'path'
struct object *object_deref(struct object *root, char *path)
{
    // NOTE (Brian) this function basically attempts to dereference strings until we get to an
    // object / array dereference, then we return an attempt in the child to dereference.

    struct object *curr;
    char *k, *e;
    size_t len;

    if (root == NULL || path == NULL)
        return NULL;

    k = path;

    if (k[0] == '.' || k[0] == '[') {
        return object_deref(root->child, path + 1);
    } else {
        for (e = k; *e && *e != '.' && *e != '[' && *e != ']'; e++)
            ;

        len = e - k;

        for (curr = root; curr; curr = curr->next) {
            if (strncmp(k, curr->k, len) == 0) {
                return curr;
            }
        }

        return NULL;
    }
}

// object_from_json: creates an object from a list of json tokens
struct object *object_from_json(char *s, size_t len)
{
    jsmn_parser parser;
    jsmntok_t tokens[1024];
    int rc;

    jsmn_init(&parser);

    rc = jsmn_parse(&parser, s, len, tokens, ARRSIZE(tokens));

    if (rc < 0) {
        return NULL;
    }

    return object_from_tokens(s, tokens, rc);
}

// object_from_tokens: recursive function building up the 'object' tree
struct object *object_from_tokens(char *s, jsmntok_t *tokens, size_t len)
{
    struct object *object;
    struct object *curr;
    size_t i;
    char *vstr;

    jsmntok_t key, val;

    if (len <= 0) {
        return NULL;
    }

    object = calloc(1, sizeof(*object));

    // do things based on what the key is
    if (tokens[0].type == JSMN_OBJECT) {
        object->type = OBJECT_OBJECT;

        object->child = object_from_tokens(s, tokens + 1, tokens[0].size);
        object->child->parent = object;
    } else if (tokens[0].type == JSMN_ARRAY) {
        object->type = OBJECT_ARRAY;

        object->child = object_from_tokens(s, tokens + 1, tokens[0].size);
        object->child->parent = object;
    } else {

        // NOTE (Brian): here's the part where we parse all of the k/v pairs of the JSON object.
        // It's the case that when we get into this block, 'len' has the number of tokens left to
        // parse. Given that while in this block we parse two tokens at a time (key and value), we
        // simply continue until we have < 1 tokens, then we return

        curr = object;

        for (i = 0; i < len; i++) {
            if (0 < i) {
                curr->next = calloc(1, sizeof(*curr->next));
                curr = curr->next;
            }

            key = tokens[i * 2 + 0];
            val = tokens[i * 2 + 1];

            vstr = s + val.start;

            curr->k = strndup(s + key.start, key.end - key.start);

			if (val.type == JSMN_STRING) {
                curr->type = OBJECT_STRING;
                curr->v_string = strndup(s + val.start, val.end - val.start);
            } else if (vstr[0] == 't' || vstr[0] == 'f') { // check if value is boolean
                curr->type = OBJECT_BOOL;
                curr->v_bool = vstr[0] == 't';
            } else if (isdigit(vstr[0]) || vstr[0] == '-') { // check if value is number
                curr->type = OBJECT_NUMBER;
                curr->v_num = atof(s + val.start);
            } else if (vstr[0] == 'n') { // if it's NULL
                curr->type = OBJECT_NULL;
            } else {
                assert(0); // HOW???
            }
        }
    }

    return object;
}

// object_free: frees the object
void object_free(struct object *object)
{
    if (object) {
        if (object->child)
            object_free(object->child);

        if (object->next)
            object_free(object->next);

        free(object->k);

        if (object->type == OBJECT_STRING)
            free(object->v_string);
    }
}

