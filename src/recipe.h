#ifndef RECIPE_H
#define RECIPE_H

#include "common.h"
#include "objects.h"

#define MAX_LIST_RESULTS (100)

// Recipe : this is the object the user will, effectively, send us
struct Recipe {
	recipe_id id;
	char *name;

	int cook_time;
	int prep_time;
	int servings;

	char *ingredients[128];
	size_t ingredients_len;

	char *steps[128];
	size_t steps_len;

	char *tags[64];
	size_t tags_len;

	char *note;
};

// RecipeSearchQuery : contains the properties you're allowed to search for
struct RecipeSearchQuery {
	size_t siz; // page size (up to 100, common sizes are 10, 20, 25, 50, 100)
	size_t num; // page number
	char *text; // the text of a user's search
};

struct RecipeSearchResult {
	recipe_id id;
};

// 
typedef struct RecipeSearch {
	recipe_id id;
} RecipeSearch;

// search_t : the results of a search query (we only search on one thing in this app)
typedef struct search_t {
	// request
	char query[BUFSMALL];

	int type; // record type (RT_*)

	// response
	u64 id[MAX_LIST_RESULTS]; // we support up to 100 response items
	size_t cnt;
} search_t;

// recipe_api_post : endpoint, POST - /api/v1/recipe
int recipe_api_post(struct http_request_s *req, struct http_response_s *res);

// recipe_api_put : endpoint, PUT - /api/v1/recipe/{id}
int recipe_api_put(struct http_request_s *req, struct http_response_s *res);

// recipe_api_get : endpoint, GET - /api/v1/recipe/{id}
int recipe_api_get(struct http_request_s *req, struct http_response_s *res);

// recipe_api_delete : endpoint, DELETE - /api/v1/recipe/{id}
int recipe_api_delete(struct http_request_s *req, struct http_response_s *res);

// recipe_api_getlist : endpoint, GET - /api/v1/recipe/list
int recipe_api_getlist(struct http_request_s *req, struct http_response_s *res);

#endif

