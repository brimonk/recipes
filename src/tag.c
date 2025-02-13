#define _GNU_SOURCE
#include "common.h"

#include <sodium.h>
#include <jansson.h>

#include "mongoose.h"
#include "sqlite3.h"

#include "tag.h"

extern sqlite3 *DATABASE;

static int tag_select_cb(void *ptr, int ncols, char ** tcol, char **colnames)
{
	char ***tags = ptr;
	char *clone = strdup(tcol[0]);

	if (clone == NULL) {
		return -1;
	} else {
		arrput(*tags, clone);
		return 0;
	}
}

// tag_api_getlist : endpoint, GET - /api/v1/tags
int tag_api_getlist(struct mg_connection *conn, struct mg_http_message *hm)
{
	char **tags = NULL;
	char *errmsg = NULL;

	const char *sql = "select distinct text from tags;";

	int rc = sqlite3_exec(DATABASE, sql, tag_select_cb, &tags, &errmsg);
	if (rc != SQLITE_OK) {
		assert(0); // TODO HANDLE
	}

	json_t *object = json_object();
	json_t *array = json_array();

	for (size_t i = 0; i < arrlen(tags); i++) {
		json_array_append_new(array, json_string(tags[i]));
		free(tags[i]);
	}

	arrfree(tags);

	json_object_set_new(object, "tags", array);
	char *s = json_dumps(object, 0);

	mg_http_reply(conn, 200, NULL, "%s", s);
	json_decref(object);

	free(s);

	return 0;
}
