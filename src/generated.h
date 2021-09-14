#ifndef META_GEN_H
#define META_GEN_H

#include <stddef.h>
#include <stdint.h>

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

