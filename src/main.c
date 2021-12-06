// Brian Chrzanowski
// 2021-06-08 20:04:01
//
// TODO (Brian)
// - New User
// - Login
// - Session Cookies
// - All Strings are in a HashMap
// - Timing Regex (Frontend / Backend)
// - HTTP Parsing Bug (Why?) (Library doesn't handle malformed inputs?)
//
// - Performance Tests
//
//   Full Recipe Tests:
//     - x1000 Recipes
//     - Create
//     - Read (Search)
//     - Read (Individual)
//     - Update
//     - Read (Search)
//     - Read (Individual)
//     - Delete
//
//   Full User Tests:
//     - 1000 Users
//     - Create
//     - Login
//
// - Tags are a Dropdown
//   I'm not sure how to actually make this look good on mobile.
//
// - Remove existing migration code, and remake migrations, one file each, so data structures can be
//   redefined for each state of the database.

#define COMMON_IMPLEMENTATION
#include "common.h"

#include "store.h"

#include <unistd.h>
#include <sys/types.h>

#include <magic.h>
#include <sodium.h>
#include <jansson.h>

#define MG_ENABLE_LOG 0
#include "mongoose.h"

#include "objects.h"
#include "ht.h"

#include "recipe.h"
#include "tag.h"
#include "user.h"

#define PORT (2000)

// STATIC STUFF

static magic_t MAGIC_COOKIE;

// init: initializes the program
void init(char *fname);

// event_handler : handles mongoose (web) events
void event_handler(struct mg_connection *conn, int ev, void *ev_data, void *fn_data);

// request_handler: the http request handler
void request_handler(struct mg_connection *conn, struct mg_http_message *hm);

// send_file_static : sends the static data JSON blob
int send_file_static(struct mg_connection *conn, struct mg_http_message *hm);
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

extern handle_t handle;

#define USAGE ("USAGE: %s <dbname>\n")
#define SCHEMA ("src/schema.sql")

int running;

struct ht *routes;

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

	store_write();

	signal(SIGINT, handle_sigint);

	// setup the routing hashtable
	routes = ht_create();

	ht_set(routes, "POST /api/v1/recipe", (void *)recipe_api_post);
	ht_set(routes, "GET /api/v1/recipe/list", (void *)recipe_api_getlist);
	ht_set(routes, "GET /api/v1/recipe/:id", (void *)recipe_api_get);
	ht_set(routes, "PUT /api/v1/recipe/:id", (void *)recipe_api_put);
	ht_set(routes, "DELETE /api/v1/recipe/:id", (void *)recipe_api_delete);

	ht_set(routes, "POST /api/v1/newuser", (void *)user_api_newuser);

	ht_set(routes, "GET /api/v1/tags", (void *)tag_api_getlist);

	ht_set(routes, "GET /api/v1/static", (void *)send_file_static);
	ht_set(routes, "GET /ui.js", (void *)send_file_uijs);
	ht_set(routes, "GET /styles.css", (void *)send_file_styles);
	ht_set(routes, "GET /index.html", (void *)send_file_index);
	ht_set(routes, "GET /", (void *)send_file_index);

	mg_mgr_init(&mgr);

	char url[BUFSMALL];

	snprintf(url, sizeof url, "http://0.0.0.0:%d", PORT);

	mg_http_listen(&mgr, url, event_handler, NULL);

	printf("listening on http://localhost:%d\n", PORT);

	for (running = true; running;) {
		mg_mgr_poll(&mgr, 1000);
	}

	mg_mgr_free(&mgr);

	store_write();

	ht_destroy(routes);

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

#define SNDERR(E) send_error(conn, (E))
#define CHKERR(E) do { if ((rc) < 0) { send_error(conn, (E)); } } while (0)

	memset(buf, 0, sizeof buf);

	rc = format_target_string(buf, hm, sizeof buf);

	printf("%s\n", buf);

	func = ht_get(routes, buf);

	if (func == NULL) {
        SNDERR(404);
	} else {
		rc = func(conn, hm);
		CHKERR(503);
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
	mg_http_reply(conn, errcode, NULL, NULL);

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

	rc = store_open(fname);
	if (rc < 0) {
		ERR("Couldn't initizlize the backing store!\n");
		exit(1);
	}
}

