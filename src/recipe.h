#ifndef RECIPE_H
#define RECIPE_H

#include "common.h"
#include "objects.h"

#include "mongoose.h"

#define MAX_LIST_RESULTS (100)

// Recipe : this is the object the user will, effectively, send us
struct Recipe {
	recipe_id id;
	char *name;

    char *prep_time;
    char *cook_time;
    char *servings;

	char *ingredients[128];
	size_t ingredients_len;

	char *steps[128];
	size_t steps_len;

	char *tags[64];
	size_t tags_len;

	char *note;
};

// RecipeResultRecord : one result from a RecipeSearch
struct RecipeResultRecord {
    recipe_id id;
	char name[128];
    char prep_time[128];
	char cook_time[128];
	char servings[128];
};

// RecipeResultRecords : an aggregation of results from a search
struct RecipeResultRecords {
    s64 matches;
	struct RecipeResultRecord *records;
	size_t records_len, records_cap;
};

// SearchQuery : contains the properties you're allowed to search for
struct SearchQuery {
	size_t page_size;   // page size (up to 100, common sizes are 10, 20, 25, 50, 100)
	size_t page_number; // page number [1 - N)
	char *text; // the text of a user's search
};

// recipe_api_post : endpoint, POST - /api/v1/recipe
int recipe_api_post(struct mg_connection *conn, struct mg_http_message *hm);

// recipe_api_put : endpoint, PUT - /api/v1/recipe/{id}
int recipe_api_put(struct mg_connection *conn, struct mg_http_message *hm);

// recipe_api_get : endpoint, GET - /api/v1/recipe/{id}
int recipe_api_get(struct mg_connection *conn, struct mg_http_message *hm);

// recipe_api_delete : endpoint, DELETE - /api/v1/recipe/{id}
int recipe_api_delete(struct mg_connection *conn, struct mg_http_message *hm);

// recipe_api_getlist : endpoint, GET - /api/v1/recipe/list
int recipe_api_getlist(struct mg_connection *conn, struct mg_http_message *hm);

#endif

