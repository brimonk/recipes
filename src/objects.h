#ifndef OBJECTS_H_
#define OBJECTS_H_

#include "common.h"

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
// db_metadata_free: releases the members of 'metadata', but NOT 'metadata' itself
void db_metadata_free(DB_Metadata *metadata);
// db_insert_textlist: inserts the entire textlist as a single db transaction
int db_insert_textlist(char *table, char *id, char **list);
// db_get_textlist: fetches a textlist from the database with 'parent_id' as 'id'
char **db_get_textlist(char *table, char *id);
// db_delete_textlist: deletes all of the textlists from the table with parent_id = id
int db_delete_textlist(char *table, char *id);

// db_transaction_begin: begins a transaction on the database
void db_transaction_begin();
// db_transaction_commit: commits the currently open transaction
void db_transaction_commit();
// db_transaction_rollback: rolls the currently open transaction back
void db_transaction_rollback();

// metadata_clone: useful to ensure updated / deleted records have all of the metadata before
// performing database operations with them.
DB_Metadata metadata_clone(DB_Metadata original);

#endif // OBJECTS_H
