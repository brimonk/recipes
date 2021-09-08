// Brian Chrzanowski
// 2021-06-08 20:04:01

#define COMMON_IMPLEMENTATION
#include "common.h"

#define HTTPSERVER_IMPL
#include "httpserver.h"

#include "jsmn.h"

#include "sqlite3.h"

#include <magic.h>
#include <sodium.h>

#define PORT (2000)

struct kvpair {
	char *k;
	char *v;
};

struct kvpairs {
	struct kvpair *kvpair;
	size_t kvpair_len, kvpair_cap;
};

enum {
    OBJECT_UNDEFINED,
    OBJECT_NULL,
    OBJECT_BOOL,
    OBJECT_NUM,
    OBJECT_STRING,
    OBJECT_OBJECT,
    OBJECT_ARRAY,
    OBJECT_TOTAL
};

struct object {
    int type;
    char *k;
    union {
        char *v_string;
        f64 v_num;
        int v_bool;
    };
    struct object *next;
    struct object *child;
    struct object *parent;
};

static magic_t MAGIC_COOKIE;

static sqlite3 *db;

// APPLICATION DATA STRUCTURES HERE
struct user {
    char *username;
    char *password;
    char *verify;
    char *email;
};

struct search {
    char *query;
    char *sort_column;
    char *sort_dir;
    size_t page;
    size_t size;
};

struct recipe {
    char *name;

    int cook_time;
    int prep_time;
    int servings;

    char *notes;

    char **ingredients;
    char **steps;
    char **tags;
};

// APPLICATION FUNCTIONS HERE

// init: initializes the program
void init(char *db_file_name, char *sql_file_name);
// cleanup: cleans up resources open so we can elegantly close the program
void cleanup(void);

// object_from_json: creates an object from a list of json tokens
struct object *object_from_json(char *s, size_t len);

// object_from_tokens: recursive function building up the 'object' tree
struct object *object_from_tokens(char *s, jsmntok_t *tokens, size_t len);

// object_free: frees the object
void object_free(struct object *object);

// object_t: locates the object with 'path', returns the type
int object_t(struct object *object, char *path);

// object_s: locates the object with 'path', returns the string
char *object_s(struct object *object, char *path);

// object_n: locates the object with 'path', returns the number
double object_n(struct object *object, char *path);

// object_b: locates the object with 'path', returns the boolean
int object_b(struct object *object, char *path);

// object_o: finds the object with 'path', returns that object (NULL if it isn't an object)
struct object *object_o(struct object *object, char *path);

// object_a: finds the object with 'path', returns that object (NULL if it's an array)
struct object *object_a(struct object *object, char *path);

// object_from_path: locates the object with 'path'
struct object *object_from_path(struct object *root, char *path);

// user_fromjson: creates a user from a json string input
struct user *user_fromjson(char *s, size_t len);

// user_free: frees the user object
void user_free(struct user *user);

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

// RECIPE FUNCTIONS
//   SERVER FUNCTIONS
// recipe_post: handles the POSTing of a recipe record
int recipe_post(struct http_request_s *req, struct http_response_s *res);

// recipe_put: handles the PUTting of a recipe record
int recipe_put(struct http_request_s *req, struct http_response_s *res);

// recipe_delete: handles the DELETEting of a recipe record
int recipe_delete(struct http_request_s *req, struct http_response_s *res);

// recipe_validation: returns true if the form fits a recipe
int recipe_validation(struct kvpairs *form);

//   DATABASE FUNCTIONS
// recipe_insert: inserts the recipe from the form into the database
int recipe_insert(struct kvpairs *form);

// ingredients_insert: inserts all of the available ingredients
int ingredients_insert(struct kvpairs *form, s64 rowid);

// steps_insert: inserts all of the available steps, in the right order
int steps_insert(struct kvpairs *form, s64 rowid);

// tags_insert: inserts all of the tags
int tags_insert(struct kvpairs *form, s64 rowid);

// USER FUNCTIONS
//   SERVER FUNCTIONS
// user_post: handles the POSTing of a new user record
int user_post(struct http_request_s *req, struct http_response_s *res);

// user_insert: handles the inserting of a user
int user_insert(struct user *user);

// user_validation: returns true if this is a valid user form
int user_validation(struct user *user);

// user_login: logs the user in
int user_login(struct http_request_s *req, struct http_response_s *res, struct user *user);

// user_setcookie: sets up a safe cookie with a 32 byte integer as the user's session id
int user_setcookie(struct http_response_s *res, char *id);

// user_logout: logs the user out
int user_logout(struct http_request_s *req, struct http_response_s *res);

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

#define SNDERR(E) send_error(req, res, (E))
#define CHKERR(E) do { if ((rc) < 0) { send_error(req, res, (E)); } } while (0)

	res = http_response_init();

	if (0) {

    // user endpoints
    } else if (rcheck(req, "/api/v1/user/create", "POST")) {
        rc = user_post(req, res);
        CHKERR(503);

    } else if (rcheck(req, "/api/v1/user/login", "POST")) {
        rc = -1;
        CHKERR(503); // TODO (Brian) fix login too

        /*
	} else if (rcheck(req, "/newuser", "GET")) {
		rc = send_file(req, res, "html/newuser.html");
        CHKERR(503);

	} else if (rcheck(req, "/newuser.js", "GET")) {
		rc = send_file(req, res, "html/newuser.js");
        CHKERR(503);

    } else if (rcheck(req, "/newuser", "POST")) {
        rc = user_post(req, res);
        CHKERR(503);

    } else if (rcheck(req, "/login", "GET")) {
        rc = send_file(req, res, "html/login.html");
        CHKERR(503);

	} else if (rcheck(req, "/login.js", "GET")) {
		rc = send_file(req, res, "html/login.js");
		CHKERR(503);

    } else if (rcheck(req, "/login", "POST")) {
        rc = user_login(req, res);
        CHKERR(503);

    } else if (rcheck(req, "/logout", "GET")) {
        rc = user_logout(req, res);
        CHKERR(503);
        */

	// static files, unrelated to main CRUD operations
	} else if (rcheck(req, "/style.css", "GET")) {
		rc = send_file(req, res, "html/style.css");
        CHKERR(503);

	} else if (rcheck(req, "/", "GET") || rcheck(req, "/index.html", "GET")) {
		rc = send_file(req, res, "html/index.html");
        CHKERR(503);

	} else if (rcheck(req, "/ui.js", "GET")) {
		rc = send_file(req, res, "html/ui.js");
        CHKERR(503);

	} else {
        SNDERR(404);
	}
}

// user_logout: logs the user out
int user_logout(struct http_request_s *req, struct http_response_s *res)
{
    sqlite3_stmt *stmt;
    struct http_string_s h_cookie;
    char cookie[128];
    // char *username;
    char *sql;
    unsigned char blob[256];
    int variant;
    int rc;

    // NOTE (Brian) deletes the user's session record, and unsets the cookie

    variant = sodium_base64_VARIANT_URLSAFE_NO_PADDING;

    h_cookie = http_request_header(req, "Cookie");

    if (sizeof cookie < h_cookie.len) {
        return -1;
    }

    // TODO (Brian) this is super gross code, don't ship with this, it literally won't do the thing
    // all the time
    //
    // NO FOR REAL, THIS WILL NOT PROVIDE THE COOKIE AUTHORIZATION THAT WE NEED FOR PRODUCTION USE

    if (h_cookie.len >= 3 && !(h_cookie.buf[0] == 'i' && h_cookie.buf[1] == 'd' && h_cookie.buf[2] == '=')) {
        return -1;
    }

    memset(cookie, 0, sizeof cookie);
    memcpy(cookie, h_cookie.buf + 3, h_cookie.len - 3);

    int len;
    len = strlen(cookie) / 4 * 3;
    assert(sizeof blob >= len); // TODO (Brian) handle the case where the user's cookie is too big

    size_t bin_len;
    char *bin_end;

    rc = sodium_base642bin(blob, sizeof blob, cookie, strlen(cookie), NULL, &bin_len, (const char ** const)&bin_end, variant);
    if (rc < 0) {
        printf("parse error!\n");
        if (bin_end == NULL) {
            printf("bin end is null?\n");
        } else {
            printf("end is %ld bytes off\n", bin_end - (char *)blob);
        }
        // TODO (Brian) what?
    }

    sql = "delete from user_session where user_id = (select user_id from user_session where session_id = ?);";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        SQLITE_ERRMSG(rc);
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_bind_blob(stmt, 1, blob, bin_len, NULL);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        SQLITE_ERRMSG(rc);
        sqlite3_finalize(stmt);
        return -1;
    }

    send_file(req, res, "html/success.html");

    return 0;
}

// user_login: logs the user in
int user_login(struct http_request_s *req, struct http_response_s *res, struct user *user)
{
    sqlite3_stmt *stmt;
    char *username;
    char *password;
    char *id;
    char *hash;
    char *sql;
    int rc;

    username = user->username;
    password = user->password;

    if (username == NULL || password == NULL) {
        return -1;
    }

    // first, we have to get the password

    sql = "select id, password from user where username = ?;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        SQLITE_ERRMSG(rc);
        goto user_login_error;
    }

    sqlite3_bind_text(stmt, 1, username, -1, NULL);

    rc = sqlite3_step(stmt);

    id = strdup((char *)sqlite3_column_text(stmt, 0));
    hash = strdup((char *)sqlite3_column_text(stmt, 1));

    sqlite3_finalize(stmt);

    // now, we'll check it against the password we got
    // if they match, you can be logged in
    // if not, we'll send you to the error page temporarily
    if (crypto_pwhash_str_verify(hash, password, strlen(password)) == -1) {
        goto user_login_error;
    }

    // now that we're past this, we have to set the user cookie, and like,
    // redirect them to the home page

    rc = user_setcookie(res, id);
    if (rc < 0) {
        return -1;
    }

    // TODO (Brian) probably don't want to send the success file anymore

    send_file(req, res, "html/success.html");

    return 0;

user_login_error:
    sqlite3_finalize(stmt);
    return -1;
}

// user_setcookie: sets up a safe cookie with a 32 byte integer as the user's session id
int user_setcookie(struct http_response_s *res, char *id)
{
    unsigned char session_id[32];
    char session_base64[128];
    char cookie[BUFLARGE];
    sqlite3_stmt *stmt;
    char *sql;
    int rc;

    sodium_memzero(session_id, sizeof session_id);
    randombytes_buf(session_id, sizeof session_id);

    sql = "insert into user_session(user_id, session_id) values (?,?);";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, id, -1, NULL);
    sqlite3_bind_blob(stmt, 2, session_id, sizeof session_id, NULL);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        SQLITE_ERRMSG(rc);
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    // now that we have the session cookie, we can set it up, and send it to the user
    // we're setting
    //
    //   id=session_id
    //   Max-Age=(3600 * 24 * 7) (7 days)
    //
    // and in production, we're going to need to add the following attributes
    //
    //   Secure
    //   HttpOnly
    //   SameSite=Strict

    memset(cookie, 0, sizeof cookie);

    s32 variant;

    variant = sodium_base64_VARIANT_URLSAFE_NO_PADDING;

    {
        s32 len;
        len = sodium_base64_ENCODED_LEN(sizeof session_id, variant);
        assert(sizeof session_base64 >= len);
    }

    sodium_bin2base64(session_base64, sizeof session_base64, session_id, sizeof session_id, variant);

#define MAX_AGE (60 * 60 * 24 * 7)

#if 1
#define FMT_STR ("id=%s; Max-Age=%d")
    snprintf(cookie, sizeof cookie, FMT_STR, session_base64, MAX_AGE);
#undef FMT_STR
#else
#define FMT_STR ("id=%s; Max-Age=%d; Secure; HttpOnly; SameSite=Strict")
    snprintf(cookie, sizeof cookie, FMT_STR, session_base64, age);
#undef FMT_STR
#endif

#undef MAX_AGE

    http_response_header(res, "Set-Cookie", cookie);

    return 0;
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

	form = parse_url_encoded(body);

	if (!recipe_validation(&form)) {
		ERR("invalid form data!\n");
		free_kvpairs(form);
		return -1;
	}

	rc = recipe_insert(&form);
	if (rc < 0) {
		ERR("couldn't insert recipe!\n");
		goto recipe_post_error;
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
    sqlite3_stmt *stmt;
	char *ingredients[512];
	size_t i, j;
	size_t idx;
    char *sql;
	char *s;
    int rc;

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

    sql = "insert into ingredient (recipe_id, desc, sort) values ((select id from recipe where rowid = ?), ?, ?);";

	for (i = j = 0; i < 512; i++) {
		if (ingredients[i]) {
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                SQLITE_ERRMSG(rc);
                sqlite3_finalize(stmt);
                return -1;
            }

            rc = sqlite3_bind_int64(stmt, 1, rowid);
            rc = sqlite3_bind_text(stmt, 2, ingredients[i], -1, NULL);
            rc = sqlite3_bind_int(stmt, 3, (int)j++);

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                SQLITE_ERRMSG(rc);
                sqlite3_finalize(stmt);
                return -1;
            }

            sqlite3_finalize(stmt);
        }
	}

	return 0;
}

// steps_insert: inserts all of the available steps, in the right order
int steps_insert(struct kvpairs *form, s64 rowid)
{
	struct kvpair *pair;
    sqlite3_stmt *stmt;
	char *steps[512];
	size_t i, j;
	size_t idx;
    char *sql;
	char *s;
    int rc;

	memset(steps, 0, sizeof steps);

	for (i = 0; i < form->kvpair_len; i++) { // gather all of the steps
		pair = form->kvpair + i;
		if (regex("steps[.*]", pair->k)) {
			s = strchr(pair->k, '[') + 1;
			idx = atoi(s);
			if (idx < 512 && !steps[idx]) {
				steps[idx] = pair->v;
			} else {
				ERR("user sent out of bounds or duplicate index '%ld\n", i);
			}
		}
	}

    sql = "insert into step (recipe_id, text, sort) values ((select id from recipe where rowid = ?), ?, ?);";

	for (i = j = 0; i < 512; i++) {
		if (steps[i]) {
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                SQLITE_ERRMSG(rc);
                sqlite3_finalize(stmt);
                return -1;
            }

            rc = sqlite3_bind_int64(stmt, 1, rowid);
            rc = sqlite3_bind_text(stmt, 2, steps[i], -1, NULL);
            rc = sqlite3_bind_int(stmt, 3, (int)j++);

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                SQLITE_ERRMSG(rc);
                sqlite3_finalize(stmt);
                return -1;
            }

            sqlite3_finalize(stmt);
        }
	}

	return 0;
}

// tags_insert: inserts all of the tags
int tags_insert(struct kvpairs *form, s64 rowid)
{
	struct kvpair *pair;
    sqlite3_stmt *stmt;
	char *tags[512];
	size_t i;
	size_t idx;
    char *sql;
	char *s;
    int rc;

	memset(tags, 0, sizeof tags);

	for (i = 0; i < form->kvpair_len; i++) { // gather all of the tags
		pair = form->kvpair + i;
		if (regex("tags[.*]", pair->k)) {
			s = strchr(pair->k, '[') + 1;
			idx = atoi(s);
			if (idx < 512 && !tags[idx]) {
				tags[idx] = pair->v;
			} else {
				ERR("user sent out of bounds or duplicate index '%ld\n", i);
			}
		}
	}

    sql = "insert into tag (recipe_id, text) values ((select id from recipe where rowid = ?), ?);";

	for (i = 0; i < 512; i++) {
		if (tags[i]) {
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                SQLITE_ERRMSG(rc);
                sqlite3_finalize(stmt);
                return -1;
            }

            rc = sqlite3_bind_int64(stmt, 1, rowid);
            rc = sqlite3_bind_text(stmt, 2, tags[i], -1, NULL);

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                SQLITE_ERRMSG(rc);
                sqlite3_finalize(stmt);
                return -1;
            }

            sqlite3_finalize(stmt);
        }
	}

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

// user_post: handles the POSTing of a new user record
int user_post(struct http_request_s *req, struct http_response_s *res)
{
    struct user *user;
    struct http_string_s body;
    int rc;

    body = http_request_body(req);

    user = user_fromjson((char *)body.buf, body.len);
    if (user == NULL) {
        ERR("could not create user object from JSON input!");
        return -1;
    }

    if (user_validation(user) < 0) {
        ERR("user object invalid!");
        user_free(user);
        return -1;
    }

    rc = user_insert(user);
    if (rc < 0) {
        ERR("couldn't insert the user!");
        user_free(user);
        return -1;
    }

    rc = user_login(req, res, user);
    if (rc < 0) {
        ERR("couldn't log the user in!");
        user_free(user);
        return -1;
    }

    user_free(user);

    return 0;
}

// user_insert: handles the inserting of a user
int user_insert(struct user *user)
{
    sqlite3_stmt *stmt;
    char hash[crypto_pwhash_STRBYTES];
    char *sql;
    int rc;

    rc = crypto_pwhash_str(hash, user->password, strlen(user->password), 3, 1 << 20);
    if (rc < 0) {
        return -1;
    }

    sql = "insert into user (username, email, password) values (?,?,?);";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user->username, -1, NULL);
    sqlite3_bind_text(stmt, 2, user->email, -1, NULL);
    sqlite3_bind_text(stmt, 3, hash, -1, NULL);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        SQLITE_ERRMSG(rc);
        return -1;
    }

    sqlite3_finalize(stmt);

    return 0;
}

// user_validation: returns true if this is a valid user form
int user_validation(struct user *user)
{
    // username
    if (user->username == NULL) {
        return -1;
    } else if (strlen(user->username) == 0) {
        return -1;
    } else if (strlen(user->username) > 50) {
        return -1;
    } else if (strchr(user->username, ' ') != NULL) {
        return -1;
    }

    // email
    if (user->email == NULL) {
        return -1;
    } else if (strlen(user->email) == 0) {
        return -1;
    }
    // TODO (Brian) email regex

    // password
    if (user->password == NULL) {
        return -1;
    } else if (strlen(user->password) < 6) {
        return -1;
    }

    // verify_password
    if (user->verify == NULL) {
        return -1;
    } else if (strlen(user->verify) == 0) {
        return -1;
    } else if (strcmp(user->password, user->verify) != 0) {
        return -1;
    }

    return 0;
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

// user_fromjson: creates a user from a json string input
struct user *user_fromjson(char *s, size_t len)
{
    struct user *user;
    struct object *object;

    object = object_from_json(s, len);
    if (object == NULL) {
        return NULL;
    }

    user = calloc(1, sizeof(*user));
    if (user == NULL) {
        object_free(object);
        return NULL;
    }

    user->username = object_s(object, ".username");
    user->password = object_s(object, ".password");
    user->verify = object_s(object, ".verify");
    user->email = object_s(object, ".email");

    object_free(object);

    return user;
}

// user_free: frees the user object
void user_free(struct user *user)
{
    if (user) {
        free(user->username);
        free(user->password);
        free(user->verify);
        free(user->email);
        free(user);
    }
}

// object_t: locates the object with 'path', returns the type
int object_t(struct object *object, char *path)
{
    struct object *z;

    z = object_from_path(object, path);

    return z == NULL ? OBJECT_UNDEFINED : z->type;
}

// object_s: locates the object with 'path', returns the string
char *object_s(struct object *object, char *path)
{
    struct object *z;

    z = object_from_path(object, path);

    return z == NULL ? NULL : strdup(z->v_string);
}

// object_n: locates the object with 'path', returns the number
double object_n(struct object *object, char *path)
{
    struct object *z;

    z = object_from_path(object, path);

    return z == NULL ? 0.0 : z->v_num;
}

// object_b: locates the object with 'path', returns the boolean
int object_b(struct object *object, char *path)
{
    struct object *z;

    z = object_from_path(object, path);

    return z == NULL ? -1 : z->v_bool;
}

// object_o: finds the object with 'path', returns that object (NULL if it isn't an object)
struct object *object_o(struct object *object, char *path)
{
    struct object *z;

    z = object_from_path(object, path);

    return z != NULL && z->type == OBJECT_OBJECT ? z : NULL;
}

// object_a: finds the object with 'path', returns that object (NULL if it's an array)
struct object *object_a(struct object *object, char *path)
{
    struct object *z;

    z = object_from_path(object, path);

    return z != NULL && z->type == OBJECT_ARRAY ? z : NULL;
}

// object_from_path: locates the object with 'path'
struct object *object_from_path(struct object *root, char *path)
{
    // NOTE (Brian) this function basically attempts to dereference strings until we get to an
    // object / array dereference, then we return an attempt in the child to dereference.

    struct object *curr;
    char *k, *e;
    size_t len;

    if (root == NULL || path == NULL)
        return NULL;

    k = path;

    if (k[0] == '.' || k[0] == '[') {
        return object_from_path(root->child, path + 1);
    } else {
        for (e = k; *e && *e != '.' && *e != '[' && *e != ']'; e++)
            ;

        len = e - k;

        for (curr = root; curr; curr = curr->next) {
            if (strncmp(k, curr->k, len) == 0) {
                return curr;
            }
        }

        return NULL;
    }
}

// object_from_json: creates an object from a list of json tokens
struct object *object_from_json(char *s, size_t len)
{
    jsmn_parser parser;
    jsmntok_t tokens[1024];
    int rc;

    jsmn_init(&parser);

    rc = jsmn_parse(&parser, s, len, tokens, ARRSIZE(tokens));

    if (rc < 0) {
        return NULL;
    }

    return object_from_tokens(s, tokens, rc);
}

// object_from_tokens: recursive function building up the 'object' tree
struct object *object_from_tokens(char *s, jsmntok_t *tokens, size_t len)
{
    struct object *object;
    struct object *curr;
    size_t i;
    char *vstr;

    jsmntok_t key, val;

    if (len <= 0) {
        return NULL;
    }

    object = calloc(1, sizeof(*object));

    // do things based on what the key is
    if (tokens[0].type == JSMN_OBJECT) {
        object->type = OBJECT_OBJECT;

        object->child = object_from_tokens(s, tokens + 1, tokens[0].size);
        object->child->parent = object;
    } else if (tokens[0].type == JSMN_ARRAY) {
        object->type = OBJECT_ARRAY;

        object->child = object_from_tokens(s, tokens + 1, tokens[0].size);
        object->child->parent = object;
    } else {

        // NOTE (Brian): here's the part where we parse all of the k/v pairs of the JSON object.
        // It's the case that when we get into this block, 'len' has the number of tokens left to
        // parse. Given that while in this block we parse two tokens at a time (key and value), we
        // simply continue until we have < 1 tokens, then we return

        curr = object;

        for (i = 0; i < len; i++) {
            if (0 < i) {
                curr->next = calloc(1, sizeof(*curr->next));
                curr = curr->next;
            }

            key = tokens[i * 2 + 0];
            val = tokens[i * 2 + 1];

            vstr = s + val.start;

            curr->k = strndup(s + key.start, key.end - key.start);

            if (val.type == JSMN_STRING) {
                curr->type = OBJECT_STRING;
                curr->v_string = strndup(s + val.start, val.end - val.start);
            } else if (vstr[0] == 't' || vstr[0] == 'f') { // check if value is boolean
                curr->type = OBJECT_BOOL;
                curr->v_bool = vstr[0] == 't';
            } else if (isdigit(vstr[0]) || vstr[0] == '-') { // check if value is number
                curr->type = OBJECT_NUM;
                curr->v_num = atof(s + val.start);
            } else if (vstr[0] == 'n') { // if it's NULL
                curr->type = OBJECT_NULL;
            } else {
                assert(0); // HOW???
            }
        }
    }

    return object;
}

// object_free: frees the object
void object_free(struct object *object)
{
    if (object) {
        if (object->child)
            object_free(object->child);

        if (object->next)
            object_free(object->next);

        free(object->k);

        if (object->type == OBJECT_STRING)
            free(object->v_string);
    }
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

    rc = sodium_init();
    if (rc < 0) {
        ERR("Couldn't initialize libsodium!\n");
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

