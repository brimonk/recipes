#ifndef OBJECTS_H
#define OBJECTS_H

#include "common.h"

typedef struct objectbase_t {
	u64 id;
	u64 flags;
} objectbase_t;

// user_t : user struct
typedef struct user_t {
	objectbase_t base;
	char username[64];
	char email[64];
	char password[64];
	char verify[64];
	char secret[64];
} user_t;

// recipe_t : recipe struct
typedef struct recipe_t {
	objectbase_t base;
	int cook_time;
	int prep_time;
	int servings;
	char ingredients[100][100];
	char steps[100][100];
	char tags[100][100];
	char notes[200];
} recipe_t;

#define OBJECT_MAGIC   (0xf00d4all)

#define OBJECT_VERSION (0x000001) // 2021-10-13 01:44:40

#define DEFAULT_RECORD_CNT     (32)

typedef enum RECORD_TYPE {
	RT_USER,
	RT_RECIPE,
	RT_TOTAL
} RECORD_TYPE;

typedef struct lump_t {
	RECORD_TYPE type;
	u64 size;  // size of each record, in bytes
	u64 cnt;   // count of allocated records
	u64 len;   // count of used records (highest index written)
	u64 start; // starting byte from the base pointer
} lump_t;

// storeheader_t : the header for the file
typedef struct storeheader_t {
	u32 header;
	u32 version;
	u64 size;
	lump_t lumps[RT_TOTAL];
} storeheader_t;

// NOTE (Brian): THIS DOES NOT GET STORED

typedef struct handle_t {
	void *ptr;
	int fd;
	u64 size;
} handle_t;

#endif // OBJECTS_H

