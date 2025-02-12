// Brian Chrzanowski
// 2021-06-08 20:04:01
//
// TODO (Brian)
// - convert to sqlite
//     - get uuid sqlite library (db will generate uuids for us)
//     - recipe insert
//     - recipe select
//     - recipe update
//     - recipe delete
//     - recipe search with pagination (should be done?)
//     - enforce ordering on textlist operations
// - prep_time / cook_time verification(?)
// - performance tests
//     - insert performance
//     - get performance
//     - search performance
//     - delete performance
// - ui
//     - tags are a dropdown
// - security
//     - email verification
//     - forgot password / email password reset
// - image support
//     - upload
//     - profile picture / icon
//     - retrieval via uri
// - user support
//     - email verification
//     - forgot password / email password reset
//     - settings page
//         - password change
//         - profile picture(?)
//         - email change(?)
// - cleanup
//     - why do we have an RNG situation?
// - navigation

#define COMMON_IMPLEMENTATION
#include "common.h"

#include <unistd.h>
#include <sys/types.h>

#include <magic.h>
#include <sodium.h>
#include <jansson.h>

#define MG_ENABLE_LOG 0
#include "mongoose.h"

#include "sqlite3.h"

#include "objects.h"

#include "recipe.h"
#include "user.h"
#include "tag.h"

#define PORT (2000)

static magic_t MAGIC_COOKIE;

sqlite3 *DATABASE;

// init: initializes the program
void init(char *fname);
// cleanup: cleans up everything from 'init'
void cleanup();

// event_handler : handles mongoose (web) events
void event_handler(struct mg_connection *conn, int ev, void *ev_data, void *fn_data);

// request_handler: the http request handler
void request_handler(struct mg_connection *conn, struct mg_http_message *hm);

// send_file_static : sends the static data JSON blob
int send_file_static(struct mg_connection *conn, struct mg_http_message *hm);
// send_file_mithriljs : sends the javascript for the ui to the user
int send_file_mithriljs(struct mg_connection *conn, struct mg_http_message *hm);
// send_file_uijs : sends the javascript for the ui to the user
int send_file_uijs(struct mg_connection *conn, struct mg_http_message *hm);
// send_file_styles : sends the styles file to the user
int send_file_styles(struct mg_connection *conn, struct mg_http_message *hm);
// send_file_index : sends the index file to the user
int send_file_index(struct mg_connection *conn, struct mg_http_message *hm);

// send_error: sends an error
int send_error(struct mg_connection *conn, int errcode);

// get_codepoint: returns an integer representing the percent encoded codepoint
int get_codepoint(char *s);

// xctoi: converts a hex char (ascii) to the corresponding integer value
int xctoi(char v);

#define USAGE ("USAGE: %s <dbname>\n")
#define SCHEMA ("src/schema.sql")

int running;

typedef struct RouteTableEntry {
	char *key;
	int (*value)(struct mg_connection *, struct mg_http_message *);
} RouteTableEntry;
RouteTableEntry *routes = NULL;

// handle_sigint: handles SIGINT so we can write to the database
void handle_sigint(int sig)
{
	running = false;
}

int main(int argc, char **argv)
{
	struct mg_mgr mgr;

	if (argc < 2) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	init(argv[1]);

	signal(SIGINT, handle_sigint);

	// setup the routing hashtable

	sh_new_strdup(routes);

	shput(routes, "POST /api/v1/recipe", (void *)recipe_api_post);
	shput(routes, "GET /api/v1/recipe/list", (void *)recipe_api_getlist);
	shput(routes, "GET /api/v1/recipe/:id", (void *)recipe_api_get);
	shput(routes, "PUT /api/v1/recipe/:id", (void *)recipe_api_put);
	shput(routes, "DELETE /api/v1/recipe/:id", (void *)recipe_api_delete);

	shput(routes, "POST /api/v1/newuser", (void *)user_api_newuser);
	shput(routes, "POST /api/v1/login", (void *)user_api_login);
	shput(routes, "POST /api/v1/logout", (void *)user_api_logout);
	shput(routes, "GET /api/v1/whoami", (void *)user_api_whoami);

	shput(routes, "GET /api/v1/tags", (void *)tag_api_getlist);

    for (size_t i = 0; i < hmlen(routes); i++) {
        printf("K: '%s', V: %p\n", routes[i].key, routes[i].value);
    }

	mg_mgr_init(&mgr);

	char url[BUFSMALL];

	snprintf(url, sizeof url, "http://0.0.0.0:%d", PORT);

	mg_http_listen(&mgr, url, event_handler, NULL);

	printf("listening on http://localhost:%d\n", PORT);

	for (running = true; running;) {
		mg_mgr_poll(&mgr, 1000);
	}

	mg_mgr_free(&mgr);

	shfree(routes);

    cleanup();

	return 0;
}

// event_handler : handles mongoose (web) events
void event_handler(struct mg_connection *conn, int ev, void *ev_data, void *fn_data)
{
	switch (ev) {
		case MG_EV_ERROR: {
			ERR("Mongoose Error! %s\n", (char *)ev_data);
			break;
		}

		case MG_EV_HTTP_MSG: {
			// mg_http_serve_dir(conn, ev_data, &opts);
			request_handler(conn, (struct mg_http_message *)ev_data);
			break;
		}

		case MG_EV_HTTP_CHUNK: {
			ERR("We don't know what to do here yet! Partial Message Chunk\n");
			break;
		}

		default: {
			break;
		}
	}
}

// format_target_string : format the incomming requets for the routing hashtable
int format_target_string(char *s, struct mg_http_message *hm, size_t len)
{
	size_t buflen;
	char *tok;
	char *hasq;
	char copy[BUFLARGE];
	char readfrom[BUFLARGE];

	// TODO (Brian):
	//
	// - make sure we uppercase (normalize, really) all of the data from the user. HTTP blows.
	// - ensure 'len' isn't larger than BUFLARGE

	assert(s != NULL);
	assert(hm != NULL);

	memset(copy, 0, sizeof copy);
	memset(readfrom, 0, sizeof readfrom);

	strncpy(readfrom, hm->uri.ptr, MIN(sizeof(readfrom), hm->uri.len));

	buflen = 0;

	for (tok = strtok(readfrom, "/"); tok != NULL && buflen <= len; tok = strtok(NULL, "/")) {
		if (isdigit(*tok)) {
			buflen += snprintf(copy + buflen, sizeof(copy) - buflen, "/:id");
		} else {
			hasq = strchr(tok, '?');
			if (hasq) {
				buflen += snprintf(copy + buflen, sizeof(copy) - buflen, "/%.*s", (int)(hasq - tok), tok);
			} else {
				buflen += snprintf(copy + buflen, sizeof(copy) - buflen, "/%s", tok);
			}
		}
	}

	if (copy[0] == '\0') {
		copy[0] = '/';
	}

	snprintf(s, len, "%.*s %s", (int)hm->method.len, hm->method.ptr, copy);

	return 0;
}

// request_handler: the http request handler
void request_handler(struct mg_connection *conn, struct mg_http_message *hm)
{
	int rc;
	int (*func) (struct mg_connection *conn, struct mg_http_message *hm);
	char buf[BUFLARGE];
	int route_index = 0;

#define SNDERR(E) send_error(conn, (E))
#define CHKERR(E) do { if ((rc) < 0) { send_error(conn, (E)); } } while (0)

	memset(buf, 0, sizeof buf);

	rc = format_target_string(buf, hm, sizeof buf);

	printf("%s\n", buf);

	if (shgeti(routes, buf) >= 0) {
		func = routes[route_index].value;
        printf("FUNCTION POINTER: %p\n", func);
		rc = func(conn, hm);
		CHKERR(503);
	} else {
        printf("FUNCTION POINTER NOT FOUND\n");
		struct mg_http_serve_opts opts = { .root_dir = "./html" };
		mg_http_serve_dir(conn, hm, &opts);
	}
}

// send_file_static : sends the static data JSON blob
int send_file_static(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct mg_http_serve_opts opts = {
		.mime_types = "application/json"
	};

	mg_http_serve_file(conn, hm, "src/static.json", &opts);

	return 0;
}

// send_file_mithriljs : sends the javascript for the ui to the user
int send_file_mithriljs(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct mg_http_serve_opts opts = {
		.mime_types = "text/javascript"
	};

	mg_http_serve_file(conn, hm, "lib/mithril.js", &opts);

	return 0;
}


// send_file_uijs : sends the javascript for the ui to the user
int send_file_uijs(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct mg_http_serve_opts opts = {
		.mime_types = "application/json"
	};

	mg_http_serve_file(conn, hm, "src/ui.js", &opts);

	return 0;
}

// send_file_styles : sends the styles file to the user
int send_file_styles(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct mg_http_serve_opts opts = {
		.mime_types = "text/css"
	};

	mg_http_serve_file(conn, hm, "src/styles.css", &opts);

	return 0;
}

// send_file_index : sends the index file to the user
int send_file_index(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct mg_http_serve_opts opts = {
		.mime_types = "text/css"
	};

	mg_http_serve_file(conn, hm, "src/index.html", &opts);

	return 0;

}

// send_error: sends an error
int send_error(struct mg_connection *conn, int errcode)
{
	mg_http_reply(conn, errcode, NULL, "");

	return 0;
}

// setup_sqlite: sets up sqlite on the global handle (DATABASE)
int setup_sqlite(char *fname)
{
	char *errmsg;
	int rc;

	rc = sqlite3_open(fname, &DATABASE);
	if (rc != SQLITE_OK) {
		ERR("sqlite3_open error: %s\n", sqlite3_errstr(rc));
		return -1;
	}

    // read in the schema, and execute it (more involved than I'd like...)
    {
        size_t schema_len = 0;
        char *schema = sys_readfile("./src/schema.sql", &schema_len);

        for (char *sql = trim(schema), *next = NULL; sql && strlen(sql) > 0; sql = trim(next)) {
            sqlite3_stmt *stmt = NULL;

            rc = sqlite3_prepare_v2(DATABASE, sql, -1, &stmt, (const char **)&next);
            if (rc != SQLITE_OK) {
                ERR("SQL Error while bootstrapping database! %s\n", sqlite3_errstr(rc));
                exit(1);
            }

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                printf("RC = %d\n", rc);
                printf("SQL %ld\n%s\n", strlen(sql), sql);
                ERR("SQL Execution failed while bootstrapping database! %s\n", sqlite3_errstr(rc));
                exit(1);
            }

            sqlite3_finalize(stmt);
        }

        free(schema);
    }

	// load our various extensions
	{
		sqlite3_enable_load_extension(DATABASE, true);

		rc = sqlite3_load_extension(DATABASE, "./sqlite3_uuid", "sqlite3_uuid_init", &errmsg);
		if (rc == SQLITE_ERROR) {
			ERR("Error loading 'uuid' extension: %s\n", errmsg);
			exit(1);
		}

		sqlite3_enable_load_extension(DATABASE, false);
	}

    // TODO (Brian) execute migrations in the future

	return 0;
}

// init : initializes the program
void init(char *fname)
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

    rc = sodium_init();
    if (rc < 0) {
        ERR("Couldn't initialize libsodium!\n");
        exit(1);
    }

	rc = setup_sqlite(fname);
	if (rc < 0) {
		ERR("Couldn't initialize sqlite!\n");
		exit(1);
	}
}

// cleanup: cleans up everything from 'init'
void cleanup()
{
    sqlite3_close(DATABASE);
    magic_close(MAGIC_COOKIE);
}
