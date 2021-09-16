#ifndef META_GEN_H
#define META_GEN_H

// Brian Chrzanowski (C) 2021
// 
// This file was generated using the 'meta' program inside of the repository.
// Upon additional schema updates and builds, it will become overwritten.

#include <stddef.h>
#include <stdint.h>

#include "httpserver.h"

typedef int8_t   db_int8;
typedef int16_t  db_int16;
typedef int32_t  db_int32;
typedef int64_t  db_int64;

typedef uint8_t  db_uint8;
typedef uint16_t db_uint16;
typedef uint32_t db_uint32;
typedef uint64_t db_uint64;

typedef float    db_float;
typedef double   db_double;

typedef struct {
	char *str;
	size_t str_len, str_cap;
} db_string;

typedef struct {
	char *blob;
	size_t blob_len, blob_cap;
} db_blob;

typedef struct {
	char *tabname;
	char *colname;
	char *coltype;
	int ordpos;
	int isnull;
	int ispk;
} db_meta;

typedef struct {
	db_string id;
	db_string created_ts;
	db_string updated_ts;
	db_string username;
	db_string password;
	db_string email;
	db_int64 is_verified;
} user_t;

typedef struct {
	db_string user_id;
	db_string session_id;
	db_string expires_ts;
} user_session_t;

typedef struct {
	db_string id;
	db_string created_ts;
	db_string updated_ts;
	db_string name;
	db_int64 prep_time;
	db_int64 cook_time;
	db_string note;
} recipe_t;

typedef struct {
	db_string id;
	db_string created_ts;
	db_string updated_ts;
	db_string recipe_id;
	db_string desc;
	db_int64 sort;
} ingredient_t;

typedef struct {
	db_string id;
	db_string created_ts;
	db_string updated_ts;
	db_string recipe_id;
	db_string text;
	db_int64 sort;
} step_t;

typedef struct {
	db_string id;
	db_string created_ts;
	db_string updated_ts;
	db_string recipe_id;
	db_string text;
} tag_t;

// db_user_insert: handles 'insert' operations for the 'user' table
int db_user_insert(user_t *item);
// db_user_update: handles 'update' operations for the 'user' table
int db_user_update(user_t *item);
// db_user_delete: handles 'delete' operations for the 'user' table
int db_user_delete(user_t *item);
// db_user_select_by_id: handles 'select' operations for the 'user' table, by id
user_t *db_user_select_by_id(char *id);

// db_user_session_insert: handles 'insert' operations for the 'user_session' table
int db_user_session_insert(user_session_t *item);
// db_user_session_update: handles 'update' operations for the 'user_session' table
int db_user_session_update(user_session_t *item);
// db_user_session_delete: handles 'delete' operations for the 'user_session' table
int db_user_session_delete(user_session_t *item);
// db_user_session_select_by_id: handles 'select' operations for the 'user_session' table, by id
user_session_t *db_user_session_select_by_id(char *id);

// db_recipe_insert: handles 'insert' operations for the 'recipe' table
int db_recipe_insert(recipe_t *item);
// db_recipe_update: handles 'update' operations for the 'recipe' table
int db_recipe_update(recipe_t *item);
// db_recipe_delete: handles 'delete' operations for the 'recipe' table
int db_recipe_delete(recipe_t *item);
// db_recipe_select_by_id: handles 'select' operations for the 'recipe' table, by id
recipe_t *db_recipe_select_by_id(char *id);

// db_ingredient_insert: handles 'insert' operations for the 'ingredient' table
int db_ingredient_insert(ingredient_t *item);
// db_ingredient_update: handles 'update' operations for the 'ingredient' table
int db_ingredient_update(ingredient_t *item);
// db_ingredient_delete: handles 'delete' operations for the 'ingredient' table
int db_ingredient_delete(ingredient_t *item);
// db_ingredient_select_by_id: handles 'select' operations for the 'ingredient' table, by id
ingredient_t *db_ingredient_select_by_id(char *id);

// db_step_insert: handles 'insert' operations for the 'step' table
int db_step_insert(step_t *item);
// db_step_update: handles 'update' operations for the 'step' table
int db_step_update(step_t *item);
// db_step_delete: handles 'delete' operations for the 'step' table
int db_step_delete(step_t *item);
// db_step_select_by_id: handles 'select' operations for the 'step' table, by id
step_t *db_step_select_by_id(char *id);

// db_tag_insert: handles 'insert' operations for the 'tag' table
int db_tag_insert(tag_t *item);
// db_tag_update: handles 'update' operations for the 'tag' table
int db_tag_update(tag_t *item);
// db_tag_delete: handles 'delete' operations for the 'tag' table
int db_tag_delete(tag_t *item);
// db_tag_select_by_id: handles 'select' operations for the 'tag' table, by id
tag_t *db_tag_select_by_id(char *id);

// json_parse_user: a json parsing function for the 'user' table
user_t *json_parse_user(char *s, size_t len);

// json_parse_user_session: a json parsing function for the 'user_session' table
user_session_t *json_parse_user_session(char *s, size_t len);

// json_parse_recipe: a json parsing function for the 'recipe' table
recipe_t *json_parse_recipe(char *s, size_t len);

// json_parse_ingredient: a json parsing function for the 'ingredient' table
ingredient_t *json_parse_ingredient(char *s, size_t len);

// json_parse_step: a json parsing function for the 'step' table
step_t *json_parse_step(char *s, size_t len);

// json_parse_tag: a json parsing function for the 'tag' table
tag_t *json_parse_tag(char *s, size_t len);

// http_user_get: the http 'get' handler for the 'user' table
int http_user_get(struct http_request_s *req, struct http_response_s *res);
// http_user_post: the http 'post' handler for the 'user' table
int http_user_post(struct http_request_s *req, struct http_response_s *res);
// http_user_put: the http 'put' handler for the 'user' table
int http_user_put(struct http_request_s *req, struct http_response_s *res);
// http_user_delete: the http 'delete' handler for the 'user' table
int http_user_delete(struct http_request_s *req, struct http_response_s *res);

// http_user_session_get: the http 'get' handler for the 'user_session' table
int http_user_session_get(struct http_request_s *req, struct http_response_s *res);
// http_user_session_post: the http 'post' handler for the 'user_session' table
int http_user_session_post(struct http_request_s *req, struct http_response_s *res);
// http_user_session_put: the http 'put' handler for the 'user_session' table
int http_user_session_put(struct http_request_s *req, struct http_response_s *res);
// http_user_session_delete: the http 'delete' handler for the 'user_session' table
int http_user_session_delete(struct http_request_s *req, struct http_response_s *res);

// http_recipe_get: the http 'get' handler for the 'recipe' table
int http_recipe_get(struct http_request_s *req, struct http_response_s *res);
// http_recipe_post: the http 'post' handler for the 'recipe' table
int http_recipe_post(struct http_request_s *req, struct http_response_s *res);
// http_recipe_put: the http 'put' handler for the 'recipe' table
int http_recipe_put(struct http_request_s *req, struct http_response_s *res);
// http_recipe_delete: the http 'delete' handler for the 'recipe' table
int http_recipe_delete(struct http_request_s *req, struct http_response_s *res);

// http_ingredient_get: the http 'get' handler for the 'ingredient' table
int http_ingredient_get(struct http_request_s *req, struct http_response_s *res);
// http_ingredient_post: the http 'post' handler for the 'ingredient' table
int http_ingredient_post(struct http_request_s *req, struct http_response_s *res);
// http_ingredient_put: the http 'put' handler for the 'ingredient' table
int http_ingredient_put(struct http_request_s *req, struct http_response_s *res);
// http_ingredient_delete: the http 'delete' handler for the 'ingredient' table
int http_ingredient_delete(struct http_request_s *req, struct http_response_s *res);

// http_step_get: the http 'get' handler for the 'step' table
int http_step_get(struct http_request_s *req, struct http_response_s *res);
// http_step_post: the http 'post' handler for the 'step' table
int http_step_post(struct http_request_s *req, struct http_response_s *res);
// http_step_put: the http 'put' handler for the 'step' table
int http_step_put(struct http_request_s *req, struct http_response_s *res);
// http_step_delete: the http 'delete' handler for the 'step' table
int http_step_delete(struct http_request_s *req, struct http_response_s *res);

// http_tag_get: the http 'get' handler for the 'tag' table
int http_tag_get(struct http_request_s *req, struct http_response_s *res);
// http_tag_post: the http 'post' handler for the 'tag' table
int http_tag_post(struct http_request_s *req, struct http_response_s *res);
// http_tag_put: the http 'put' handler for the 'tag' table
int http_tag_put(struct http_request_s *req, struct http_response_s *res);
// http_tag_delete: the http 'delete' handler for the 'tag' table
int http_tag_delete(struct http_request_s *req, struct http_response_s *res);

#ifdef META_GEN_IMPLEMENTATION_H
#define META_GEN_IMPLEMENTATION_H

static db_meta metadata[] = {
	{"user","id","text",0,1,0},
	{"user","created_ts","text",1,1,0},
	{"user","updated_ts","text",2,1,0},
	{"user","username","text",3,0,0},
	{"user","password","text",4,0,0},
	{"user","email","text",5,0,0},
	{"user","is_verified","int",6,1,0},
	{"user_session","user_id","text",0,0,0},
	{"user_session","session_id","text",1,0,0},
	{"user_session","expires_ts","text",2,1,0},
	{"recipe","id","text",0,1,0},
	{"recipe","created_ts","text",1,1,0},
	{"recipe","updated_ts","text",2,1,0},
	{"recipe","name","text",3,0,0},
	{"recipe","prep_time","integer",4,0,0},
	{"recipe","cook_time","integer",5,0,0},
	{"recipe","note","text",6,1,0},
	{"ingredient","id","text",0,1,0},
	{"ingredient","created_ts","text",1,1,0},
	{"ingredient","updated_ts","text",2,1,0},
	{"ingredient","recipe_id","text",3,0,0},
	{"ingredient","desc","text",4,0,0},
	{"ingredient","sort","integer",5,0,0},
	{"step","id","text",0,1,0},
	{"step","created_ts","text",1,1,0},
	{"step","updated_ts","text",2,1,0},
	{"step","recipe_id","text",3,0,0},
	{"step","text","text",4,0,0},
	{"step","sort","integer",5,0,0},
	{"tag","id","text",0,1,0},
	{"tag","created_ts","text",1,1,0},
	{"tag","updated_ts","text",2,1,0},
	{"tag","recipe_id","text",3,0,0},
	{"tag","text","text",4,0,0}
};

#endif // META_GEN_IMPLEMENTATION_H

#endif // META_GEN_H

