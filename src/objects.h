#ifndef OBJECTS_H_
#define OBJECTS_H_

#include "common.h"

// U_TextList: stores a list of strings
typedef struct U_TextList {
	struct U_TextList *next;
	char *text;
} U_TextList;

// u_textlist_get: returns the text at index 'idx'
char *u_textlist_get(U_TextList *list, int idx);
// u_textlist_len: returns the length of the textlist
int u_textlist_len(U_TextList *list);
// u_textlist_append: appends 'text' to the 'list'
int u_textlist_append(U_TextList **list, char *text);
// u_textlist_free: releases the memory from a textlist
void u_textlist_free(U_TextList *list);

// DB_Metadata: every table needs to implement a DB_Metadata as its first member
typedef struct DB_Metadata {
    int64_t rowid;
    char *id;
    char *create_ts;
    char *update_ts;
    char *delete_ts;
} DB_Metadata;

// DB_ChildTextRecord: represents a "child" text entry, a fixed schema for lists of strings
typedef struct DB_ChildTextRecord {
    int64_t rowid;
    char *parent_id;
    char *text;
} DB_ChildTextRecord;

// UI_SearchQuery: user input for performing search queries
typedef struct UI_SearchQuery {
	char *table;
	char **columns;
	char *search_field;
	size_t page_size;
	size_t page_number;
	char *search;
	char *sort_field;
	char *sort_order;
} UI_SearchQuery;

// db_search_to_json: takes in a UI_SearchQuery object, returns a JSON schema, see func for details
json_t *db_search_to_json(UI_SearchQuery *query);
// db_load_metadata_from_rowid: fills out the metadata struct given the table and rowid
int db_load_metadata_from_rowid(DB_Metadata *metadata, char *table, int64_t rowid);

// UI_SearchResults: user input (result) for searches
typedef struct UI_SearchResults {
	size_t records;
	void *data;
} UI_SearchResults;

#endif // OBJECTS_H
