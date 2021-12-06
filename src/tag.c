// Brian Chrzanowski
// 2021-11-05 14:49:08
//
// NOTE (Brian): At some point, we're going to have to rectify these functions to work for more than
// just a single user. But, that'll come when user(s) actually work.

#include "common.h"

#include <sodium.h>
#include <math.h>
#include <jansson.h>

#include "mongoose.h"

#include "recipe.h"
#include "objects.h"
#include "store.h"

#include "tag.h"

// tag_api_getlist : endpoint, GET - /api/v1/tags
int tag_api_getlist(struct mg_connection *conn, struct mg_http_message *hm)
{
	tag_t *tag;
	string_128_t *string128;
	char **tags;
	size_t tags_len, tags_cap;
	json_t *object, *array;
	char *s;
	s64 i, len;

	// NOTE (Brian): only fetch the list of unique tags, we'll worry about the other things for
	// later.

	tags = NULL;
	tags_len = tags_cap = 0;

	object = json_object();
	array = json_array();

	for (i = 1, len = store_getlen(RT_TAG); i <= len; i++) {
		C_RESIZE(&tags);

		tag = store_getobj(RT_TAG, i);
		if (tag == NULL)
            continue;

		string128 = store_getobj(RT_STRING128, tag->string_id);
		if (string128 == NULL)
            continue;

		tags[tags_len] = strndup(string128->string, sizeof(string128->string));
		json_array_append_new(array, json_string(tags[tags_len]));

		tags_len++;
	}

	// now, we convert it to a json object, with an array of tags
	json_object_set_new(object, "tags", array);

	s = json_dumps(object, 0);

    mg_http_reply(conn, 200, NULL, "%s", s);

	// cleanup
	json_decref(object);

    free(s);

	for (i = 0; i < tags_len; i++)
		free(tags[i]);
	free(tags);

	return 0;
}

