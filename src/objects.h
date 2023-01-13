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

// DB_Query: currently used to abstract query responsibilities for "UI_SearchQuery"
typedef struct DB_Query {
    char *pk_column;
    char **columns;
    char *table;
    char **where;
	char **bind;
    char *sort_field;
    char *sort_order;
} DB_Query;

// UI_SearchQuery: user input for performing search queries
typedef struct UI_SearchQuery {
    DB_Query search;
    DB_Query results;
    size_t page_size;
    size_t page_number;
} UI_SearchQuery;

// db_search_to_json: takes in a UI_SearchQuery object, returns a JSON schema, see func for details
json_t *db_search_to_json(UI_SearchQuery *query);
// db_load_metadata_from_rowid: fills out the metadata struct given the table and rowid
int db_load_metadata_from_rowid(DB_Metadata *metadata, char *table, int64_t rowid);
// db_load_metadata_from_id: fetches database metadata from the uuid 'id'
int db_load_metadata_from_id(DB_Metadata *metadata, char *table, char *id);
// db_insert_textlist: inserts the entire textlist as a single db transaction
int db_insert_textlist(char *table, char *id, U_TextList *list);
// db_get_textlist: fetches a textlist from the database with 'parent_id' as 'id'
U_TextList *db_get_textlist(char *table, char *id);

#endif // OBJECTS_H
