// Brian Chrzanowski
// 2021-10-14 22:48:02
//
// All of the recipe objects go in here.
//
// TODO (Brian)
//
// Recipe Validation:
//
// We're going to need to not only perform some backend validation, and give it back to the
// user, but we're going to need to perform heaps of NULL checking mostly because we don't know if
// someone's going to directly try to use the REST API to do CRUD operations.
//
// What is required:
//   name
//   prep_time
//   cook_time
//   servings
//   ingredients
//   steps
//   tags
//
// Not Required:
//   note

#include "common.h"

#include <sodium.h>
#include <jansson.h>

#include "mongoose.h"

#include "recipe.h"
#include "objects.h"
#include "store.h"

// recipe_free : frees all of the data in the recipe object
void recipe_free(struct Recipe *recipe);

// recipe_add : adds a recipe to the backing store
s64 recipe_add(struct Recipe *recipe);

// recipe_get : fetches a recipe object from the store, and parses it
struct Recipe *recipe_get(s64 id);

// recipe_delete : marks the recipe at 'id', as unallocated
s64 recipe_delete(s64 id);

// recipe_validation : returns non-zero if the input object is invalid
int recipe_validation(struct Recipe *recipe);

// recipe_from_json : converts a JSON string into a Recipe
static struct Recipe *recipe_from_json(char *s);

// recipe_to_json : converts a Recipe to a JSON string
static char *recipe_to_json(struct Recipe *recipe);

// search_results_to_json : converts the search records from memory to JSON
char *search_results_to_json(struct RecipeResultRecords *records);

// search_matches : returns true if the recipe matches the text, false if it doesn't
int search_matches(recipe_id id, char *text);

// recipe_search : fills out a search structure with results
struct RecipeResultRecords *recipe_search(struct SearchQuery *search);

// search_results_free : frees the recipe result search records
void search_results_free(struct RecipeResultRecords *records);

// NOTE (Brian): I'm putting this here because I'm not sure where else it's really going to be used.
// Feel free to move it in the future

// recipe_api_post : endpoint, POST - /api/v1/recipe
int recipe_api_post(struct mg_connection *conn, struct mg_http_message *hm)
{
	recipe_id id;
	struct Recipe *recipe;
	char *body;

	body = strndup(hm->body.ptr, hm->body.len);

	recipe = recipe_from_json(body);

	free(body);

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

	mg_http_reply(conn, 200, NULL, "{\"id\":%lld}", id);

	recipe_free(recipe);

	return 0;
}

// recipe_api_put : endpoint, PUT - /api/v1/recipe/{id}
int recipe_api_put(struct mg_connection *conn, struct mg_http_message *hm)
{
	recipe_id id;
	s64 rc;
	struct Recipe *recipe;
	char *url;
	char *json;

	url = strndup(hm->uri.ptr, hm->uri.len);

    rc = sscanf(url, "/api/v1/recipe/%lld", &id);
    assert(rc == 1);

	free(url);

	json = strndup(hm->body.ptr, hm->body.len);
	if (json == NULL) { // TODO (Brian): HTTP Error
		return -1;
	}

	recipe = recipe_from_json(json);

	free(json);

	if (recipe == NULL) { // TODO (Brian): HTTP Error
		ERR("couldn't parse recipe from json!\n");
		return -1;
	}

	if (recipe_validation(recipe) < 0) { // TODO (Brian): HTTP Error
		ERR("recipe record invalid!\n");
		return -1;
	}

	rc = recipe_delete(id);
	if (rc < 0) {
		ERR("couldn't nuke the recipe!\n");
		return -1;
	}

	id = recipe_add(recipe);
	if (id < 0) {
		ERR("couldn't make the new recipe!\n");
		return -1;
	}

	mg_http_reply(conn, 200, NULL, "{\"id\":%lld}", id);

	recipe_free(recipe);

	return 0;
}

// recipe_api_get : endpoint, GET - /api/v1/recipe/{id}
int recipe_api_get(struct mg_connection *conn, struct mg_http_message *hm)
{
	recipe_id id;
	struct Recipe *recipe;
	char *url;
	char *json;
    int rc;

	url = strndup(hm->uri.ptr, hm->uri.len);

    rc = sscanf(url, "/api/v1/recipe/%lld", &id);
    assert(rc == 1);

	free(url);

	recipe = recipe_get(id);
	if (recipe == NULL) { // TODO (Brian): return HTTP error
		ERR("couldn't fetch the recipe from the database!\n");
		return -1;
	}

	json = recipe_to_json(recipe);
	if (json == NULL) {
		ERR("couldn't convert the recipe to JSON\n");
		return -1;
	}

	mg_http_reply(conn, 200, NULL, "%s", json);

	free(json);

	return 0;
}

// recipe_api_getlist : endpoint, GET - /api/v1/recipe/list
int recipe_api_getlist(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct SearchQuery query;
	char *json;
	char *qstr;
	char *s, *k, *v;

	memset(&query, 0, sizeof query);

	query.page_size = 20;
	query.page_number = 0;
	query.text = NULL;

	qstr = strndup(hm->query.ptr, hm->query.len);

	for (s = strtok(qstr, "&"); s; s = strtok(NULL, "&")) {
		k = s;
		v = strchr(k, '=');

		if (v == NULL)
			continue;

		*v++ = 0;

		if (streq(k, "siz")) {
			query.page_size = atol(v);
		} else if (streq(k, "num")) {
			query.page_number = atol(v);
		} else if (streq(k, "q")) {
			query.text = v; // WARN (Brian): will this be okay if we defer freeing 'qstr'?
		} else {
			// TODO (Brian): handle bad parameters here
		}
	}

	struct RecipeResultRecords *results;

	results = recipe_search(&query);

	json = search_results_to_json(results);

	search_results_free(results);

	mg_http_reply(conn, 200, NULL, "%s", json);

	free(qstr);
	free(json);

	return 0;
}

// recipe_api_delete : endpoint, DELETE - /api/v1/recipe/{id}
int recipe_api_delete(struct mg_connection *conn, struct mg_http_message *hm)
{
    recipe_id id;
    char *url;
    int rc;

    url = strndup(hm->uri.ptr, hm->uri.len);

    rc = sscanf(url, "/api/v1/recipe/%lld", &id);
    assert(rc == 1);

    free(url);

    rc = recipe_delete(id);
    if (rc < 0) {
        // TODO (Brian): return HTTP error
        ERR("couldn't delete the recipe from the database!\n");
        return -1;
    }

	mg_http_reply(conn, 200, NULL, NULL);

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

	string128 = store_addobj(RT_STRING128);
	strncpy(string128->string, recipe->name, sizeof(string128->string));
	record->name_id = string128->base.id;

    string128 = store_addobj(RT_STRING128);
    strncpy(string128->string, recipe->prep_time, sizeof(string128->string));
    record->prep_time_id = string128->base.id;

    string128 = store_addobj(RT_STRING128);
    strncpy(string128->string, recipe->cook_time, sizeof(string128->string));
    record->cook_time_id = string128->base.id;

    string128 = store_addobj(RT_STRING128);
    strncpy(string128->string, recipe->servings, sizeof(string128->string));
    record->servings_id = string128->base.id;

	if (recipe->note != NULL) {
		string256 = store_addobj(RT_STRING256);
		strncpy(string256->string, recipe->note, sizeof(string256->string));
		record->note_id = string256->base.id;
	}

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

    string128 = store_getobj(RT_STRING128, record->prep_time_id);
	recipe->prep_time = strndup(string128->string, sizeof(string128->string));

    string128 = store_getobj(RT_STRING128, record->cook_time_id);
	recipe->cook_time = strndup(string128->string, sizeof(string128->string));

    string128 = store_getobj(RT_STRING128, record->servings_id);
	recipe->servings = strndup(string128->string, sizeof(string128->string));

	string256 = store_getobj(RT_STRING256, record->note_id);
	if (string256 != NULL) {
		recipe->note = strndup(string256->string, sizeof(string256->string));
	}

    // NOTE (Brian): the wrapping NULL checks make sure the item is actually in use

	// aggregate all of the ingredients
	for (i = 1, len = store_getlen(RT_INGREDIENT); i <= len; i++) {
		ingredient = store_getobj(RT_INGREDIENT, i);
		if (ingredient != NULL && recipe->id == ingredient->recipe_id) {
			string128 = store_getobj(RT_STRING128, ingredient->string_id);

			recipe->ingredients[recipe->ingredients_len++] =
				strndup(string128->string, sizeof(string128->string));
		}
	}

	// aggregate all of the steps
	for (i = 1, len = store_getlen(RT_STEP); i <= len; i++) {
		step = store_getobj(RT_STEP, i);
		if (step != NULL && recipe->id == step->recipe_id) {
			string128 = store_getobj(RT_STRING128, step->string_id);

			recipe->steps[recipe->steps_len++] =
				strndup(string128->string, sizeof(string128->string));
		}
	}

	// aggregate all of the tags
	for (i = 1, len = store_getlen(RT_TAG); i <= len; i++) {
		tag = store_getobj(RT_TAG, i);
		if (tag != NULL && recipe->id == tag->recipe_id) {
			string128 = store_getobj(RT_STRING128, tag->string_id);

			recipe->tags[recipe->tags_len++] =
				strndup(string128->string, sizeof(string128->string));
		}
	}

	return recipe;
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

	for (i = 1, len = store_getlen(RT_INGREDIENT); i <= len; i++) {
		ingredient_t *ingredient;
		ingredient = store_getobj(RT_INGREDIENT, i);
		if (ingredient != NULL && recipe->base.id == ingredient->recipe_id) {
			store_freeobj(RT_STRING128, ingredient->string_id);
			store_freeobj(RT_INGREDIENT, i);
		}
	}

	for (i = 1, len = store_getlen(RT_STEP); i <= len; i++) {
		step_t *step;
		step = store_getobj(RT_STEP, i);
		if (step != NULL && recipe->base.id == step->recipe_id) {
			store_freeobj(RT_STRING128, step->string_id);
			store_freeobj(RT_STEP, i);
		}
	}

	for (i = 1, len = store_getlen(RT_TAG); i <= len; i++) {
		tag_t *tag;
		tag = store_getobj(RT_TAG, i);
		if (tag != NULL && recipe->base.id == tag->recipe_id) {
			store_freeobj(RT_STRING128, tag->string_id);
			store_freeobj(RT_TAG, i);
		}
	}

	store_freeobj(RT_STRING128, recipe->name_id);

	if (0 < recipe->note_id) {
		store_freeobj(RT_STRING256, recipe->note_id);
	}

	store_freeobj(RT_RECIPE, id);

	store_write();

	return 0;
}

// search_matches : returns true if the recipe matches the text, false if it doesn't
int search_matches(recipe_id id, char *text)
{
	recipe_t *recipe;
	string_128_t *name;
	int rc;

	if (text == NULL) {
		text = "";
	}

	if (strlen(text) == 0) {
		return 1;
	}

	recipe = store_getobj(RT_RECIPE, id);
	if (recipe == NULL) {
		return 0;
	}

	name = store_getobj(RT_STRING128, recipe->name_id);
	if (name == NULL) {
		return -1;
	}

	// NOTE (Brian): GNU doesn't have a strnstr function, so at some point we should probably
	// write our own.

	char *s;

	s = strndup(name->string, sizeof(name->string));

	rc = strstr(s, text) != NULL;

	free(s);

	return rc;
}

// recipe_search : fills out a search structure with results
struct RecipeResultRecords *recipe_search(struct SearchQuery *search)
{
	struct RecipeResultRecords *records;
	recipe_t *recipe;
	string_128_t *name;
	string_128_t *prep_time;
	string_128_t *cook_time;
	string_128_t *servings;
	size_t i;
	size_t len;
	size_t skip;
	size_t page_size;
	size_t page_number;

	assert(search != NULL);

	records = calloc(1, sizeof(*records));
	if (records == NULL) {
		return NULL;
	}

	page_size = search->page_size;
	page_number = search->page_number;

	skip = page_size * page_number;

	// NOTE (Brian):
	//
	// For performance speedups, we'll need to compute / store an index to walk instead. For the
	// moment, we sure do just check every single record. This really doesn't scale too well.
    //
    // It's also the case that at some point, we really want something that looks like bsearch, or
    // similar to find items from the hunks. We'd probably want something like this:
    //
    //     store_find(int type, s64 id, int (*func)(void *), void *param);
    //
    // Where the search criteria can be passed as the void *param. As of now, none of these things
    // are sorted in any way. It could be useful to basically create indexes for every column, so
    // you could maybe have something like this:
    //
    //     store_find_idx(int type, s64 id, int (*func)(void *), void *param, int idx);
    //
    // And that function could work just like bsearch, with possible search comparator functions for
    // the individual record types. That being said, I don't know how you'd do anything about the
    // integer indexes. We'd probably just have to assign the next highest int, and perform our
    // searches just like a database at that point.
    //
    // Basically, I haven't found any super satisfying ways to do this searching.

	// NOTE (Brian): Now we get to do our dynamic allocation, to pull back page_size records.
	records->records_cap = page_size;
	records->records = calloc(records->records_cap, sizeof(*records->records));

    for (i = 1, len = store_getlen(RT_RECIPE); i <= len; i++) {
        if (store_isused(RT_RECIPE, i) && search_matches((recipe_id)i, search->text)) {
            records->matches++;

            if (skip) {
                // we have to skip some records because we aren't on the first page
                skip--;
            } else if (records->records_len < records->records_cap) {
                // we can pull back this record
                struct RecipeResultRecord *result;

                // NOTE (Brian): these are pointer fetches, so they should always be pretty quick...
                recipe = store_getobj(RT_RECIPE, i);

                name = store_getobj(RT_STRING128, recipe->name_id);
                if (name == NULL) { // HOW THE FUCK
                    break;
                }

                prep_time = store_getobj(RT_STRING128, recipe->prep_time_id);
                if (name == NULL) { // HOW THE FUCK
                    break;
                }

                cook_time = store_getobj(RT_STRING128, recipe->cook_time_id);
                if (name == NULL) { // HOW THE FUCK
                    break;
                }

                servings = store_getobj(RT_STRING128, recipe->servings_id);
                if (name == NULL) { // HOW THE FUCK
                    break;
                }

                result = records->records + records->records_len++;

                result->id = recipe->base.id;

                strncpy(result->name, name->string, sizeof(result->name));
                strncpy(result->prep_time, prep_time->string, sizeof(result->prep_time));
                strncpy(result->cook_time, cook_time->string, sizeof(result->cook_time));
                strncpy(result->servings, servings->string, sizeof(result->servings));
            }
        }
    }

	return records;
}

// recipe_validation : returns non-zero if the input object is invalid
int recipe_validation(struct Recipe *recipe)
{
	size_t i;

	if (recipe == NULL) {
		return -1;
	}

	if (recipe->name == NULL || 128 <= strlen(recipe->name)) {
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

	if (recipe->note != NULL && 256 <= strlen(recipe->note)) {
		return -1;
	}

	if (ARRSIZE(recipe->ingredients) < recipe->ingredients_len) {
		return -1;
	}
	for (i = 0; i < recipe->ingredients_len; i++) {
		if (128 <= strlen(recipe->ingredients[i])) {
			return -1;
		}
	}

	if (ARRSIZE(recipe->steps) < recipe->steps_len) {
		return -1;
	}
	for (i = 0; i < recipe->steps_len; i++) {
		if (128 <= strlen(recipe->steps[i])) {
			return -1;
		}
	}

	if (ARRSIZE(recipe->tags) < recipe->tags_len) {
		return -1;
	}
	for (i = 0; i < recipe->tags_len; i++) {
		if (128 <= strlen(recipe->tags[i])) {
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
	if (!json_is_string(prep_time)) {
		json_decref(root);
		return NULL;
	}

	cook_time = json_object_get(root, "cook_time");
	if (!json_is_string(cook_time)) {
		json_decref(root);
		return NULL;
	}

	servings = json_object_get(root, "servings");
	if (!json_is_string(servings)) {
		json_decref(root);
		return NULL;
	}

	// NOTE (Brian): notes in a recipe should be optional.
	note = json_object_get(root, "note");

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
	recipe->prep_time = strdup(json_string_value(prep_time));
	recipe->cook_time = strdup(json_string_value(cook_time));
	recipe->servings = strdup(json_string_value(servings));
	recipe->note = strdup_null((char *)json_string_value(note));

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
		"{s:I, s:s, s:s, s:s, s:s, s:s?, s:o, s:o, s:o}",
		"id", (json_int_t)recipe->id,
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

// search_results_to_json : converts the search records from memory to JSON
char *search_results_to_json(struct RecipeResultRecords *records)
{
	size_t i;
	json_t *array;
	json_t *object;
	char *s;

	array = json_array();
	for (i = 0; i < records->records_len; i++) {
		object = json_object();

		json_object_set_new(object, "id", json_integer(records->records[i].id));
		json_object_set_new(object, "name", json_string(records->records[i].name));
		json_object_set_new(object, "prep_time", json_string(records->records[i].prep_time));
		json_object_set_new(object, "cook_time", json_string(records->records[i].cook_time));
		json_object_set_new(object, "servings", json_string(records->records[i].servings));

		json_array_append_new(array, object);
	}

    object = json_object();

    json_object_set_new(object, "total", json_integer(records->matches));
    json_object_set_new(object, "results", array);

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
		free(recipe->prep_time);
		free(recipe->cook_time);
		free(recipe->servings);
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

// search_results_free : frees the recipe result search records
void search_results_free(struct RecipeResultRecords *records)
{
	assert(records != NULL);

	free(records->records);
	free(records);
}

