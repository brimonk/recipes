#ifndef OBJECTS_H
#define OBJECTS_H

// Brian Chrzanowski
// 2021-10-14 16:47:32
//
// This file just defines a bunch of structured items we keep in arrays, in memory, until we
// need to flush them to disk, then we do.

#include "common.h"

#define OBJECT_FLAG_USED           (0x0001)
#define OBJECT_FLAG_LOCKED         (0x0002)
#define OBJECT_FLAG_VERIFIED       (0x0004)
#define OBJECT_FLAG_PUBLISHED      (0x0008)

typedef char string_char;

// objectbase_t : stores just the object's id, and some flags
typedef struct objectbase_t {
	u64 id;
	u64 flags;
} objectbase_t;

// string_128_t : a 128 char string
typedef struct string_128_t {
	objectbase_t base;
	string_char string[128];
} string_128_t;

typedef u64 string_128_id;

// string_256_t : a 256 char string
typedef struct string_256_t {
	objectbase_t base;
	string_char string[256];
} string_256_t;

typedef u64 string_256_id;

// user_t : user struct
typedef struct user_t {
	objectbase_t base;
	string_128_id username;
	string_128_id email;
	string_128_id password;
	string_128_id verify;
	string_128_id secret;
} user_t;

typedef u64 user_id;

// usersession_t : a (hopefully) 1:1 mapping with the user table, everyone only gets one session
typedef struct usersession_t {
    objectbase_t base;
    string_128_id secret;
} usersession_t;

// recipe_id : the pk for a recipe item (just a u64 that's the array offset)
typedef u64 recipe_id;

// ingredient_t : a single ingredient (it's just a double pointer)
typedef struct ingredient_t {
	objectbase_t base;
	recipe_id recipe_id;
	string_128_id string_id;
} ingredient_t;

typedef u64 ingredient_id;

// step_t : a single step for a recipe
typedef struct step_t {
	objectbase_t base;
	recipe_id recipe_id;
	string_128_id string_id;
} step_t;

// tag_t : a single tag in the system (it's just a double pointer)
typedef struct tag_t {
	objectbase_t base;
	recipe_id recipe_id;
	string_128_id string_id;
} tag_t;

// recipe_t : recipe struct
typedef struct recipe_t {
	objectbase_t base;

	user_id user_id;

	string_128_id name_id;

	int prep_time;
	int cook_time;
	int servings;

	// ingredients, steps, and tags are "joined" to this record on "recipe_id"

	string_256_id note_id;
} recipe_t;

#define OBJECT_MAGIC   ("Chrzanowski Recipe V1")

#define OBJECT_VERSION (0x000001) // 2021-10-13 01:44:40

#define DEFAULT_RECORD_CNT (32)

typedef enum RECORD_TYPE {
	RT_USER,
	RT_RECIPE,
	RT_STEP,
	RT_INGREDIENT,
	RT_TAG,
	RT_STRING128,
	RT_STRING256,
	RT_TOTAL
} RECORD_TYPE;

typedef struct lump_t {
	RECORD_TYPE type;
	u64 start;      // starting byte from the base pointer
	u64 recsize;    // size of each record, in bytes
	u64 allocated;  // count of allocated records
	u64 used;       // count of used records (highest index written)
} lump_t;

// storeheader_t : the header for the file
typedef struct storeheader_t {
	char magic[32];
	u64 version;
	u64 size;
	lump_t lumps[RT_TOTAL];
} storeheader_t;

// NOTE (Brian): THIS DOES NOT GET STORED

typedef struct handle_t {
	storeheader_t header;
	char *fname;
	void *ptrs[RT_TOTAL];
} handle_t;

#endif // OBJECTS_H

