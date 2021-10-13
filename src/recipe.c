// Brian Chrzanowski
// 2021-06-08 20:04:01

#define COMMON_IMPLEMENTATION
#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mman.h>

#define HTTPSERVER_IMPL
#include "httpserver.h"

#include <magic.h>
#include <sodium.h>

#include "objects.h"

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

// cleanup: cleans up resources open so we can elegantly close the program
void cleanup(void);

// is_uuid: returns true if the input string is a uuid
int is_uuid(char *id);

// request_handler: the http request handler
void request_handler(struct http_request_s *req);

// rcheck: returns true if the route in req matches the path and method
int rcheck(struct http_request_s *req, char *target, char *method);

// parse_url_encoded: parses the query out from the http_string_s
struct object *parse_url_encoded(struct http_string_s parseme);

// send_style: sends the style sheet, and ensures the correct mime type is sent
int send_style(struct http_request_s *req, struct http_response_s *res, char *path);

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

handle_t handle;

#define USAGE ("USAGE: %s <dbname>\n")
#define SCHEMA ("src/schema.sql")

// handle_sigint: handles SIGINT so we can write to the database
void handle_sigint(int sig)
{
	printf("\n");
	exit(1);
}

int main(int argc, char **argv)
{
	struct http_server_s *server;

	if (argc < 2) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	init(argv[1]);

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

	// static files, unrelated to main CRUD operations
	} else if (rcheck(req, "/style.css", "GET")) {
		rc = send_file(req, res, "html/style.css");
        CHKERR(503);
#endif

	} else if (rcheck(req, "/", "GET") || rcheck(req, "/index.html", "GET")) {
		rc = send_file(req, res, "html/index.html");
        CHKERR(503);

	} else if (rcheck(req, "/ui.js", "GET")) {
		rc = send_file(req, res, "html/ui.js");
        CHKERR(503);

	} else if (rcheck(req, "/styles.css", "GET")) {
		rc = send_style(req, res, "html/styles.css");
        CHKERR(503);

	} else {
        SNDERR(404);
	}
}

// get_list: returns the list page for a given table
int get_list(struct http_request_s *req, struct http_response_s *res, char *table)
{
	return 0;

#if 0
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
#endif
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

	// char 13 should always be '4'
	return id[14] == '4';
}

// parse_url_encoded: parses the query out from the http_string_s
struct object *parse_url_encoded(struct http_string_s parseme)
{
	struct object *root, *curr;
	char pbuf[BUFSMALL];
	char *content;
	char *ss, *s;
	char *k, *v;
	int i;

	root = NULL;

	content = strndup(parseme.buf, parseme.len);

	ss = strchr(content, '?');
	if (ss == NULL) {
		ss = content;
	} else {
		ss++;
	}

	for (i = 0, s = strtok(ss, "=&"); s; i++, s = strtok(NULL, "=&")) {
		if (i % 2 == 0) {
			k = decode_string(s);
		} else {
			v = decode_string(s);

			snprintf(pbuf, sizeof pbuf, ".%s", k);

#if 0
			curr = object_deref_and_create(&root, pbuf);

			object_ss(curr, v);
#endif
		}
	}

	if (i % 2 == 1) {
		free(k);
	}

	free(content);

	return root;
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

// init : initializes the program
void init(char *fname)
{
	int rc;
	struct stat st;

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

	memset(&handle, 0, sizeof handle);

	handle.fd = open(fname, O_CREAT | O_RDWR, 0666);

	if (handle.fd < 0) {
		fprintf(stderr, "couldn't open file '%s'\n", fname);
		perror("open");
		exit(1);
	}

	stat(fname, &st);

	if (st.st_size == 0) {
		setup_store();
	}

	load_from_store();
}

// cleanup : cleans up for a shutdown (probably doesn't ever happen)
void cleanup(void)
{
	int rc;

	sync_to_store();

	munmap(handle.ptr, ((storeheader_t *)handle.ptr)->size);

	if (handle.fd >= 0) {
		close(handle.fd);
	}
}

// NOTE (Brian):
//
// We actually might want to implement something where we just serialize
// to the lump system every nowandagain, and we instead just have arrays that we
// can realloc and whatnot.

// setup_store : sets up storage
int setup_store(void)
{
	storeheader_t header;
	size_t sz;
	int rc;

	header.header = OBJECT_MAGIC;
	header.version = OBJECT_VERSION;

	sz = 0x1000;
	sz += DEFAULT_RECORD_CNT * sizeof(user_t);
	sz += DEFAULT_RECORD_CNT * sizeof(recipe_t);

	header.size = sz;

	header.lumps[RT_USER].type = RT_USER;
	header.lumps[RT_USER].size = sizeof(user_t);
	header.lumps[RT_USER].cnt = DEFAULT_RECORD_CNT;
	header.lumps[RT_USER].len = 0;
	header.lumps[RT_USER].start = 0x1000;

	header.lumps[RT_RECIPE].type = RT_RECIPE;
	header.lumps[RT_RECIPE].size = sizeof(recipe_t);
	header.lumps[RT_RECIPE].cnt = DEFAULT_RECORD_CNT;
	header.lumps[RT_RECIPE].len = 0;
	header.lumps[RT_RECIPE].start = // yikes
		header.lumps[RT_RECIPE - 1].start
			+ header.lumps[RT_RECIPE - 1].size * header.lumps[RT_RECIPE - 1].cnt;

	rc = ftruncate(handle.fd, sz);
	if (rc < 0) {
		fprintf(stderr, "couldn't setup the storage!\n");
		exit(1);
	}

	handle.size = sz;

	handle.ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, handle.fd, 0);

	close(handle.fd);
	handle.fd = -1;

	memcpy(handle.ptr, &header, sizeof header);

	// test

	user_t *user;

	user = (void *)((unsigned char *)handle.ptr) + header.lumps[RT_USER].start;

	strncpy((char *)user->username, "Brian Chrzanowski", sizeof(user->username));

	msync(handle.ptr, sz, MS_ASYNC);

	return 0;
}

// load_from_store : setup our memory mapped storage
int load_from_store(void)
{
	return 0;
}

int sync_to_store(void)
{
	return 0;
}

int grow_store(void)
{
	return 0;
}

