// Brian Chrzanowski
// 2021-09-14 15:38:24
//
// This program will both bootstrap the database if needed, run migration scripts if there are any

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"

#define COMMON_IMPLEMENTATION
#include "common.h"

#define META_PATH   ("src/metaprogram.sql")
#define SCHEMA_PATH ("src/schema.sql")

#define USAGE ("%s <dbname>\n")

static sqlite3 *db;

typedef struct {
	char *table_name;
	int ordinal_pos;
	char *colname;
	char *coltype;
	int isnull;
	int ispk;
} metadata_t;

typedef struct {
	metadata_t *data;
	size_t data_len, data_cap;
} metadata_list_t;

// create_tables: bootstraps the database (and the rest of the app)
int create_tables(sqlite3 *db, char *fname);

// help: prints help info
void help(char *prog);

char *GetCType(char *sqltype)
{
	// NOTE (Brian) this is totally for sqlite only because of the data type assumptions.

	if (sqltype == NULL)
		return NULL;
	if (sqltype[0] == 'i')
		return "db_int64";
	if (sqltype[0] == 'r')
		return "db_double";
	if (sqltype[0] == 't')
		return "db_string";
	if (sqltype[0] == 'b')
		return "db_blob";
	ERR("couldn't find c datatype for db datatype: '%s'!\n", sqltype);
	return NULL;
}

// metaprogram_cb: sqlite3_exec callback function for src/metaprogram.sql
int metaprogram_cb(void *ptr, int colnum, char **vals, char **cols)
{
	metadata_list_t *list;
	metadata_t *datum;
	int i;

	char *expected_cols[] = {
		"table_name", "ordinal_pos", "colname", "coltype", "isnull", "ispk"
	};

	assert(ptr != NULL);
	list = ptr;

	assert(colnum == 6);
	for (i = 0; i < ARRSIZE(expected_cols); i++) {
		assert(streq(cols[i], expected_cols[i]));
	}

	C_RESIZE(&list->data);

	datum = list->data + list->data_len++;

	// now, try to map everything over
	datum->table_name = strdup(vals[0]);
	datum->ordinal_pos = atoi(vals[1]);
	datum->colname = strdup(vals[2]);

	datum->coltype = strdup(vals[3]); // ?

	datum->isnull = atoi(vals[4]);
	datum->ispk = atoi(vals[5]);

	return 0;
}

void PrintHeader(void)
{
	printf("#ifndef META_GEN_H\n");
	printf("#define META_GEN_H\n");
	printf("\n");
}

void PrintFooter(void)
{
	printf("#endif // META_GEN_H\n");
	printf("\n");
}

void PrintImplementationGuardStart(void)
{
	printf("#ifdef META_GEN_IMPLEMENTATION_H\n");
	printf("#define META_GEN_IMPLEMENTATION_H\n");
	printf("\n");
}

void PrintImplementationGuardEnd(void)
{
	printf("#endif // META_GEN_IMPLEMENTATION_H\n");
	printf("\n");
}

void PrintDatabaseTypes(void)
{
	printf("#include <stddef.h>\n");
	printf("#include <stdint.h>\n");
	printf("\n");

	// NOTE (Brian): this totally includes more types than we'll ever need for SQLite. SQLite types
	// are loosey goosey like JSON types, so we'll basically end up using like, ~5 of these.
	// However, I think it'll be good to have them here for future expansion.

	printf("typedef int8_t   db_int8;\n");
	printf("typedef int16_t  db_int16;\n");
	printf("typedef int32_t  db_int32;\n");
	printf("typedef int64_t  db_int64;\n");
	printf("\n");

	printf("typedef uint8_t  db_uint8;\n");
	printf("typedef uint16_t db_uint16;\n");
	printf("typedef uint32_t db_uint32;\n");
	printf("typedef uint64_t db_uint64;\n");
	printf("\n");

	printf("typedef float    db_float;\n");
	printf("typedef double   db_double;\n");
	printf("\n");

	printf("typedef struct {\n");
	printf("\tchar *str;\n");
	printf("\tsize_t str_len, str_cap;\n");
	printf("} db_string;\n");
	printf("\n");

	printf("typedef struct {\n");
	printf("\tchar *blob;\n");
	printf("\tsize_t blob_len, blob_cap;\n");
	printf("} db_blob;\n");
	printf("\n");

	printf("typedef struct {\n");
	printf("\tchar *tabname;\n");
	printf("\tchar *colname;\n");
	printf("\tchar *coltype;\n");
	printf("\tint ordpos;\n");
	printf("\tint isnull;\n");
	printf("\tint ispk;\n");
	printf("} db_meta;\n");
	printf("\n");
}

void PrintStructures(metadata_list_t *list)
{
	size_t i;
	char *tabname;

	for (tabname = NULL, i = 0; i < list->data_len; i++) {
		if (tabname == NULL || !streq(tabname, list->data[i].table_name)) {
			if (tabname != NULL) {
				printf("} %s_t;\n", tabname);
				printf("\n");
			}

			printf("typedef struct {\n");

			tabname = list->data[i].table_name;
		}

		// NOTE (Brian): Here really isn't the greatest place for a datatype check because we've
		// already generated a whole bunch of the file, but we can move that later when we rewrite
		// this.

		if (list->data[i].coltype == NULL) {
			ERR("index %d has a NULL column type!\n", i);
		}

		if (list->data[i].colname == NULL) {
			ERR("index %d has a NULL column name!\n", i);
		}

		printf("\t%s %s;\n", GetCType(list->data[i].coltype), list->data[i].colname);
	}

	printf("} %s_t;\n", tabname);
	printf("\n");
}

void PrintMetadataTable(metadata_list_t *list)
{
	metadata_t *datum;
	size_t i;

	printf("static db_meta metadata[] = {\n");

	for (i = 0; i < list->data_len; i++) {
		datum = list->data + i;

		printf("\t{");

		printf("\"%s\",", datum->table_name);
		printf("\"%s\",", datum->colname);
		printf("\"%s\",", datum->coltype);
		printf("%d,",     datum->ordinal_pos);
		printf("%d,",     datum->isnull);
		printf("%d",      datum->ispk);

		if (i < list->data_len - 1) {
			printf("},\n");
		} else {
			printf("}\n");
		}
	}

	printf("};\n");
	printf("\n");
}

void PrintDBFunctionDef(metadata_list_t *list)
{
	size_t i;
}

void PrintJSONFunctionDef(metadata_list_t *list)
{
	size_t i;
}

void PrintAPIFunctionDef(metadata_list_t *list)
{
	size_t i;
}

void PrintDBFunctions(metadata_list_t *list)
{
	size_t i;
}

void PrintJSONFunctions(metadata_list_t *list)
{
	size_t i;
}

void PrintAPIFunctions(metadata_list_t *list)
{
	size_t i;
}

// create_tables: bootstraps the database (and the rest of the app)
int create_tables(sqlite3 *db, char *fname)
{
	sqlite3_stmt *stmt;
	char *sql, *s;
	int rc;

	sql = sys_readfile(fname, NULL);
	if (sql == NULL) {
		return -1;
	}

	for (s = sql; *s;) {
		rc = sqlite3_prepare_v2(db, s, -1, &stmt, (const char **)&s);
		if (rc != SQLITE_OK) {
			ERR("Couldn't Prepare STMT : %s\n", sqlite3_errmsg(db));
			return -1;
		}

		rc = sqlite3_step(stmt);

		//  TODO (brian): goofy hack, ensure that this can really just run
		//  all of our bootstrapping things we'd ever need
		if (rc == SQLITE_MISUSE) {
			continue;
		}

		if (rc != SQLITE_DONE) {
			ERR("Couldn't Execute STMT : %s\n", sqlite3_errmsg(db));
			return -1;
		}

		rc = sqlite3_finalize(stmt);
		if (rc != SQLITE_OK) {
			ERR("Couldn't Finalize STMT : %s\n", sqlite3_errmsg(db));
			return -1;
		}
	}

	return 0;
}

// help: prints help info
void help(char *prog)
{
	fprintf(stderr, USAGE, prog);
}

int main(int argc, char **argv)
{
	metadata_list_t list;
	char *metaprogram;
	int rc, i;

	if (argc < 2) {
		help(argv[0]);
		exit(1);
	}

	rc = sqlite3_open(argv[1], &db);
	if (rc != SQLITE_OK) {
		ERR("Can't open the database!");
		return 1;
	}

	sqlite3_db_config(db,SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		exit(1);
	}

	rc = sqlite3_load_extension(db, "./ext_uuid.so", "sqlite3_uuid_init", NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		exit(1);
	}

	rc = create_tables(db, SCHEMA_PATH);
	if (rc < 0) {
		ERR("Critical error in creating sql tables!!\n");
		exit(1);
	}

	metaprogram = sys_readfile(META_PATH, NULL);
	assert(metaprogram != NULL);

	memset(&list, 0, sizeof list);

	rc = sqlite3_exec(db, metaprogram, metaprogram_cb, &list, NULL);
	if (rc != SQLITE_OK) {
		ERR("couldn't gather table metadata, quitting\n");
		exit(1);
	}

	sqlite3_close(db);

	// We have all of the data we need at this point to generate the header.

	PrintHeader();

	PrintDatabaseTypes();
	PrintStructures(&list);

	PrintDBFunctionDef(&list);
	PrintJSONFunctionDef(&list);
	PrintAPIFunctionDef(&list);

	PrintImplementationGuardStart();

	PrintMetadataTable(&list);

	PrintImplementationGuardEnd();

	PrintFooter();

	free(list.data);

	return 0;
}

