#include <jansson.h>

#include "common.h"
#include "objects.h"

#include "sqlite3.h"

extern sqlite3 *DATABASE;

// u_textlist_get: returns the text at index 'idx'
char *u_textlist_get(U_TextList *list, int idx)
{
	for (int i = 0; i < idx && list; i++)
		list = list->next;
	return list ? list->text : NULL;
}

// u_textlist_len: returns the length of the textlist
int u_textlist_len(U_TextList *list)
{
	int len = 0;
	for (; list; list = list->next)
		len++;
	return len;
}

// u_textlist_append: appends 'text' to the 'list'
int u_textlist_append(U_TextList **list, char *text)
{
	U_TextList *curr = NULL;

	if (*list == NULL) {
		*list = calloc(1, sizeof(*list));
		if ((*list) == NULL)
			return -1;

		(*list)->next = NULL;
		(*list)->text = text;
	} else {
		for (curr = *list; curr && curr->next; curr = curr->next)
			;

		curr->next = calloc(1, sizeof(*list));
		if (curr->next == NULL)
			return -1;

		curr->next->next = NULL;
		curr->next->text = text;
	}

	return 0;
}

// u_textlist_free: releases the memory from a textlist
void u_textlist_free(U_TextList *list)
{
    while (list) {
        U_TextList *curr = list;
        list = list->next;
        free(curr->text);
        free(curr);
    }
}

// this is a really bad spot for this function, but I'm not sure where else it should go
// maybe in a file called 'search.c'

// db_search_to_json: takes in a UI_SearchQuery object, and returns a JSON object with search results
json_t *db_search_to_json(UI_SearchQuery *query)
{
	// NOTE (Brian)
	//
	// This function will return results with the following schema:
	//
	// {
	//     "results": [/* result rows as objects here */],
	//     "total": int,
	//     "page": int,
	//     "size": int,
	// }

#define RETURNNOW(V_) \
    if (query_text) free(query_text); \
    if (stmt) sqlite3_finalize(stmt); \
    if (colvalus) free(colvalus); \
    return V_;

    // used while preparing the query
    char *query_text = NULL;
    size_t query_sz = 0;
    sqlite3_stmt *stmt = NULL;
    int rc;

    // used in results gathering
    int results_ncols = 0;
    char **colvalus = NULL;
    char **colnames = NULL;

    assert(query != NULL);

    FILE *stream = open_memstream(&query_text, &query_sz);
    if (stream == NULL) {
        RETURNNOW(NULL);
    }

    fprintf(stream, "select ");
    for (char **i = &query->columns[0]; i; i++) {
        fprintf(stream, "%s%s", i[0], i[1] == NULL ? " " : ", ");
	}

    fprintf(stream, "from %s where text contains ? order by %s %s limit %ld offset %ld",
        query->table,
        query->sort_field,
        query->sort_order != NULL ? "" : query->sort_order,
        query->page_size,
        query->page_number * query->page_size
    );

    fclose(stream);

    rc = sqlite3_prepare_v2(DATABASE, query_text, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        RETURNNOW(NULL);
    }

    rc = sqlite3_bind_text(stmt, 1, (const char *)query->search, -1, NULL);
    if (rc != SQLITE_OK) {
        RETURNNOW(NULL);
    }

    json_t *json = json_object();

    json_object_set_new(json, "total", json_integer(0));
    json_object_set_new(json, "page", json_integer(query->page_number));
    json_object_set_new(json, "size", json_integer(query->page_size));
    json_object_set_new(json, "results", json_array());

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (results_ncols == 0) {
            results_ncols = sqlite3_column_count(stmt);
            colvalus = calloc(results_ncols, sizeof(*colvalus));
            colnames = calloc(results_ncols, sizeof(*colnames));

            for (int i = 0; i < results_ncols; i++) {
                colnames[i] = (char *)sqlite3_column_name(stmt, i);
            }

            if (colnames == NULL) {
                RETURNNOW(NULL);
            }
        }

		json_t *elem = json_object();

		for (int i = 0; i < results_ncols; i++) {
			switch (sqlite3_column_type(stmt, i)) {
				case SQLITE_INTEGER:
					json_object_set_new(
						elem, colnames[i], json_integer(sqlite3_column_int64(stmt, i))
					);
					break;

				case SQLITE_FLOAT:
					json_object_set_new(
						elem, colnames[i], json_real(sqlite3_column_double(stmt, i))
					);
					break;

				case SQLITE_TEXT:
					json_object_set_new(
						elem, colnames[i], json_string((char *)sqlite3_column_text(stmt, i))
					);
					break;

				case SQLITE_BLOB:
					assert(0); // we have to base64 encode this one, because it isn't already UTF-8 encoded
					break;

				case SQLITE_NULL:
					json_object_set_new(
						elem, colnames[i], json_null()
					);
					break;

				default:
					break;
			}
		}

		json_array_append(json_object_get(json, "results"), elem);
    }

    RETURNNOW(json);

#undef RETURNING
}

// db_load_metadata_from_rowid: fills out the metadata struct given the table and rowid
int db_load_metadata_from_rowid(DB_Metadata *metadata, char *table, int64_t rowid)
{
	char query[128] = {0};
	sqlite3_stmt *stmt = NULL;
	int rc;

	snprintf(query, sizeof query,
		"select id, create_ts, update_ts, delete_ts from %s where rowid = ?;", table
	);

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) { // TODO log error
		return -1;
	}

	sqlite3_bind_int64(stmt, 1, rowid);

	sqlite3_step(stmt);

	metadata->id = strdup_null((char *)sqlite3_column_text(stmt, 0));
	metadata->create_ts = strdup_null((char *)sqlite3_column_text(stmt, 1));
	metadata->update_ts = strdup_null((char *)sqlite3_column_text(stmt, 2));
	metadata->delete_ts = strdup_null((char *)sqlite3_column_text(stmt, 3));

	sqlite3_finalize(stmt);

	return 0;

}

// db_metadata_free: releases the members of 'metadata', but NOT 'metadata' itself
void db_metadata_free(DB_Metadata *metadata)
{
	if (metadata) {
		free(metadata->id);
		free(metadata->create_ts);
		free(metadata->update_ts);
		free(metadata->delete_ts);
	}
}
