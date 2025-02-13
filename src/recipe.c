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
	int rc;
	struct Recipe *updated;
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

	updated = recipe_from_json(json);

	free(json);

	if (updated == NULL) { // TODO (Brian): HTTP Error
		ERR("couldn't parse updated recipe from json!\n");
		return -1;
	}

    recipe = recipe_get_by_id(id);
    if (recipe == NULL) {
        ERR("couldn't load the recipe with id: '%s'", id);
        return -1;
    }

    updated->metadata = metadata_clone(recipe->metadata);
	recipe_free(recipe);

	if (recipe_validation(updated) < 0) { // TODO (Brian): HTTP Error
		ERR("updated recipe record invalid!\n");
		return -1;
	}

	rc = recipe_update(updated);
	if (rc < 0) {
		ERR("couldn't update the recipe!\n");
		return -1;
	}

	mg_http_reply(conn, 200, NULL, "{\"id\":\"%s\"}", updated->metadata.id);

	recipe_free(updated);

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
		free(json);
		recipe_free(recipe);
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
		"insert into recipes (name, prep_time, cook_time, servings, link, notes) values (?, ?, ?, ?, ?, ?);";

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		return -1;
	}

	sqlite3_bind_text(stmt, 1, (const char *)recipe->name, -1, NULL);

	if (recipe->prep_time) {
		sqlite3_bind_text(stmt, 2, (const char *)recipe->prep_time, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 2);
	}

	if (recipe->cook_time) {
		sqlite3_bind_text(stmt, 3, (const char *)recipe->cook_time, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 3);
	}

	if (recipe->servings) {
		sqlite3_bind_text(stmt, 4, (const char *)recipe->servings, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 4);
	}

	if (recipe->link) {
		sqlite3_bind_text(stmt, 5, (const char *)recipe->link, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 5);
	}

	if (recipe->notes) {
		sqlite3_bind_text(stmt, 6, (const char *)recipe->notes, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 6);
	}

	rc = sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	stmt = NULL;

	if (rc != SQLITE_DONE) { // deal with error
		sqlite3_finalize(stmt);
		ERR("error inserting recipe record! %s", sqlite3_errstr(rc));
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

	rc = db_insert_textlist("steps", recipe->metadata.id, recipe->steps);
	if (rc < 0) { // ?
		goto recipe_insert_fail;
	}

	rc = db_insert_textlist("tags", recipe->metadata.id, recipe->tags);
	if (rc < 0) { // ?
		goto recipe_insert_fail;
	}

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
	// NOTE (Brian) Deleting a recipe deletes all children tables (ingredients, steps, tags),
	// updates the main table, then adds all of the new child text lists again.

	int rc;

	db_transaction_begin();

	rc = db_delete_textlist("ingredients", recipe->metadata.id);
	if (rc < 0) goto recipe_update_fail;
	rc = db_delete_textlist("steps", recipe->metadata.id);
	if (rc < 0) goto recipe_update_fail;
	rc = db_delete_textlist("tags", recipe->metadata.id);
	if (rc < 0) goto recipe_update_fail;

	char *query = "update recipes set name = ?, prep_time = ?, cook_time = ?, servings = ?, link = ?, notes = ? where id = ?;";

	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
        rc = -1;
		goto recipe_update_fail;
	}

	sqlite3_bind_text(stmt, 1, (const char *)recipe->name, -1, NULL);

	if (recipe->prep_time) {
		sqlite3_bind_text(stmt, 2, (const char *)recipe->prep_time, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 2);
	}

	if (recipe->cook_time) {
		sqlite3_bind_text(stmt, 3, (const char *)recipe->cook_time, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 3);
	}

	if (recipe->servings) {
		sqlite3_bind_text(stmt, 4, (const char *)recipe->servings, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 4);
	}

	if (recipe->link) {
		sqlite3_bind_text(stmt, 5, (const char *)recipe->link, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 5);
	}

	if (recipe->notes) {
		sqlite3_bind_text(stmt, 6, (const char *)recipe->notes, -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 6);
	}

    sqlite3_bind_text(stmt, 7, (const char *)recipe->metadata.id, -1, NULL);

	rc = sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	stmt = NULL;

	rc = db_insert_textlist("ingredients", recipe->metadata.id, recipe->ingredients);
	if (rc < 0) goto recipe_update_fail;
	rc = db_insert_textlist("steps", recipe->metadata.id, recipe->steps);
	if (rc < 0) goto recipe_update_fail;
	rc = db_insert_textlist("tags", recipe->metadata.id, recipe->tags);
	if (rc < 0) goto recipe_update_fail;

	db_transaction_commit();

	rc = 0;

recipe_update_fail:
	if (rc) db_transaction_rollback();
	if (stmt != NULL) sqlite3_finalize(stmt);

	return rc;
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
	fprintf(stream, "select name, prep_time, cook_time, servings, link, notes from recipes where id = ?;");
	fclose(stream);

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		ERR("%s\n", sqlite3_errmsg(DATABASE));
		free(query);
		return NULL;
	}

	sqlite3_bind_text(stmt, 1, id, -1, NULL);

	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		recipe->name	  = strdup_null((char *)sqlite3_column_text(stmt, 0));
		recipe->prep_time = strdup_null((char *)sqlite3_column_text(stmt, 1));
		recipe->cook_time = strdup_null((char *)sqlite3_column_text(stmt, 2));
		recipe->servings  = strdup_null((char *)sqlite3_column_text(stmt, 3));
		recipe->link     = strdup_null((char *)sqlite3_column_text(stmt, 4));
		recipe->notes	 = strdup_null((char *)sqlite3_column_text(stmt, 5));
	}

	sqlite3_finalize(stmt);

	db_load_metadata_from_id(&recipe->metadata, "recipes", id);

	recipe->ingredients = db_get_textlist("ingredients", recipe->metadata.id);
	recipe->steps = db_get_textlist("steps", recipe->metadata.id);
	recipe->tags = db_get_textlist("tags", recipe->metadata.id);

	free(query);

	return recipe;
}

// recipe_delete : updates 'deleted_ts' on the given recipe, such that it is 'deleted'
int recipe_delete(char *id)
{
	sqlite3_stmt *stmt;
	int rc;
	char *query = "update recipes set delete_ts = (strftime('%Y%m%d-%H%M%f', 'now')) where id = ?;";

	// TODO (Brian) handle errors

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
	}

	rc = sqlite3_bind_text(stmt, 1, id, -1, NULL);
	if (rc != SQLITE_OK) {
	}

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
	}

	sqlite3_finalize(stmt);

	return 0;
}

// recipe_search : fills out a search structure with results
json_t *recipe_search(char *query, size_t page_size, size_t page_number)
{
	// TODO (Brian) finish updating the 
	char *search_cols[] = { "id", "search_text", NULL };
	char *where_clause[] = { "search_text", "like", "?", NULL };
	char *search_bind_params[] = { COALESCE(query, "%"), NULL };
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
	return recipe == NULL || recipe->name == NULL || 100 < strlen(recipe->name);
}

// recipe_from_json : converts a JSON string into a Recipe
static struct Recipe *recipe_from_json(char *s)
{
	struct Recipe *recipe;
	json_t *root;
	json_error_t error;
	size_t i;

	recipe = calloc(1, sizeof(*recipe));
	if (recipe == NULL) {
		return NULL;
	}

	root = json_loads(s, 0, &error);
	if (root == NULL) {
		free(recipe);
		return NULL;
	}

	if (!json_is_object(root)) {
		json_decref(root);
		return NULL;
	}

	// name is the only required recipe value, everything else is optional
	json_t *name = json_object_get(root, "name");
	if (json_is_string(name)) {
		recipe->name = strdup(json_string_value(name));
	} else {
		json_decref(root);
		free(recipe);
		return NULL;
	}

	// everything else, we can just check for the key, and if it's present and a string/array,
	// just add it into the structure
	json_t *prep_time = json_object_get(root, "prep_time");
	if (json_is_string(prep_time)) {
		recipe->prep_time = strdup(json_string_value(prep_time));
	}

	json_t *cook_time = json_object_get(root, "cook_time");
	if (json_is_string(cook_time)) {
		recipe->cook_time = strdup(json_string_value(cook_time));
	}

	json_t *servings = json_object_get(root, "servings");
	if (json_is_string(servings)) {
		recipe->servings = strdup(json_string_value(servings));
	}

	json_t *notes = json_object_get(root, "note");
	if (json_is_string(notes)) {
		recipe->notes = strdup(json_string_value(notes));
	}

	json_t *link = json_object_get(root, "link");
	if (json_is_string(link)) {
		recipe->link = strdup(json_string_value(link));
	}

	json_t *ingredients = json_object_get(root, "ingredients");
	if (json_is_array(ingredients)) {
		for (i = 0; i < json_array_size(ingredients); i++) {
			// TODO (Brian): check that this is actually a string value
			json_t *ingredient = json_array_get(ingredients, i);
			arrput(recipe->ingredients, strdup(json_string_value(ingredient)));
		}
	}

	json_t *steps = json_object_get(root, "steps");
	if (json_is_array(steps)) {
		for (i = 0; i < json_array_size(steps); i++) {
			// TODO (Brian): check that this is actually a string value
			json_t *step = json_array_get(steps, i);
			arrput(recipe->steps, strdup(json_string_value(step)));
		}
	}

	json_t *tags = json_object_get(root, "tags");
	if (json_is_array(tags)) {
		for (i = 0; i < json_array_size(tags); i++) {
			// TODO (Brian): check that this is actually a string value
			json_t *tag = json_array_get(tags, i);
			arrput(recipe->tags, strdup(json_string_value(tag)));
		}
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
	for (i = 0, len = arrlen(recipe->ingredients); i < len; i++) {
		json_array_append_new(ingredients, json_string(recipe->ingredients[i]));
	}

	steps = json_array();
	for (i = 0, len = arrlen(recipe->steps); i < len; i++) {
		json_array_append_new(steps, json_string(recipe->steps[i]));
	}

	tags = json_array();
	for (i = 0, len = arrlen(recipe->tags); i < len; i++) {
		json_array_append_new(tags, json_string(recipe->tags[i]));
	}

	object = json_pack_ex(
		&error, 0,
		"{s:s, s:s, s:s?, s:s?, s:s, s:s?, s:s?, s:s?, s:s?, s:s?, s:o, s:o, s:o}",
		"id", recipe->metadata.id,
		"create_ts", recipe->metadata.create_ts,
		"update_ts", recipe->metadata.update_ts,
		"delete_ts", recipe->metadata.delete_ts,
		"name", recipe->name,
		"prep_time", recipe->prep_time,
		"cook_time", recipe->cook_time,
		"servings", recipe->servings,
		"link", recipe->link,
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
		db_metadata_free(&recipe->metadata);

		free(recipe->name);
		free(recipe->prep_time);
		free(recipe->cook_time);
		free(recipe->servings);
		free(recipe->notes);
		free(recipe->link);

		for (size_t i = 0; i < arrlen(recipe->ingredients); i++)
			free(recipe->ingredients[i]);
		arrfree(recipe->ingredients);
		for (size_t i = 0; i < arrlen(recipe->steps); i++)
			free(recipe->steps[i]);
		arrfree(recipe->steps);
		for (size_t i = 0; i < arrlen(recipe->tags); i++)
			free(recipe->tags[i]);
		arrfree(recipe->tags);

		free(recipe);
	}
}
