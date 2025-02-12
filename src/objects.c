#include <jansson.h>

#include "common.h"
#include "objects.h"

#include "sqlite3.h"

extern sqlite3 *DATABASE;

// this is a really bad spot for this function, but I'm not sure where else it should go
// maybe in a file called 'search.c'

// db_search_to_json: takes in a UI_SearchQuery object, and returns a JSON object with search results
json_t *db_search_to_json(UI_SearchQuery *query)
{
	// NOTE (Brian) (rewrite this, this is a mess)
	//
	// The query that gets executed has the following form:
	//
	// select query.results.[columns] from query.results.table o
	//   inner join (select query.search.[columns] from query.search.table where query.search.[where]) i
	//     on o.[query.results.pk_column] = i.[query.search.pk_column]
	//   (order by query.results.sort_field (query.results.sort_order))
	//   (limit query.page_size offset (query.page_size * query.page_number));
	//
	// Once this query gets executed, it will return a JSON structure that looks like this:
	//
	// {
	//   "total": (total results),
	//   "page": (query.page_number),
	//   "size": (query.page_size),
	//   "results": [(each record has name of column from result set)]
	// }

#define RETURNNOW(V_) \
    if (query_text) free(query_text); \
    if (stmt) sqlite3_finalize(stmt); \
    if (colvalus) free(colvalus); \
    if (colnames) free(colnames); \
	if (json && (V_) == NULL) json_decref(json); \
    return (V_);

    // used while preparing the query
    char *query_text = NULL;
    size_t query_sz = 0;
    sqlite3_stmt *stmt = NULL;
    int rc;

    // used in results gathering
    int results_ncols = 0;
    char **colvalus = NULL;
    char **colnames = NULL;
	json_t *json = NULL;
	json_t *results = NULL;

    // TODO (Brian) check object before we build it
    assert(query != NULL);

    FILE *stream = open_memstream(&query_text, &query_sz);
    if (stream == NULL) {
        RETURNNOW(NULL);
    }

    fprintf(stream, "select ");
    for (char **s = &query->results.columns[0]; *s; s++) {
        fprintf(stream, "o.%s%s", s[0], s[1] == NULL ? " " : ", ");
	}

    fprintf(stream, "from %s o ", query->results.table);
	fprintf(stream, "inner join (");

	fprintf(stream, "select ");
	for (char **s = &query->search.columns[0]; *s; s++) {
		fprintf(stream, "%s%s", s[0], s[1] == NULL ? " " : ", ");
	}

	fprintf(stream, "from %s ", query->search.table);

	// NOTE (Brian) Currently the WHERE API has people adding in strings for their entire query.
	// We probably want to make that not the case in the future. We should make a query builder
	// or something.
	fprintf(stream, "where ");
	for (char **s = &query->search.where[0]; *s; s++) {
		fprintf(stream, "%s ", s[0]);
	}

	fprintf(stream, ") i on o.%s = i.%s ", query->search.pk_column, query->results.pk_column);

    if (query->results.sort_field) {
        fprintf(stream, " order by %s %s",
			query->results.sort_field, COALESCE(query->results.sort_order, ""));
    }

    fprintf(stream, " limit %ld offset %ld;",
        query->page_size, query->page_number * query->page_size);

    fclose(stream);

    rc = sqlite3_prepare_v2(DATABASE, query_text, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
		ERR("Query prepare error! %s\n", sqlite3_errmsg(DATABASE));
		fprintf(stderr, "Query was:\n%s\n", query_text);
        RETURNNOW(NULL);
    }

	// NOTE (Brian) we probably want to bind more than strings at some point. But, it's just
	// strings for now. It'd also probably be a good idea to check if the number of things in the
	// bind array matches the number of '?' in the where clause.
	for (int i = 0; query->search.bind[i]; i++) {
		rc = sqlite3_bind_text(stmt, i + 1, (const char *)query->search.bind[i], -1, NULL);
		if (rc != SQLITE_OK) {
			RETURNNOW(NULL);
		}
	}

    json = json_object();
	results = json_array();

    json_object_set_new(json, "total", json_integer(0));
    json_object_set_new(json, "page", json_integer(query->page_number));
    json_object_set_new(json, "size", json_integer(query->page_size));

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

		json_array_append_new(results, elem);
    }

    json_object_set_new(json, "results", results);

	if (!(rc == SQLITE_OK || rc == SQLITE_DONE)) {
		fprintf(stderr, "Query was:\n%s\n", query_text);
		fprintf(stderr, "SQLITE ERROR: %s\n", sqlite3_errstr(rc));
	}

    RETURNNOW(json);

#undef RETURNING
}

// db_load_metadata_from_rowid: fills out the metadata struct given the table and rowid
int db_load_metadata_from_rowid(DB_Metadata *metadata, char *table, int64_t rowid)
{
	char *query;
	size_t query_sz;
	sqlite3_stmt *stmt = NULL;
	int rc;

    FILE *stream = open_memstream(&query, &query_sz);
	fprintf(stream, "select id, create_ts, update_ts, delete_ts from %s where rowid = ?;", table);
    fclose(stream);

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

	free(query);

	return 0;
}

// db_load_metadata_from_id: fetches database metadata from the uuid 'id'
int db_load_metadata_from_id(DB_Metadata *metadata, char *table, char *id)
{
	char *query;
	size_t query_sz;
	sqlite3_stmt *stmt = NULL;
	int rc;

    FILE *stream = open_memstream(&query, &query_sz);
    fprintf(stream, "select id, create_ts, update_ts, delete_ts from %s where id = ?;", table);
    fclose(stream);

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) { // TODO log error
		return -1;
	}

	sqlite3_bind_text(stmt, 1, (const char *)id, -1, NULL);

	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        metadata->id = strdup_null((char *)sqlite3_column_text(stmt, 0));
        metadata->create_ts = strdup_null((char *)sqlite3_column_text(stmt, 1));
        metadata->update_ts = strdup_null((char *)sqlite3_column_text(stmt, 2));
        metadata->delete_ts = strdup_null((char *)sqlite3_column_text(stmt, 3));
    }

	sqlite3_finalize(stmt);

    free(query);

	return 0;
}

// db_insert_textlist: inserts the entire textlist as a single db transaction
int db_insert_textlist(char *table, char *id, char **list)
{
    char *query;
    size_t query_sz;
    sqlite3_stmt *stmt;
    int rc;

	// TODO (Brian) put this into a transaction (so we can rollback)
    // TODO (Brian) handle errors in this OR THERE BE DRAGONS

    FILE *stream = open_memstream(&query, &query_sz);
    fprintf(stream, "insert into %s (parent_id, sorting, text) values (?, ?, ?);", table);
    fclose(stream);

    rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Query prepare error! %s\n", sqlite3_errstr(rc));
		return -1;
	}

    sqlite3_bind_text(stmt, 1, (const char *)id, -1, NULL);

	for (size_t i = 0; i < arrlen(list); i++) {
        sqlite3_bind_int64(stmt, 2, i);
        sqlite3_bind_text(stmt, 3, list[i], -1, NULL);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
	}

    sqlite3_finalize(stmt);
    free(query);

    return 0;
}

// db_get_textlist: fetches a textlist from the database with 'parent_id' as 'id'
char **db_get_textlist(char *table, char *id)
{
    char **list = NULL;

    char *query;
	size_t query_sz;
    FILE *stream = open_memstream(&query, &query_sz);
    fprintf(stream, "select parent_id, text from %s where parent_id = ?;", table);
    fclose(stream);

    sqlite3_stmt *stmt = NULL;
    int rc;

    rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(query);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, (const char *)id, -1, NULL);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		arrput(list, strdup((const char *)sqlite3_column_text(stmt, 1)));
    }

    sqlite3_finalize(stmt);

    free(query);

    return list;
}

// db_delete_textlist: deletes all of the textlists from the table with parent_id = id
int db_delete_textlist(char *table, char *id)
{
	char *query;
	size_t query_sz;

	FILE *fp = open_memstream(&query, &query_sz);
	fprintf(fp, "delete from %s where parent_id = ?;", table);
	fclose(fp);

	sqlite3_stmt *stmt;
	int rc;

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		free(query);
		return -1;
	}

	sqlite3_bind_text(stmt, 1, id, -1, NULL);

	rc = sqlite3_step(stmt);

	sqlite3_finalize(stmt);

	return rc == SQLITE_DONE ? 0 : -1;
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

// db_transaction_begin: begins a transaction on the database
void db_transaction_begin()
{
	sqlite3_exec(DATABASE, "begin transaction;", NULL, NULL, NULL);
}

// db_transaction_commit: commits the currently open transaction
void db_transaction_commit()
{
	sqlite3_exec(DATABASE, "commit transaction;", NULL, NULL, NULL);
}

// db_transaction_rollback: rolls the currently open transaction back
void db_transaction_rollback()
{
	sqlite3_exec(DATABASE, "rollback transaction;", NULL, NULL, NULL);
}
