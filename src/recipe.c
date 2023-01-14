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

#define _GNU_SOURCE
#include "common.h"

#include <sodium.h>
#include <jansson.h>

#include "mongoose.h"
#include "sqlite3.h"

#include "recipe.h"
#include "objects.h"

extern sqlite3 *DATABASE;

// recipe_free : frees all of the data in the recipe object
void recipe_free(struct Recipe *recipe);

// recipe_get_by_id : fetches a recipe object from the store, and parses it
struct Recipe *recipe_get_by_id(char *id);

// recipe_insert : adds a recipe to the backing store
int recipe_insert(struct Recipe *recipe);

// recipe_update: updates the recipe in the database
int recipe_update(Recipe *recipe);

// recipe_delete : marks the recipe at 'id', as unallocated
int recipe_delete(char *id);

// recipe_validation : returns non-zero if the input object is invalid
int recipe_validation(struct Recipe *recipe);

// recipe_from_json : converts a JSON string into a Recipe
static struct Recipe *recipe_from_json(char *s);

// recipe_to_json : converts a Recipe to a JSON string
static char *recipe_to_json(struct Recipe *recipe);

// recipe_search : fills out a search structure with results
json_t *recipe_search(char *query, size_t page_size, size_t page_number);

// NOTE (Brian): I'm putting this here because I'm not sure where else it's really going to be used.
// Feel free to move it in the future

// recipe_api_post : endpoint, POST - /api/v1/recipe
int recipe_api_post(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct Recipe *recipe;
	char *body;
	int rc;

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

	rc = recipe_insert(recipe);
	if (rc < 0) {
		ERR("couldn't save the recipe to the disk!\n");
		return -1;
	}

	mg_http_reply(conn, 200, NULL, "{\"id\":\"%s\"}", recipe->metadata.id);

	recipe_free(recipe);

	return 0;
}

// recipe_api_put : endpoint, PUT - /api/v1/recipe/{id}
int recipe_api_put(struct mg_connection *conn, struct mg_http_message *hm)
{
	s64 rc;
	struct Recipe *recipe;
	char *url;
	char *json;
	char id[128] = {0};

	url = strndup(hm->uri.ptr, hm->uri.len);

	rc = sscanf(url, "/api/v1/recipe/%127s", id);
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

	rc = recipe_update(recipe);
	if (rc < 0) {
		ERR("couldn't update the recipe!\n");
		return -1;
	}

	mg_http_reply(conn, 200, NULL, "{\"id\":\"%s\"}", recipe->metadata.id);

	recipe_free(recipe);

	return 0;
}

// recipe_api_get : endpoint, GET - /api/v1/recipe/{id}
int recipe_api_get(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct Recipe *recipe;
	char *url;
	char *json;
	int rc;
	char id[128] = {0};

	url = strndup(hm->uri.ptr, hm->uri.len);

	rc = sscanf(url, "/api/v1/recipe/%127s", id);
	assert(rc == 1);

	free(url);

	recipe = recipe_get_by_id(id);
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
	recipe_free(recipe);

	return 0;
}

// recipe_api_getlist : endpoint, GET - /api/v1/recipe/list
int recipe_api_getlist(struct mg_connection *conn, struct mg_http_message *hm)
{
    char *query = NULL;
	char tbuf[BUFSMALL];
    size_t siz, num;
	int rc;

    siz = 20;
    num = 0;

	rc = mg_http_get_var(&hm->query, "siz", tbuf, sizeof tbuf);
	if (rc >= 0 && isdigit(tbuf[0])) { siz = atol(tbuf); }

	rc = mg_http_get_var(&hm->query, "num", tbuf, sizeof tbuf);
	if (rc >= 0 && isdigit(tbuf[0])) { num = atol(tbuf); }

	rc = mg_http_get_var(&hm->query, "q", tbuf, sizeof tbuf);
	if (rc >= 0 && isdigit(tbuf[0])) { query = tbuf; }

	json_t *json = recipe_search(query, siz, num);
    if (json == NULL) {
        ERR("search couldn't be performed!\n");
    }

	char *json_str = json_dumps(json, JSON_SORT_KEYS|JSON_COMPACT);

	mg_http_reply(conn, 200, NULL, "%s", json_str);

	free(json_str);
	json_decref(json);

	return 0;
}

// recipe_api_delete : endpoint, DELETE - /api/v1/recipe/{id}
int recipe_api_delete(struct mg_connection *conn, struct mg_http_message *hm)
{
	char *url;
	int rc;
	char id[128];

	url = strndup(hm->uri.ptr, hm->uri.len);

	rc = sscanf(url, "/api/v1/recipe/%s", id);
	assert(rc == 1);

	free(url);

	rc = recipe_delete(id);
	if (rc < 0) {
		// TODO (Brian): return HTTP error
		ERR("couldn't delete the recipe from the database!\n");
		return -1;
	}

	mg_http_reply(conn, 200, "", "");

	return 0;
}

// recipe_insert: adds a recipe to the database. writes DB_Metadata into the recipe pointer after add
int recipe_insert(Recipe *recipe)
{
	sqlite3_stmt *stmt;
	int64_t rowid;
	int rc;

    sqlite3_exec(DATABASE, "begin transaction;", NULL, NULL, NULL);

	char *query =
		"insert into recipes (name, prep_time, cook_time, servings, notes) values (?, ?, ?, ?, ?);";

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		return -1;
	}

    sqlite3_bind_text(stmt, 1, (const char *)recipe->name,      -1, NULL);
    sqlite3_bind_text(stmt, 2, (const char *)recipe->prep_time, -1, NULL);
    sqlite3_bind_text(stmt, 3, (const char *)recipe->cook_time, -1, NULL);
    sqlite3_bind_text(stmt, 4, (const char *)recipe->servings,  -1, NULL);
    sqlite3_bind_text(stmt, 5, (const char *)recipe->notes,     -1, NULL);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    stmt = NULL;

    if (rc != SQLITE_DONE) { // deal with error
        sqlite3_finalize(stmt);
        return -1;
    }

    // get metadata so we can insert child tables
	rowid = sqlite3_last_insert_rowid(DATABASE);

	rc = db_load_metadata_from_rowid(&recipe->metadata, "recipes", rowid);
	if (rc < 0) {
        goto recipe_insert_fail;
	}

    rc = db_insert_textlist("ingredients", recipe->metadata.id, recipe->ingredients);
    if (rc < 0) {
        goto recipe_insert_fail;
    }

    db_insert_textlist("steps", recipe->metadata.id, recipe->steps);
    if (rc < 0) { // ?
        goto recipe_insert_fail;
    }

    db_insert_textlist("tags", recipe->metadata.id, recipe->tags);

    sqlite3_exec(DATABASE, "commit transaction;", NULL, NULL, NULL);

	return 0;

recipe_insert_fail:
    if (stmt != NULL) sqlite3_finalize(stmt);
    sqlite3_exec(DATABASE, "rollback transaction;", NULL, NULL, NULL);
    return -1;
}

// recipe_update: updates the recipe in the database
int recipe_update(Recipe *recipe)
{
	return 0;
}

// recipe_get_by_id : fetches a recipe object from the store by id, and parses it
struct Recipe *recipe_get_by_id(char *id)
{
	Recipe *recipe = calloc(1, sizeof(*recipe));
    if (recipe == NULL) {
        return NULL;
    }

    size_t query_sz;
    char *query;
    sqlite3_stmt *stmt;
    int rc;

    FILE *stream = open_memstream(&query, &query_sz);
    fprintf(stream, "select name, prep_time, cook_time, servings, notes from recipes where id = ?;");
    fclose(stream);

    rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(query);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, id, -1, NULL);

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        recipe->name      = strdup_null((char *)sqlite3_column_text(stmt, 0));
        recipe->prep_time = strdup_null((char *)sqlite3_column_text(stmt, 1));
        recipe->cook_time = strdup_null((char *)sqlite3_column_text(stmt, 2));
        recipe->servings  = strdup_null((char *)sqlite3_column_text(stmt, 3));
        recipe->notes     = strdup_null((char *)sqlite3_column_text(stmt, 4));
    }

    sqlite3_finalize(stmt);

    db_load_metadata_from_id(&recipe->metadata, "recipes", id);

    recipe->ingredients = db_get_textlist("ingredients", recipe->metadata.id);
    recipe->steps = db_get_textlist("ingredients", recipe->metadata.id);
    recipe->tags = db_get_textlist("ingredients", recipe->metadata.id);

    free(query);

	return recipe;
}

// recipe_delete : updates 'deleted_ts' on the given recipe, such that it is 'deleted'
int recipe_delete(char *id)
{
	// TODO (Brian) update the delete_ts
	return 0;
}

// recipe_search : fills out a search structure with results
json_t *recipe_search(char *query, size_t page_size, size_t page_number)
{
    // TODO (Brian) finish updating the 
    char *search_cols[] = { "id", "search_text", NULL };
    char *where_clause[] = { "search_text", "like", "?", NULL };
	char *search_bind_params[] = { COALESCE(query, ""), NULL };
    DB_Query search = {
        .pk_column = "id",
        .columns = search_cols,
        .table = "v_recipes",
		.where = where_clause,
		.bind = search_bind_params
    };

    char *results_cols[] = { "id", "name", "prep_time", "cook_time", "servings", NULL };
    DB_Query results = {
        .pk_column = "id",
        .columns = results_cols,
        .table = "ui_recipes",
    };

    UI_SearchQuery runme = {
        .search = search,
        .results = results,
        .page_size = page_size,
        .page_number = page_number
    };

	return db_search_to_json(&runme);
}

// recipe_validation : returns non-zero if the input object is invalid
int recipe_validation(struct Recipe *recipe)
{
#define VALID_PTR(P_)     if ((P_) == NULL) { return -1; }
#define VALID_STR(S_, L_) if ((S_) == NULL || (L_) <= strlen(S_)) { return -1; }

	VALID_PTR(recipe);
	VALID_PTR(recipe->name); VALID_STR(recipe->name, 100);
	VALID_PTR(recipe->prep_time); VALID_STR(recipe->prep_time, 100);
	VALID_PTR(recipe->cook_time); VALID_STR(recipe->cook_time, 100);
	VALID_PTR(recipe->servings); VALID_STR(recipe->servings, 100);
	VALID_PTR(recipe->notes); VALID_STR(recipe->notes, 100);

	for (U_TextList *curr = recipe->ingredients; curr; curr = curr->next) {
		VALID_PTR(curr); VALID_PTR(curr->text); VALID_STR(curr->text, 128);
	}

	for (U_TextList *curr = recipe->steps; curr; curr = curr->next) {
		VALID_PTR(curr); VALID_PTR(curr->text); VALID_STR(curr->text, 128);
	}

	for (U_TextList *curr = recipe->tags; curr; curr = curr->next) {
		VALID_PTR(curr); VALID_PTR(curr->text); VALID_STR(curr->text, 128);
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
	if (!json_is_array(ingredients)) {
		json_decref(root);
		return NULL;
	}

	steps = json_object_get(root, "steps");
	if (!json_is_array(steps)) {
		json_decref(root);
		return NULL;
	}

	tags = json_object_get(root, "tags");
	if (!json_is_array(tags)) {
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
	recipe->notes = strdup_null((char *)json_string_value(note));

	for (i = 0; i < json_array_size(ingredients); i++) {
		json_t *ingredient;
		ingredient = json_array_get(ingredients, i);

		// TODO (Brian): check that this is actually a string value

		u_textlist_append(&recipe->ingredients, strdup(json_string_value(ingredient)));
	}

	for (i = 0; i < json_array_size(steps); i++) {
		json_t *step;
		step = json_array_get(steps, i);

		// TODO (Brian): What the hell do we do if / when we get an array element that isn't a
		// string? That's a thinker.

		u_textlist_append(&recipe->steps, strdup(json_string_value(step)));
	}

	for (i = 0; i < json_array_size(tags); i++) {
		json_t *tag;
		tag = json_array_get(tags, i);

		// TODO (Brian): What the hell do we do if / when we get an array element that isn't a
		// string? That's a thinker.

		u_textlist_append(&recipe->tags, strdup(json_string_value(tag)));
	}

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
	size_t i, len;

	// we have to setup the arrays of junk first

	ingredients = json_array();
	for (i = 0, len = u_textlist_len(recipe->ingredients); i < len; i++) {
		json_array_append_new(ingredients, json_string(u_textlist_get(recipe->ingredients, i)));
	}

	steps = json_array();
	for (i = 0, len = u_textlist_len(recipe->steps); i < len; i++) {
		json_array_append_new(steps, json_string(u_textlist_get(recipe->steps, i)));
	}

	tags = json_array();
	for (i = 0, len = u_textlist_len(recipe->tags); i < len; i++) {
		json_array_append_new(tags, json_string(u_textlist_get(recipe->tags, i)));
	}

	object = json_pack_ex(
		&error, 0,
		"{s:s, s:s, s:s, s:s, s:s, s:s, s:s, s:s, s:s?, s:o, s:o, s:o}",
		"id", (json_int_t)recipe->metadata.id,
		"create_ts", (json_int_t)recipe->metadata.create_ts,
		"update_ts", (json_int_t)recipe->metadata.update_ts,
		"delete_ts", (json_int_t)recipe->metadata.delete_ts,
		"name", recipe->name,
		"prep_time", recipe->prep_time,
		"cook_time", recipe->cook_time,
		"servings", recipe->servings,
		"note", recipe->notes,
		"ingredients", ingredients,
		"steps", steps,
		"tags", tags
		);

	if (object == NULL) {
		fprintf(stderr, "%s %d\n", error.text, error.position);
	}

	s = json_dumps(object, JSON_SORT_KEYS|JSON_COMPACT);

	json_decref(object);

	return s;
}

// recipe_free : frees all of the data in the recipe object
void recipe_free(struct Recipe *recipe)
{
	if (recipe) {
		free(recipe->name);
		free(recipe->prep_time);
		free(recipe->cook_time);
		free(recipe->servings);
		free(recipe->notes);

		u_textlist_free(recipe->ingredients);
		u_textlist_free(recipe->steps);
		u_textlist_free(recipe->tags);

		free(recipe);
	}
}
