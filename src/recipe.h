#ifndef RECIPE_H_
#define RECIPE_H_

#include "common.h"
#include "objects.h"

#include "mongoose.h"

typedef struct DB_ChildTextRecord C_Ingredients;
typedef struct DB_ChildTextRecord C_Steps;
typedef struct DB_ChildTextRecord C_Tags;

// Recipe: the recipe structure
typedef struct Recipe {
	DB_Metadata metadata;

	// base fields
    char *name;
    char *prep_time;
    char *cook_time;
    char *servings;
    char *notes;

	// child tables
	U_TextList *ingredients;
	U_TextList *steps;
	U_TextList *tags;
} Recipe;

// V_Recipe: the recipe search view
typedef struct V_Recipe {
	DB_Metadata metadata;
	char *name;
	char *prep_time;
	char *cook_time;
	char *servings;
} V_Recipe;

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

#endif // RECIPE_H_
