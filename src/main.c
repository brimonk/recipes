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

#include "recipe.h"
#include "user.h"

#define PORT (2000)

// STATIC STUFF

static magic_t MAGIC_COOKIE;

// APPLICATION DATA STRUCTURES HERE
struct search {
    char *query;
    char *sort_column;
    char *sort_dir;
    size_t page;
    size_t size;
};

// APPLICATION FUNCTIONS HERE

// init: initializes the program
void init(char *fname);

// request_handler: the http request handler
void request_handler(struct http_request_s *req);

// rcheck: returns true if the route in req matches the path and method
int rcheck(struct http_request_s *req, char *target, char *method);

// send_style: sends the style sheet, and ensures the correct mime type is sent
int send_style(struct http_request_s *req, struct http_response_s *res, char *path);

// send_file: sends the file in the request, coalescing to '/index.html' from "html/"
int send_file(struct http_request_s *req, struct http_response_s *res, char *path);

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

struct http_server_s *server;

// handle_sigint: handles SIGINT so we can write to the database
void handle_sigint(int sig)
{
	store_write();
	store_free();

	http_server_free(server);

	printf("\n");
	exit(1);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	init(argv[1]);

	server = http_server_init(PORT, request_handler);

	printf("listening on http://localhost:%d\n", PORT);

	signal(SIGINT, handle_sigint);

	http_server_listen(server);

	http_server_free(server);

	store_write();

	return 0;
}

// rcheck: returns true if the route in req matches the path and method
int rcheck(struct http_request_s *req, char *target, char *method)
{
	struct http_string_s m, t;
    char *uri;
	int i, rc;

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
    uri = strndup(t.buf, t.len);

    if (strchr(uri, '?') != NULL) {
        (*strchr(uri, '?')) = 0;
    }

    rc = 1;

    for (i = 0; rc && i < strlen(uri) && i < strlen(target); i++) {
        if (tolower(uri[i]) != tolower(target[i])) {
            rc = 0;
        }
    }

    if (i != strlen(target)) {
        rc = 0;
    }

    free(uri);

	return rc;
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

#if 0
    // user endpoints
    } else if (rcheck(req, "/api/v1/user/create", "POST")) {
        rc = user_api_newuser(req, res);
        CHKERR(503);

    } else if (rcheck(req, "/api/v1/user/login", "POST")) {
		rc = user_api_login(req, res);
        CHKERR(503); // TODO (Brian) fix login too

	} else if (rcheck(req, "/api/v1/user/logout", "POST")) {
		rc = user_api_logout(req, res);
		CHKERR(503);
#endif

	// recipe endpoints
	} else if (rcheck(req, "/api/v1/recipe", "POST")) {
		rc = recipe_api_post(req, res);
		CHKERR(503);

	} else if (rcheck(req, "/api/v1/recipe/list", "GET")) {
		rc = recipe_api_getlist(req, res);
		CHKERR(503);

	} else if (rcheck(req, "/api/v1/recipe", "GET")) {
		rc = recipe_api_get(req, res);
		CHKERR(503);

	} else if (rcheck(req, "/api/v1/recipe", "PUT")) {
		rc = recipe_api_put(req, res);
		CHKERR(503);

	} else if (rcheck(req, "/api/v1/recipe", "DELETE")) {
		rc = recipe_api_delete(req, res);
		CHKERR(503);

	// static files, unrelated to main CRUD operations
	} else if (rcheck(req, "/ui.js", "GET")) {
		rc = send_file(req, res, "html/ui.js");
        CHKERR(503);

	} else if (rcheck(req, "/styles.css", "GET")) {
		rc = send_style(req, res, "html/styles.css");
        CHKERR(503);

	} else if (rcheck(req, "/", "GET") || rcheck(req, "/index.html", "GET")) {
		rc = send_file(req, res, "html/index.html");
        CHKERR(503);

	} else {
        SNDERR(404);
	}
}

// send_style: sends the style sheet, and ensures the correct mime type is sent
int send_style(struct http_request_s *req, struct http_response_s *res, char *path)
{
	char *file_data;
	size_t len;

    // NOTE (Brian) there's a small issue with libmagic setting the mime type for css correctly, so
    // I'm basically doing this myself. Without this, the server just sends "text/plain" as the
    // type, and the browser doesn't like that.

	if (path == NULL) {
		return -1;
	}

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

