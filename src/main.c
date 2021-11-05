// Brian Chrzanowski
// 2021-06-08 20:04:01

#define COMMON_IMPLEMENTATION
#include "common.h"

#include "store.h"

#include <unistd.h>
#include <sys/types.h>

#define HTTPSERVER_IMPL
#include "httpserver.h"

#include <magic.h>
#include <sodium.h>

#include "objects.h"
#include "ht.h"
#include "migrations.h"

#include "recipe.h"
#include "user.h"

#define PORT (2000)

// STATIC STUFF

static magic_t MAGIC_COOKIE;

// init: initializes the program
void init(char *fname);

// request_handler: the http request handler
void request_handler(struct http_request_s *req);

// rcheck: returns true if the route in req matches the path and method
int rcheck(struct http_request_s *req, char *target, char *method);

// send_style: sends the style sheet, and ensures the correct mime type is sent
int send_style(struct http_request_s *req, struct http_response_s *res);

// send_file: sends the file in the request, coalescing to '/index.html' from "html/"
int send_file(struct http_request_s *req, struct http_response_s *res);

// send_error: sends an error
int send_error(struct http_request_s *req, struct http_response_s *res, int errcode);

// get_list: returns the list page for a given table
int get_list(struct http_request_s *req, struct http_response_s *res, char *table);

// get_codepoint: returns an integer representing the percent encoded codepoint
int get_codepoint(char *s);

// xctoi: converts a hex char (ascii) to the corresponding integer value
int xctoi(char v);

extern handle_t handle;

#define USAGE ("USAGE: %s <dbname>\n")
#define SCHEMA ("src/schema.sql")

struct ht *routes;
struct http_server_s *server;

// handle_sigint: handles SIGINT so we can write to the database
void handle_sigint(int sig)
{
	store_write();
	store_free();

	http_server_free(server);

	ht_destroy(routes);

	printf("\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int rc;

	if (argc < 2) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	init(argv[1]);

	rc = migrations_exec();
	if (rc < 0) {
		ERR("couldn't apply migrations, quitting!\n");
		exit(1);
	}

	store_write();

	server = http_server_init(PORT, request_handler);

	printf("listening on http://localhost:%d\n", PORT);

	signal(SIGINT, handle_sigint);

	// setup the routing hashtable
	routes = ht_create();

	ht_set(routes, "POST /api/v1/recipe", (void *)recipe_api_post);
	ht_set(routes, "GET /api/v1/recipe/list", (void *)recipe_api_getlist);
	ht_set(routes, "GET /api/v1/recipe/:id", (void *)recipe_api_get);
	ht_set(routes, "PUT /api/v1/recipe/:id", (void *)recipe_api_put);
	ht_set(routes, "DELETE /api/v1/recipe/:id", (void *)recipe_api_delete);
	ht_set(routes, "GET /ui.js", (void *)send_file);
	ht_set(routes, "GET /styles.css", (void *)send_style);
	ht_set(routes, "GET /index.html", (void *)send_file);
	ht_set(routes, "GET /", (void *)send_file);

	http_server_listen(server);

	http_server_free(server);

	store_write();
	ht_destroy(routes);

	return 0;
}

// format_target_string : format the incomming requets for the routing hashtable
int format_target_string(char *s, struct http_request_s *req, size_t len)
{
	size_t buflen;
	struct http_string_s target, method;
	char *tok;
	char *hasq;
	char copy[BUFLARGE];
	char readfrom[BUFLARGE];

	// TODO (Brian):
	//
	// - make sure we uppercase (normalize, really) all of the data from the user. HTTP blows.
	// - ensure 'len' isn't larger than BUFLARGE

	memset(copy, 0, sizeof copy);
	memset(readfrom, 0, sizeof readfrom);

	assert(s != NULL);

	method = http_request_method(req);
	target = http_request_target(req);

	strncpy(readfrom, target.buf, MIN(sizeof(readfrom), target.len));

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

	snprintf(s, len, "%.*s %s", method.len, method.buf, copy);

	return 0;
}

// request_handler: the http request handler
void request_handler(struct http_request_s *req)
{
	struct http_response_s *res;
	int rc;
	int (*func)(struct http_request_s *, struct http_response_s *);
	char buf[BUFLARGE];

#define SNDERR(E) send_error(req, res, (E))
#define CHKERR(E) do { if ((rc) < 0) { send_error(req, res, (E)); } } while (0)

	memset(buf, 0, sizeof buf);

	rc = format_target_string(buf, req, sizeof buf);

	res = http_response_init();

	func = ht_get(routes, buf);

	if (func == NULL) {
        SNDERR(404);
	} else {
		rc = func(req, res);
		CHKERR(503);
	}
}

// send_style: sends the style sheet, and ensures the correct mime type is sent
int send_style(struct http_request_s *req, struct http_response_s *res)
{
	char *file_data;
	char *path;
	size_t len;

    // NOTE (Brian) there's a small issue with libmagic setting the mime type for css correctly, so
    // I'm basically doing this myself. Without this, the server just sends "text/plain" as the
    // type, and the browser doesn't like that.

	path = "html/styles.css";

	if (access(path, F_OK) == 0) {
		file_data = sys_readfile(path, &len);

		http_response_status(res, 200);
		http_response_header(res, "Content-Type", "text/css");
		http_response_body(res, file_data, len);

		http_respond(req, res);

		free(file_data);
	} else {
		send_error(req, res, 404);
	}

	return 0;
}

// send_file: sends the file in the request, coalescing to '/index.html' from "html/"
int send_file(struct http_request_s *req, struct http_response_s *res)
{
	struct http_string_s target;
	char *file_data;
	char *mime_type;
	char *requested;
	char *path;
	size_t len;

	// NOTE (Brian): To make this routing fit well in a hashtable (hopefully, to make routing
	// easier), this function distributes the two static files that we give a shit about.

	path = NULL;

	target = http_request_target(req);

	requested = strndup(target.buf, target.len);

	if (streq("/ui.js", requested)) {
		path = "html/ui.js";
	} else if (streq("/styles.css", requested)) {
		path = "html/styles.css";
	} else if (streq("/index.html", requested)) {
		path = "html/index.html";
	} else if (streq("/", requested)) {
		path = "html/index.html";
	}

	free(requested);

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

