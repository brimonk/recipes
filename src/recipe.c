// Brian Chrzanowski
// 2021-10-14 22:48:02
//
// All of the recipe objects go in here.

#include "common.h"

#include "httpserver.h"

#include <sodium.h>
#include <math.h>
#include <jansson.h>

#include "recipe.h"
#include "objects.h"
#include "store.h"

// recipe_free : frees all of the data in the recipe object
void recipe_free(struct Recipe *recipe);

// recipe_add : adds a recipe to the backing store
s64 recipe_add(struct Recipe *recipe);

// recipe_get : fetches a recipe object from the store, and parses it
struct Recipe *recipe_get(s64 id);

// recipe_update : updates the recipe at id 'id', with 'recipe''s data
s64 recipe_update(s64 id, struct Recipe *recipe);

// recipe_delete : marks the recipe at 'id', as unallocated
s64 recipe_delete(s64 id);

// recipe_validation : returns non-zero if the input object is invalid
int recipe_validation(struct Recipe *recipe);

// recipe_from_json : converts a JSON string into a Recipe
static struct Recipe *recipe_from_json(char *s);

// recipe_to_json : converts a Recipe to a JSON string
static char *recipe_to_json(struct Recipe *recipe);

// recipe_api_post : endpoint, POST - /api/v1/recipe
int recipe_api_post(struct http_request_s *req, struct http_response_s *res)
{
	recipe_id id;
	struct Recipe *recipe;
	struct http_string_s body;
	char *json;
	char tbuf[BUFLARGE];

	body = http_request_body(req);

	json = strndup(body.buf, body.len);
	if (json == NULL) {
		// return http error
		return -1;
	}

	recipe = recipe_from_json(json);

	free(json);

	if (recipe == NULL) {
		ERR("couldn't parse recipe from json!\n");
		return -1;
	}

	if (recipe_validation(recipe) < 0) {
		ERR("recipe record invalid!\n");
		return -1;
	}

	id = recipe_add(recipe);
	if (id < 0) {
		ERR("couldn't save the recipe to the disk!\n");
		return -1;
	}

	snprintf(tbuf, sizeof tbuf, "{\"id\":%lld}", id);

	// fill out response information
	http_response_status(res, 200);

	http_response_body(res, tbuf, strlen(tbuf));

	http_respond(req, res);

	recipe_free(recipe);

	return 0;
}

// recipe_api_put : endpoint, PUT - /api/v1/recipe/{id}
int recipe_api_put(struct http_request_s *req, struct http_response_s *res)
{
	http_response_status(res, 400);
	http_respond(req, res);

	return 0;
}

// recipe_api_get : endpoint, GET - /api/v1/recipe/{id}
int recipe_api_get(struct http_request_s *req, struct http_response_s *res)
{
	recipe_id id;
	struct Recipe *recipe;
	struct http_string_s uri;
	char *url;
	char *json;
    int rc;

	uri = http_request_target(req);
	url = strndup(uri.buf, strchr(uri.buf, ' ') - uri.buf);

    rc = sscanf(url, "/api/v1/recipe/%lld", &id);
    assert(rc == 1);

	free(url);

	recipe = recipe_get(id);
	if (recipe == NULL) {
        // TODO (Brian): return HTTP error
		ERR("couldn't fetch the recipe from the database!\n");
		return -1;
	}

	json = recipe_to_json(recipe);
	if (json == NULL) {
		ERR("couldn't convert the recipe to JSON\n");
		return -1;
	}

	recipe_free(recipe);

	// fill out response information
	http_response_status(res, 200);

	http_response_body(res, json, strlen(json));

	http_respond(req, res);

	free(json);

	return 0;
}

// recipe_api_getlist : endpoint, GET - /api/v1/recipe/list
int recipe_api_getlist(struct http_request_s *req, struct http_response_s *res)
{
	// NOTE (Brian): parameters from the uri
	//
	// siz: page size, positive integer, x > 0
	// num: page number, positive integer, x > 0
	// qry: query, text. If not present, just iterates over the available recipes

#if 0
	struct http_string_s h_siz, h_num, h_qry;
	search_t search;

	memset(&search, 0, sizeof search);

	search.siz = 20;
	search.num = 0;

	h_siz = http_request_query(req, "siz");
	if (h_siz.buf != NULL) {
		search.siz = atol(h_siz.buf);
	}

	h_num = http_request_query(req, "num");
	if (h_num.buf != NULL) {
		search.num = atol(h_num.buf);
	}

	h_qry = http_request_query(req, "qry");
	if (h_qry.buf != NULL) {
		strncpy(search.query, h_qry.buf, sizeof(search.query));
	}
#endif

	return 0;
}

// recipe_api_delete : endpoint, DELETE - /api/v1/recipe/{id}
int recipe_api_delete(struct http_request_s *req, struct http_response_s *res)
{
    recipe_id id;
    struct http_string_s uri;
    char *url;
    int rc;

    uri = http_request_target(req);
    url = strndup(uri.buf, strchr(uri.buf, ' ') - uri.buf);

    rc = sscanf(url, "/api/v1/recipe/%lld", &id);
    assert(rc == 1);

    free(url);

    rc = recipe_delete(id);
    if (rc < 0) {
        // TODO (Brian): return HTTP error
        ERR("couldn't delete the recipe from the database!\n");
        return -1;
    }

    http_response_status(res, 200);
    http_respond(req, res);

	return 0;
}

// recipe_add : adds a recipe to the backing store
s64 recipe_add(struct Recipe *recipe)
{
	recipe_t *record;

	ingredient_t *ingredient;
	step_t *step;
	tag_t *tag;

	string_128_t *string128;
	string_256_t *string256;

	size_t i;

	record = store_addobj(RT_RECIPE);

#if 0
	record->user_id = recipe->user_id;
#endif

	record->prep_time = recipe->prep_time;
	record->cook_time = recipe->cook_time;
	record->servings = recipe->servings;

	string128 = store_addobj(RT_STRING128);
	strncpy(string128->string, recipe->name, sizeof(string128->string));
	record->name_id = string128->base.id;

	string256 = store_addobj(RT_STRING256);
	strncpy(string256->string, recipe->note, sizeof(string256->string));
	record->note_id = string256->base.id;

	if (recipe->note != NULL) {
		strncpy(string256->string, recipe->note, sizeof(string256->string));
	}

	for (i = 0; i < recipe->ingredients_len; i++) {
		string128 = store_addobj(RT_STRING128);
		strncpy(string128->string, recipe->ingredients[i], sizeof(string128->string));

		ingredient = store_addobj(RT_INGREDIENT);

		ingredient->string_id = string128->base.id;
		ingredient->recipe_id = record->base.id;
	}

	for (i = 0; i < recipe->steps_len; i++) {
		string128 = store_addobj(RT_STRING128);
		strncpy(string128->string, recipe->steps[i], sizeof(string128->string));

		step = store_addobj(RT_STEP);

		step->string_id = string128->base.id;
		step->recipe_id = record->base.id;
	}

	for (i = 0; i < recipe->tags_len; i++) {
		string128 = store_addobj(RT_STRING128);
		strncpy(string128->string, recipe->tags[i], sizeof(string128->string));

		tag = store_addobj(RT_TAG);

		tag->string_id = string128->base.id;
		tag->recipe_id = record->base.id;
	}

	store_write();

	return record->base.id;
}

// recipe_get : fetches a recipe object from the store, and parses it
struct Recipe *recipe_get(s64 id)
{
	struct Recipe *recipe;
	recipe_t *record;

	ingredient_t *ingredient;
	step_t *step;
	tag_t *tag;

	string_128_t *string128;
	string_256_t *string256;

	size_t i, len;

	record = store_getobj(RT_RECIPE, id);
    if (record == NULL) {
        return NULL;
    }

	recipe = calloc(1, sizeof(*recipe));
	if (recipe == NULL) {
		return NULL;
	}

	recipe->id = record->base.id;

	string128 = store_getobj(RT_STRING128, record->name_id);
	recipe->name = strndup(string128->string, sizeof(string128->string));

	recipe->prep_time = record->prep_time;
	recipe->cook_time = record->cook_time;
	recipe->servings = record->servings;

	string256 = store_getobj(RT_STRING256, record->note_id);
	recipe->note = strndup(string256->string, sizeof(string256->string));

	// aggregate all of the ingredients
	for (i = 0, len = store_getlen(RT_INGREDIENT); i < len; i++) {
		ingredient = store_getobj(RT_INGREDIENT, i);
		if (recipe->id == ingredient->recipe_id) {
			string128 = store_getobj(RT_STRING128, ingredient->string_id);

			recipe->ingredients[recipe->ingredients_len++] =
				strndup(string128->string, sizeof(string128->string));
		}
	}

	// aggregate all of the steps
	for (i = 0, len = store_getlen(RT_STEP); i < len; i++) {
		step = store_getobj(RT_STEP, i);
		if (recipe->id == step->recipe_id) {
			string128 = store_getobj(RT_STRING128, step->string_id);

			recipe->steps[recipe->steps_len++] =
				strndup(string128->string, sizeof(string128->string));
		}
	}

	// aggregate all of the tags
	for (i = 0, len = store_getlen(RT_TAG); i < len; i++) {
		tag = store_getobj(RT_TAG, i);
		if (recipe->id == tag->recipe_id) {
			string128 = store_getobj(RT_STRING128, tag->string_id);

			recipe->tags[recipe->tags_len++] =
				strndup(string128->string, sizeof(string128->string));
		}
	}

	return recipe;
}

// recipe_update : updates the recipe at id 'id', with 'recipe''s data
s64 recipe_update(s64 id, struct Recipe *recipe)
{
	return 0;
}

// recipe_delete : marks the recipe at 'id', as unallocated
s64 recipe_delete(s64 id)
{
	// NOTE (Brian): deleting any objects in the system is SUPER easy. You just scan across the
	// table, and you mark those slots as free.

	size_t i, len;
	recipe_t *recipe;

	recipe = store_getobj(RT_RECIPE, id);
	if (recipe == NULL) {
		return -1;
	}

	for (i = 0, len = store_getlen(RT_INGREDIENT); i < len; i++) {
		ingredient_t *ingredient;
		ingredient = store_getobj(RT_INGREDIENT, i);
		if (recipe->base.id == ingredient->base.id) {
			store_freeobj(RT_INGREDIENT, i);
		}
	}

	for (i = 0, len = store_getlen(RT_STEP); i < len; i++) {
		step_t *step;
		step = store_getobj(RT_STEP, i);
		if (recipe->base.id == step->base.id) {
			store_freeobj(RT_STEP, i);
		}
	}

	for (i = 0, len = store_getlen(RT_TAG); i < len; i++) {
		tag_t *tag;
		tag = store_getobj(RT_TAG, i);
		if (recipe->base.id == tag->base.id) {
			store_freeobj(RT_TAG, i);
		}
	}

	store_freeobj(RT_STRING128, recipe->name_id);
	store_freeobj(RT_STRING256, recipe->note_id);

	store_freeobj(RT_RECIPE, id);

	store_write();

	return 0;
}

// recipe_search : fills out a search structure with results
int recipe_search(search_t *search)
{
#if 0
	recipe_t *recipe;
	tag_t *tag;
	string_128_t *name, *tagtext;
	size_t i, rlen, tlen;
	size_t skip;

	assert(search->type == RT_RECIPE);

	skip = search->siz * search->num;

	rlen = store_getlen(RT_RECIPE);
	tlen = store_getlen(RT_TAG);

	for (i = 0; i < rlen && search->cnt < ARRSIZE(search->id); i++) {
		recipe = store_getobj(RT_RECIPE, i);
		assert(recipe != NULL);

		name = store_getobj(RT_STRING128, recipe->name_id);
		assert(name != NULL);

		if (strstr(name->string, search->query) != NULL) {
			search->id[search->cnt++] = recipe->base.id;
		}
	}
#endif

	return 0;
}

// recipe_validation : returns non-zero if the input object is invalid
int recipe_validation(struct Recipe *recipe)
{
	size_t i;

	if (recipe == NULL) {
		return -1;
	}

	if (recipe->name == NULL || 128 < strlen(recipe->name)) {
		return -1;
	}

	if (recipe->prep_time <= 0) {
		return -1;
	}

	if (recipe->cook_time <= 0) {
		return -1;
	}

	if (recipe->servings <= 0) {
		return -1;
	}

	if (ARRSIZE(recipe->ingredients) < recipe->ingredients_len) {
		return -1;
	}
	for (i = 0; i < recipe->ingredients_len; i++) {
		if (128 < strlen(recipe->ingredients[i])) {
			return -1;
		}
	}

	if (ARRSIZE(recipe->steps) < recipe->steps_len) {
		return -1;
	}
	for (i = 0; i < recipe->steps_len; i++) {
		if (128 < strlen(recipe->steps[i])) {
			return -1;
		}
	}

	if (ARRSIZE(recipe->tags) < recipe->tags_len) {
		return -1;
	}
	for (i = 0; i < recipe->tags_len; i++) {
		if (128 < strlen(recipe->tags[i])) {
			return -1;
		}
	}

	return 0;
}

// recipe_from_json : converts a JSON string into a Recipe
static struct Recipe *recipe_from_json(char *s)
{
	struct Recipe *recipe;
	json_t *root;
	json_error_t error;
	size_t i;

	root = json_loads(s, 0, &error);
	if (root == NULL) {
		return NULL;
	}

	if (!json_is_object(root)) {
		json_decref(root);
		return NULL;
	}

	json_t *name, *prep_time, *cook_time, *servings, *note;

	// get all of the regular values first
	name = json_object_get(root, "name");
	if (!json_is_string(name)) {
		json_decref(root);
		return NULL;
	}

	prep_time = json_object_get(root, "prep_time");
	if (!json_is_integer(prep_time)) {
		json_decref(root);
		return NULL;
	}

	cook_time = json_object_get(root, "cook_time");
	if (!json_is_integer(cook_time)) {
		json_decref(root);
		return NULL;
	}

	servings = json_object_get(root, "servings");
	if (!json_is_integer(servings)) {
		json_decref(root);
		return NULL;
	}

	note = json_object_get(root, "note");
	if (!json_is_string(note)) {
		json_decref(root);
		return NULL;
	}

	json_t *ingredients;
	json_t *steps;
	json_t *tags;

	ingredients = json_object_get(root, "ingredients");
	if (!json_is_array(ingredients) || json_array_size(ingredients) > ARRSIZE(recipe->ingredients)) {
		json_decref(root);
		return NULL;
	}

	steps = json_object_get(root, "steps");
	if (!json_is_array(steps) || json_array_size(steps) > ARRSIZE(recipe->steps)) {
		json_decref(root);
		return NULL;
	}

	tags = json_object_get(root, "tags");
	if (!json_is_array(tags) || json_array_size(tags) > ARRSIZE(recipe->tags)) {
		json_decref(root);
		return NULL;
	}

	recipe = calloc(1, sizeof(*recipe));
	if (recipe == NULL) {
		json_decref(root);
		return NULL;
	}

	// now that we have handles to all of the pieces of our recipe object, we should be
	// able to take all of those, and get them into our regular C structure
	recipe->name = strdup(json_string_value(name));
	recipe->prep_time = json_integer_value(prep_time);
	recipe->cook_time = json_integer_value(cook_time);
	recipe->servings = json_integer_value(servings);
	recipe->note = strdup(json_string_value(note));

	for (i = 0; i < json_array_size(ingredients); i++) {
		json_t *ingredient;
		ingredient = json_array_get(ingredients, i);

		// TODO (Brian): What the hell do we do if / when we get an array element that isn't a
		// string? That's a thinker.

		recipe->ingredients[i] = strdup(json_string_value(ingredient));
	}

	recipe->ingredients_len = i;

	for (i = 0; i < json_array_size(steps); i++) {
		json_t *step;
		step = json_array_get(steps, i);

		// TODO (Brian): What the hell do we do if / when we get an array element that isn't a
		// string? That's a thinker.

		recipe->steps[i] = strdup(json_string_value(step));
	}

	recipe->steps_len = i;

	for (i = 0; i < json_array_size(tags); i++) {
		json_t *tag;
		tag = json_array_get(tags, i);

		// TODO (Brian): What the hell do we do if / when we get an array element that isn't a
		// string? That's a thinker.

		recipe->tags[i] = strdup(json_string_value(tag));
	}

	recipe->tags_len = i;

	json_decref(root);

	return recipe;
}

// recipe_to_json : converts a Recipe to a JSON string
static char *recipe_to_json(struct Recipe *recipe)
{
	json_t *object;
	json_t *ingredients;
	json_t *steps;
	json_t *tags;
	json_error_t error;
	char *s;
	size_t i;

	// we have to setup the arrays of junk first

	ingredients = json_array();
	for (i = 0; i < recipe->ingredients_len; i++) {
		json_array_append_new(ingredients, json_string(recipe->ingredients[i]));
	}

	steps = json_array();
	for (i = 0; i < recipe->steps_len; i++) {
		json_array_append_new(steps, json_string(recipe->steps[i]));
	}

	tags = json_array();
	for (i = 0; i < recipe->tags_len; i++) {
		json_array_append_new(tags, json_string(recipe->tags[i]));
	}

	object = json_pack_ex(
		&error, 0,
		"{s:s, s:i, s:i, s:i, s:s, s:o, s:o, s:o}",
		"name", recipe->name,
		"prep_time", recipe->prep_time,
		"cook_time", recipe->cook_time,
		"servings", recipe->servings,
		"note", recipe->note,
		"ingredients", ingredients,
		"steps", steps,
		"tags", tags
	);

	if (object == NULL) {
		fprintf(stderr, "%s %d\n", error.text, error.position);
	}

	s = json_dumps(object, 0);

	json_decref(object);

	return s;
}

// recipe_free : frees all of the data in the recipe object
void recipe_free(struct Recipe *recipe)
{
	size_t i;

	// NOTE (Brian): this actually frees the in-memory, dynamic version of a recipe, not the
	// version that's stored to disk

	if (recipe) {
		free(recipe->name);
		free(recipe->note);

		for (i = 0; i < ARRSIZE(recipe->ingredients) && recipe->ingredients[i]; i++) {
			free(recipe->ingredients[i]);
		}

		for (i = 0; i < ARRSIZE(recipe->steps) && recipe->steps[i]; i++) {
			free(recipe->steps[i]);
		}

		for (i = 0; i < ARRSIZE(recipe->tags) && recipe->tags[i]; i++) {
			free(recipe->tags[i]);
		}

		free(recipe);
	}
}

