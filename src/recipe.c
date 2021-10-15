// Brian Chrzanowski
// 2021-10-14 22:48:02
//
// All of the recipe objects go in here.

#include "common.h"

#include "httpserver.h"

#include <sodium.h>

#include "user.h"
#include "objects.h"

// from_json : converts a JSON string into a Recipe
static struct Recipe *from_json(char *s);

// to_json : converts a Recipe to a JSON string
static char *to_json(struct Recipe *recipe);

// recipe_api_post : endpoint, POST - /api/v1/recipe
int recipe_api_post(struct http_request_s *req, struct http_response_s *res)
{
	return 0;
}

// recipe_api_put : endpoint, PUT - /api/v1/recipe/{id}
int recipe_api_put(struct http_request_s *req, struct http_response_s *res)
{
	return 0;
}

// recipe_api_get : endpoint, GET - /api/v1/recipe/{id}
int recipe_api_get(struct http_request_s *req, struct http_response_s *res)
{
	return 0;
}

// recipe_api_delete : endpoint, DELETE - /api/v1/recipe/{id}
int recipe_api_delete(struct http_request_s *req, struct http_response_s *res)
{
	return 0;
}

// recipe_api_getlist : endpoint, GET - /api/v1/recipe/list
int recipe_api_getlist(struct http_request_s *req, struct http_response_s *res)
{
	return 0;
}

// from_json : converts a JSON string into a Recipe
static struct Recipe *from_json(char *s)
{
	struct Recipe *recipe = NULL;

	return recipe;
}

// to_json : converts a Recipe to a JSON string
static char *to_json(struct Recipe *recipe)
{
	return NULL;
}

