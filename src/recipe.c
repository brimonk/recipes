// Brian Chrzanowski
// 2021-06-08 20:04:01
//
// fitness tracking program. read the schema to see the things we care about
//
// TODO (Brian)
// 1. form (style)
// 2. unencode the percent encoded things
// 3. create functions that'll verify sent across data types
// 4. deal with 
//
// We have to do something about parsing numbers and whatever else. We sure are just accepting
// whateve the user passes to us right into functions like 'atoi', and that probably isn't very
// good.
//
// QUESTIONABLE
// - generic function that inserts to a table given a form / table name

#define COMMON_IMPLEMENTATION
#include "common.h"

#define HTTPSERVER_IMPL
#include "httpserver.h"

#include "sqlite3.h"

#include <magic.h>

#define PORT (2000)

struct kvpair {
	char *k;
	char *v;
};

struct kvpairs {
	struct kvpair *kvpair;
	size_t kvpair_len, kvpair_cap;
};

static magic_t MAGIC_COOKIE;

static sqlite3 *db;

// init: initializes the program
void init(char *db_file_name, char *sql_file_name);
// cleanup: cleans up resources open so we can elegantly close the program
void cleanup(void);

// create_tables: execs create * statements on the database
int create_tables(sqlite3 *db, char *fname);

// is_uuid: returns true if the input string is a uuid
int is_uuid(char *id);

// request_handler: the http request handler
void request_handler(struct http_request_s *req);

// rcheck: returns true if the route in req matches the path and method
int rcheck(struct http_request_s *req, char *target, char *method);

// parse_url_encoded: parses the query out from the http_string_s
struct kvpairs parse_url_encoded(struct http_string_s parseme);

// free_kvpairs: frees a kvpairs array
void free_kvpairs(struct kvpairs pairs);

// getv: gets the value from a kvpair(s) given the key
char *getv(struct kvpairs *pairs, char *k);

// send_file: sends the file in the request, coalescing to '/index.html' from "html/"
int send_file(struct http_request_s *req, struct http_response_s *res, char *path);

// send_error: sends an error
int send_error(struct http_request_s *req, struct http_response_s *res, int errcode);

// get_list: returns the list page for a given table
int get_list(struct http_request_s *req, struct http_response_s *res, char *table);

// decode_string: decodes s, of length n into a normal string
char *decode_string(char *s);

// get_codepoint: returns an integer representing the percent encoded codepoint
int get_codepoint(char *s);

// xctoi: converts a hex char (ascii) to the corresponding integer value
int xctoi(char v);

// SERVER FUNCTIONS
// exercise_post: handles the POSTing of an exercise record
int recipe_post(struct http_request_s *req, struct http_response_s *res);

// recipe_put: handles the PUTting of a recipe record
int recipe_put(struct http_request_s *req, struct http_response_s *res);

// recipe_delete: handles the DELETEting of a recipe record
int recipe_delete(struct http_request_s *req, struct http_response_s *res);

// recipe_validation: returns true if the form fits a recipe
int recipe_validation(struct kvpairs *form);

// DATABASE FUNCTIONS
// recipe_insert: inserts the recipe from the form into the database
int recipe_insert(struct kvpairs *form);

// ingredients_insert: inserts all of the available ingredients
int ingredients_insert(struct kvpairs *form, s64 rowid);

// steps_insert: inserts all of the available steps, in the right order
int steps_insert(struct kvpairs *form, s64 rowid);

// tags_insert: inserts all of the tags
int tags_insert(struct kvpairs *form, s64 rowid);

#define SQLITE_ERRMSG(x) (fprintf(stderr, "Error: %s\n", sqlite3_errstr(rc)))

#define USAGE ("USAGE: %s <dbname>\n")

// handle_sigint: handles SIGINT so we can write to the database
void handle_sigint(int sig)
{
	sqlite3_close(db);
	exit(1);
}

int main(int argc, char **argv)
{
	struct http_server_s *server;

	if (argc < 2) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	init(argv[1], "schema.sql");

	server = http_server_init(PORT, request_handler);

	printf("listening on http://localhost:%d\n", PORT);

	signal(SIGINT, handle_sigint);

	http_server_listen(server);

	cleanup();

	return 0;
}

// rcheck: returns true if the route in req matches the path and method
int rcheck(struct http_request_s *req, char *target, char *method)
{
	struct http_string_s m, t;
	int i;

	if (req == NULL)
		return 0;
	if (target == NULL)
		return 0;
	if (method == NULL)
		return 0;

	m = http_request_method(req);

	for (i = 0; i < m.len && i < strlen(method); i++) {
		if (tolower(m.buf[i]) != tolower(method[i]))
			return 0;
	}

	t = http_request_target(req);

	if (t.len == 1 && t.buf[0] == '/' && strlen(target) > 1)
		return 0;

	// determine if we have query parameters, and what the "real length" of t is
	for (i = 0; i < t.len && t.buf[i] != '?'; i++)
		;

	if (strlen(target) != i)
		return 0;

	// stop checking the target before query parameters
	for (i = 0; i < t.len && i < strlen(target) && target[i] != '?'; i++) {
		if (tolower(t.buf[i]) != tolower(target[i]))
			return 0;
	}

	return 1;
}

// request_handler: the http request handler
void request_handler(struct http_request_s *req)
{
	struct http_response_s *res;
	int rc;

#define CHKERR(E) do { if ((rc) < 0) { send_error(req, res, (E)); } } while (0)

	res = http_response_init();

	// recipe endpoints
	if (rcheck(req, "/recipe", "GET")) { // send recipe form
		rc = send_file(req, res, "html/recipe.html");
		CHKERR(503);

	} else if (rcheck(req, "/recipe", "POST")) {
		rc = recipe_post(req, res);
		CHKERR(503);

	} else if (rcheck(req, "/recipe", "PUT")) {
		rc = recipe_put(req, res);
		CHKERR(503);

	} else if (rcheck(req, "/recipe", "DELETE")) {
		rc = recipe_delete(req, res);
		CHKERR(503);

	} else if (rcheck(req, "/list", "GET")) {
		rc = get_list(req, res, "v_list_recipe");
		CHKERR(503);

	// error handling and default-ish things
	} else if (rcheck(req, "/style.css", "GET")) {
		send_file(req, res, "html/style.css");
	} else if (rcheck(req, "/recipe.js", "GET")) {
		send_file(req, res, "html/recipe.js");
	} else if (rcheck(req, "/", "GET") || rcheck(req, "/index.html", "GET")) {
		send_file(req, res, "html/index.html");
	} else {
		send_error(req, res, 404);
	}
}

// get_list: returns the list page for a given table
int get_list(struct http_request_s *req, struct http_response_s *res, char *table)
{
	sqlite3_stmt *stmt;
	size_t page_siz, page_cnt;
	char *sort_col, *sort_ord;
	struct http_string_s z;
	struct kvpairs q;
	char *t;
	int rc, i, j;
	FILE *stream;
	char *stream_ptr;
	size_t stream_siz;
	char sql[BUFLARGE];

	// NOTE (Brian) we respond to the following query parameters
	// - page_siz the size of each page
	// - page_cnt the page number we're on
	// - sort_col the column to sort on
	// - sort_ord asc or desc

	z = http_request_target(req);
	q = parse_url_encoded(z);

	t = getv(&q, "page_siz");
	page_siz = t == NULL ? 20 : atoi(t);

	t = getv(&q, "page_cnt");
	page_cnt = t == NULL ? 0 : atoi(t);

	t = getv(&q, "sort_col"); // this only works because every table has a ts column
	sort_col = t == NULL ? "ts" : t;

	t = getv(&q, "sort_ord"); // coalesce this to asc if null, or not allowed
	sort_ord = t == NULL ? "asc" : (streq(t, "asc") || streq(t, "desc") ? t : "asc");

#define THE_SQL	("select * from %s order by %s %s limit %ld offset %ld;")
	// TODO (Brian) we need to evaluate this bit for SQL injection
	memset(sql, 0, sizeof sql);

	snprintf(sql, sizeof sql, THE_SQL, table,
		sort_col, sort_ord, page_siz, page_cnt * page_siz);

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		free_kvpairs(q);
		return -1;
	}
#undef THE_SQL

	// now, we have to print out some table information
	stream = open_memstream(&stream_ptr, &stream_siz);

#define SOUT(s,...) (fprintf(stream,s"\n",##__VA_ARGS__))

	{
		t = sys_readfile("html/prologue.html", NULL);
		SOUT("%s", t);
		free(t);
	}

	SOUT("<div align=\"center\">");
	SOUT("<table>");

	for (i = 0; (rc = sqlite3_step(stmt)) == SQLITE_ROW; i++) {
		if (i == 0) {
			SOUT("\t<thead>");
			SOUT("\t<tr>");
			for (j = 0; j < sqlite3_column_count(stmt); j++) {
				SOUT("\t\t<th>%s</th>", sqlite3_column_name(stmt, j));
			}
			SOUT("\t</tr>");
			SOUT("\t</thead>");
		}

		SOUT("\t<tbody>");
		SOUT("\t<tr>");
		for (j = 0; j < sqlite3_column_count(stmt); j++) {
			t = (char *)sqlite3_column_text(stmt, j);
			SOUT("\t\t<td>%s</td>", t);
		}
		SOUT("\t</tr>");
		SOUT("\t</tbody>");
	}

	SOUT("</table>");
	SOUT("</div>");

	{
		t = sys_readfile("html/epilogue.html", NULL);
		SOUT("%s", t);
		free(t);
	}

	fclose(stream);

#undef SOUT

	if (rc != SQLITE_DONE) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		free_kvpairs(q);
		return -1;
	}

	sqlite3_finalize(stmt);

	http_response_status(res, 200);
	http_response_header(res, "Content-Type", "text/html");
	http_response_body(res, stream_ptr, stream_siz);

	http_respond(req, res);

	free(stream_ptr);
	free_kvpairs(q);

	return 0;
}

// recipe_post: handles the POSTing of a recipe record
int recipe_post(struct http_request_s *req, struct http_response_s *res)
{
	struct kvpairs form;
	struct http_string_s body;
	s64 rowid;
	int rc;

	body = http_request_body(req);

	printf("%.*s\n", body.len, body.buf);

	form = parse_url_encoded(body);

	if (!recipe_validation(&form)) {
		ERR("invalid form data!\n");
		free_kvpairs(form);
		return -1;
	}

	rc = recipe_insert(&form);
	if (rc < 0) {
		ERR("couldn't insert recipe!\n");
		return -1;
	}

	rowid = rc;

	rc = ingredients_insert(&form, rowid);
	if (rc < 0) {
		ERR("couldn't insert ingredients!\n");
		goto recipe_post_error;
	}

	rc = steps_insert(&form, rowid);
	if (rc < 0) {
		ERR("couldn't insert steps!\n");
		goto recipe_post_error;
	}

	rc = tags_insert(&form, rowid);
	if (rc < 0) {
		ERR("couldn't insert tags!\n");
		goto recipe_post_error;
	}

	free_kvpairs(form);

	// send a simple success page
	send_file(req, res, "html/success.html");

	return 0;

recipe_post_error:
	free_kvpairs(form);
	return -1;
}

// recipe_put: handles the PUTting of a recipe record
int recipe_put(struct http_request_s *req, struct http_response_s *res)
{
	assert(0);
	return 0;
}

// recipe_delete: handles the DELETEting of a recipe record
int recipe_delete(struct http_request_s *req, struct http_response_s *res)
{
	assert(0);
	return 0;
}

// recipe_insert: inserts the recipe from the form into the database
int recipe_insert(struct kvpairs *form)
{
	sqlite3_stmt *stmt;
	char *sql;
	int rc;

	sql = "insert into recipe (name, prep_time, cook_time, note) values (?,?,?,?);";

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		return -1;
	}

	sqlite3_bind_text(stmt, 1, getv(form, "name"), -1, NULL);
	sqlite3_bind_int(stmt, 2, atoi(getv(form, "prep_time")));
	sqlite3_bind_int(stmt, 3, atoi(getv(form, "cook_time")));

	if (getv(form, "note") != NULL) {
		sqlite3_bind_text(stmt, 4, getv(form, "note"), -1, NULL);
	} else {
		sqlite3_bind_null(stmt, 4);
	}


	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		return -1;
	}

	sqlite3_finalize(stmt);

	return sqlite3_last_insert_rowid(db);
}

// ingredients_insert: inserts all of the available ingredients
int ingredients_insert(struct kvpairs *form, s64 rowid)
{
	struct kvpair *pair;
	char *ingredients[512];
	size_t i;
	size_t idx;
	char *s;

	memset(ingredients, 0, sizeof ingredients);

	for (i = 0; i < form->kvpair_len; i++) { // gather all of the ingredients
		pair = form->kvpair + i;
		if (regex("ingredients[.*]", pair->k)) {
			s = strchr(pair->k, '[') + 1;
			idx = atoi(s);
			if (idx < 512 && !ingredients[idx]) {
				ingredients[idx] = pair->v;
			} else {
				ERR("user sent out of bounds or duplicate index '%ld\n", i);
			}
		}
	}

	for (i = 0; i < 512; i++) {
		if (ingredients[i])
			printf("%ld : %s\n", i, ingredients[i]);
	}

	return 0;
}

// steps_insert: inserts all of the available steps, in the right order
int steps_insert(struct kvpairs *form, s64 rowid)
{
	return 0;
}

// tags_insert: inserts all of the tags
int tags_insert(struct kvpairs *form, s64 rowid)
{
	return 0;
}

// recipe_validation: returns true if the form fits a recipe
int recipe_validation(struct kvpairs *form)
{
	char *string_values[] = {
		"name",
		"ingredients[.*]",
		"steps[.*]",
		"tags[.*]",
		"note"
	};

	char *int_values[] = {
		"prep_time",
		"cook_time"
	};

	size_t i, j;
	int found;

	for (i = 0; i < form->kvpair_len; i++) {
		struct kvpair *pair = form->kvpair + i;

		found = 0;

		// check if this is a string value
		for (j = 0; !found && j < sizeof(string_values) / sizeof(char *); j++) {
			if (regex(string_values[j], pair->k))
				found = 1;
		}

		// check if this is an integer value
		for (j = 0; !found && j < sizeof(int_values) / sizeof(char *); j++) {
			if (regex(int_values[j], pair->k))
				found = 1;
		}

#if 0
		if (!found) // we found a key in the form that ISN'T a recipe-ish key
			return 0;
#else
		if (!found)
			printf("found '%s' in form\n", pair->k);
#endif
	}

	return 1;
}

// send_file: sends the file in the request, coalescing to '/index.html' from "html/"
int send_file(struct http_request_s *req, struct http_response_s *res, char *path)
{
	char *file_data;
	char *mime_type;
	size_t len;

	if (path == NULL) {
		return -1;
	}

	if (access(path, F_OK) == 0) {
		file_data = sys_readfile(path, &len);

		mime_type = (char *)magic_buffer(MAGIC_COOKIE, file_data, len);

		http_response_status(res, 200);
		http_response_header(res, "Content-Type", mime_type);
		http_response_body(res, file_data, len);

		http_respond(req, res);

		free(file_data);
	} else {
		send_error(req, res, 404);
	}

	return 0;
}

// send_error: sends an error
int send_error(struct http_request_s *req, struct http_response_s *res, int errcode)
{
	http_response_status(res, errcode);
	http_respond(req, res);

	return 0;
}

// is_uuid: returns true if the input string is a uuid
int is_uuid(char *id)
{
	int i;

	if (id == NULL) {
		return 0;
	}

	if (strlen(id) < 36) {
		return 0;
	}

	for (i = 0; i < 36; i++) { // ensure our chars are right
		switch (id[i]) {
		case '0': case '1': case '2': case '3': case '4': case '5': case '6':
		case '7': case '8': case '9': case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case '-':
				break;
		default:
			return 0;
		}
	}

	// index 13 should always be '4'
	return id[14] == '4';
}

// getv: gets the value from a kvpair(s) given the key
char *getv(struct kvpairs *pairs, char *k)
{
	size_t i;

	for (i = 0; i < pairs->kvpair_len; i++) {
		if (streq(pairs->kvpair[i].k, k))
			return pairs->kvpair[i].v;
	}

	return NULL;
}

// parse_url_encoded: parses the query out from the http_string_s
struct kvpairs parse_url_encoded(struct http_string_s parseme)
{
	struct kvpairs pairs;
	char *content;
	char *ss, *s;
	int i;

	memset(&pairs, 0, sizeof pairs);

	content = strndup(parseme.buf, parseme.len);

	ss = strchr(content, '?');
	if (ss == NULL) {
		ss = content;
	} else {
		ss++;
	}

	for (i = 0, s = strtok(ss, "=&"); s; i++, s = strtok(NULL, "=&")) {
		C_RESIZE(&pairs.kvpair);

		if (i % 2 == 0) {
			pairs.kvpair[pairs.kvpair_len].k = decode_string(s);
		} else {
			pairs.kvpair[pairs.kvpair_len].v = decode_string(s);
			pairs.kvpair_len++;
		}
	}

	free(content);

	return pairs;
}

// decode_string: decodes s, of length n into a normal string
char *decode_string(char *s)
{
	char *t;
	size_t i, j, n;

	n = strlen(s);
	t = calloc(sizeof(char), n * 2);

	for (i = 0, j = 0; i < n; i++, j++) {
		switch (s[i]) {
		case '+': {
			t[j] = ' ';
			break;
		}

		case '%': {
			t[j] = get_codepoint(s + i);
			i += 2;
			break;
		}

		default:
			  t[j] = s[i];
			break;
		}
	}

	return t;
}

// get_codepoint: returns an integer representing the percent encoded codepoint
int get_codepoint(char *s)
{
	int z;

	z = 0;

	if (!s)
		return -1;

	if (*s == '%')
		s++;

	z = xctoi(s[0]) * 0x10 + xctoi(s[1]);

	return z >= 0 ? z : -1;
}

// xctoi: converts a hex char (ascii) to the corresponding integer value
int xctoi(char v)
{
	switch (v) {
	case '0': return 0x00;
	case '1': return 0x01;
	case '2': return 0x02;
	case '3': return 0x03;
	case '4': return 0x04;
	case '5': return 0x05;
	case '6': return 0x06;
	case '7': return 0x07;
	case '8': return 0x08;
	case '9': return 0x09;
	case 'A': case 'a': return 0x0a;
	case 'B': case 'b': return 0x0b;
	case 'C': case 'c': return 0x0c;
	case 'D': case 'd': return 0x0d;
	case 'E': case 'e': return 0x0e;
	case 'F': case 'f': return 0x0f;
	default: return -1;
	}
}

int _xctoi(char v) {
	v = tolower(v);
	if ( v >= '0' || v <= '9' ) return v - '0';
	if ( v >= 'a' || v <= 'f' ) return v - 'a' + 10;
	return -1;
}

// free_kvpairs: frees a kvpairs array
void free_kvpairs(struct kvpairs pairs)
{
	size_t i;

	for (i = 0; i < pairs.kvpair_len; i++) {
		free(pairs.kvpair[i].k);
		free(pairs.kvpair[i].v);
	}

	free(pairs.kvpair);
}

// kvpair: constructor for a kvpair
struct kvpair kvpair(char *k, char *v)
{
	struct kvpair pair;

	pair.k = k;
	pair.v = v;

	return pair;
}

// init: initializes the program
void init(char *db_file_name, char *sql_file_name)
{
	int rc;

	// seed the rng machine if it hasn't been
	pcg_seed(&localrand, time(NULL) ^ (long)printf, (unsigned long)init);

	// setup libmagic
	MAGIC_COOKIE = magic_open(MAGIC_MIME);
	if (MAGIC_COOKIE == NULL) {
		fprintf(stderr, "%s", magic_error(MAGIC_COOKIE));
		exit(1);
	}

	if (magic_load(MAGIC_COOKIE, NULL) != 0) {
		fprintf(stderr, "cannot load magic database - %s\n", magic_error(MAGIC_COOKIE));
		magic_close(MAGIC_COOKIE);
	}

	rc = sqlite3_open(db_file_name, &db);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		exit(1);
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

	rc = create_tables(db, sql_file_name);
	if (rc < 0) {
		ERR("Critical error in creating sql tables!!\n");
		exit(1);
	}

#if 0
	char *err;
#define SQL_WAL_ENABLE ("PRAGMA journal_mode=WAL;")
	char *err;
	rc = sqlite3_exec(db, SQL_WAL_ENABLE, NULL, NULL, (char **)&err);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		exit(1);
	}
#endif
}

// cleanup: cleans up for a shutdown (probably doesn't ever happen)
void cleanup(void)
{
	sqlite3_close(db);
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

