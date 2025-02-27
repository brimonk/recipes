// Brian Chrzanowski
// 2021-09-08 12:54:30
//
// User Functions
//
// NOTE (Brian)
//
// From within the webserver itself, new users cannot, and should not, be created. Mostly to ensure
// that there's no security funniness.

#include "common.h"

#include "mongoose.h"
#include "sqlite3.h"

#include <sodium.h>
#include <jansson.h>

#include "user.h"
#include "objects.h"

#define SESSION_KEY "session"

extern sqlite3 *DATABASE;

// login_free: frees a login object
static void login_free(Login *login);

static void user_session_free(UserSession *session);

// login_from_json: parses a 'Login' request from some JSON input
Login *login_from_json(char *json);

// is_valid_login: returns true if a password verify succeeds, false if it fails
static int is_valid_login(Login *login)
{
	char *query = "select passwd_verify(?, password) from users where username = ?;";
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		ERR("%s", sqlite3_errmsg(DATABASE));
		return false;
	}

	sqlite3_bind_text(stmt, 1, login->password, -1, NULL);
	sqlite3_bind_text(stmt, 2, login->username, -1, NULL);
	sqlite3_bind_text(stmt, 3, login->username, -1, NULL);

	int is_valid = false;

	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		int tmp = sqlite3_column_int(stmt, 0);
		if (tmp) {
			is_valid = true;
		}
	}

	sqlite3_finalize(stmt);

	return is_valid;
}

// user_select: returns a user object
User *user_select(char *username)
{
	User *user = calloc(1, sizeof(*user));
	if (user == NULL) {
		return NULL;
	}

	char *query = "select id from users where username = ?;";
	int rc;
	sqlite3_stmt *stmt;
	char *id = NULL;

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		ERR("%s", sqlite3_errmsg(DATABASE));
		goto failure;
	}

	sqlite3_bind_text(stmt, 1, username, -1, NULL);

	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		id = strdup((char *)sqlite3_column_text(stmt, 0));
	} else {
		goto failure;
	}

	sqlite3_finalize(stmt);

	db_load_metadata_from_id(&user->metadata, "users", id);

	return user;

failure:
	if (user) free(user);
	if (stmt) sqlite3_finalize(stmt);
	if (id) free(id);
	return NULL;
}

// create_user_session: create a user-session from a login (it's assumed the login is valid)
UserSession *create_user_session(Login *login)
{
	char *insert_query = "insert into user_sessions (user_row_id) select rowid from users where username = ?;";
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(DATABASE, insert_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		ERR("%s", sqlite3_errmsg(DATABASE));
		return false;
	}

	sqlite3_bind_text(stmt, 1, login->username, -1, NULL);

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		ERR("failed to create a 'user_sessions' record! %s", sqlite3_errmsg(DATABASE));
		sqlite3_finalize(stmt);
		return NULL;
	}

	sqlite3_finalize(stmt);

	int64_t rowid = sqlite3_last_insert_rowid(DATABASE);

	char *select_query = "select session_id, expire_ts from user_sessions where rowid = ?;";

	UserSession *session = calloc(1, sizeof(*session));

	rc = sqlite3_prepare_v2(DATABASE, select_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		ERR("%s", sqlite3_errmsg(DATABASE));
		return false;
	}

	sqlite3_bind_int64(stmt, 1, rowid);

	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		session->session_id = strdup((char *)sqlite3_column_text(stmt, 0));
		session->expire_ts = strdup((char *)sqlite3_column_text(stmt, 1));
	}

	sqlite3_finalize(stmt);

	return session;
}

char *create_cookie_header(char *session_id, char *expire_ts, bool secure, bool expire_me)
{
	// NOTE (brian) in our database, we generate SANE date formats (YYYYmmdd-HHMMSS.FFF). However,
	// according to RFC-6265 (HTTP State Management Mechanism), we need to use an RFC-1126 date
	// (Requirements for Internet Hosts -- Application and Support), which is defined in RFC-2616
	// (Hypertext Transport Protocol -- HTTP/1.1).
	//
	// To do this, we convert between our good format using good old time.h, and read out the
	// date format this this cookie will need.
	//
	// - https://www.rfc-editor.org/rfc/rfc6265#section-4.1.1
	// - https://www.rfc-editor.org/rfc/rfc1123
	// - https://www.rfc-editor.org/rfc/rfc2616#section-3.3.1

	// NOTE (brian) this currently throws away the fractional part of the time string that we have.

	size_t header_len;
	char *header = NULL;

	char expires_str[32] = { 0 };

	if (!expire_me) {
		struct tm expires = { 0 };
		char *rv = strptime(expire_ts, "%Y%m%d-%H%M%S", &expires);
		if (rv == NULL) {
			ERR("could not parse the expire_ts on the user session!");
			return NULL;
		}

		time_t expires_time = mktime(&expires);
		if (gmtime_r(&expires_time, &expires) == NULL) {
			ERR("could not convert the expires time to UTC!");
			return NULL;
		}

		strftime(expires_str, sizeof expires_str, "%a %b %d %T %Y", &expires);

		FILE *fp = open_memstream(&header, &header_len);
		// TODO (brian) determine how we can tell if we're serving the site over http or https?
		fprintf(fp, "Set-Cookie: " SESSION_KEY "=%s; Expires=%s; %sHttpOnly\r\n",
			session_id,
			expires_str,
			secure ? "Secure; " : ""
		);
		fclose(fp);
	} else {
		struct tm expires = { 0 };
		time_t expires_time = time(NULL) - 60; // 1 minute ago?
		if (gmtime_r(&expires_time, &expires) == NULL) {
			ERR("could not convert the expires time to UTC!");
			return NULL;
		}

		strftime(expires_str, sizeof expires_str, "%a %b %d %T %Y", &expires);

		FILE *fp = open_memstream(&header, &header_len);
		// TODO (brian) determine how we can tell if we're serving the site over http or https?
		fprintf(fp, "Set-Cookie: " SESSION_KEY "=garbage; Expires=%s; %sHttpOnly\r\n",
			expires_str,
			secure ? "Secure; " : ""
		);
		fclose(fp);
	}

	return header;
}

// user_api_login: endpoint, POST - /api/v1/user/login
int user_api_login(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct Login *login;
	char *body;

	body = strndup(hm->body.ptr, hm->body.len);
	login = login_from_json(body);
	free(body);

	if (login == NULL) {
		ERR("couldn't parse user from json!");
		return -1;
	}

	if (!is_valid_login(login)) {
		ERR("invalid login!");
		mg_http_reply(conn, 403, NULL, "{\"error\":\"invalid login!\"}");
		login_free(login);
		return -1;
	}

	// If we have a valid login, at this point we need to create a session cookie, set this in a
	// header, and return a success to the user. Our session cookies will last for 1 week, and
	// upon any session token usage will refresh and give another week of existing.

	UserSession *session = create_user_session(login);
	if (session == NULL) {
		ERR("could not create a valid user session!");
		mg_http_reply(conn, 500, NULL, "{\"error\":\"internal error!\"}");
		login_free(login);
		return -1;
	}

	char *header = create_cookie_header(
		session->session_id,
		session->expire_ts,
		false, // TODO (brian) check for https
		false
	);

	mg_http_reply(conn, 200, header, "{\"session_id\":\"%s\",\"expire_ts\":\"%s\"}", session->session_id, session->expire_ts);

	free(header);
	user_session_free(session);
	login_free(login);

	return 0;
}

// user_api_logout: endpoint, POST - /api/v1/logout
int user_api_logout(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct mg_str *cookies = mg_http_get_header(hm, "Cookie");
	if (cookies == NULL) {
		mg_http_reply(conn, 200, NULL, "logout successful - not logged in");
		return 0;
	}

	struct mg_str session = mg_http_get_header_var(*cookies, mg_str(SESSION_KEY));

	do {
		char *delete_query = "delete from user_sessions where session_id = ?;";

		autofree_stmt sqlite3_stmt *stmt = NULL;
		int rc = 0;

		rc = sqlite3_prepare_v2(DATABASE, delete_query, -1, &stmt, NULL);
		if (rc != SQLITE_OK) {
			ERR("could not remove user session with id '%.s' from the database.", session.ptr, session.len);
			break;
		}

		sqlite3_bind_text(stmt, 1, session.ptr, session.len, NULL);

		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE) {
			ERR("could not remove user session with id '%.s' from the database.", session.ptr, session.len);
		}
	}
	while (0);

	// NOTE (brian) the standards compliant way to REMOVE a cookie is to give it a bogus value,
	// and expire it in the past.

	autofree char *header = create_cookie_header(
		NULL,
		NULL,
		false, // TODO (brian) check for https
		true
	);

	mg_http_reply(conn, 200, header, "");

	return 0;
}

// user_api_whoami: endpoint, GET - /api/v1/whoami
int user_api_whoami(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct mg_str *cookies = mg_http_get_header(hm, "Cookie");
	if (cookies == NULL) {
		mg_http_reply(conn, 401, NULL, "");
		return 0;
	}

	struct mg_str session = mg_http_get_header_var(*cookies, mg_str(SESSION_KEY));

	autofree char *user_id = NULL;

	do {
		MSG("Getting user_id for session: %.*s", session.len, session.ptr);

		char *select_query = "select u.id from users u inner join user_sessions us on u.rowid = us.user_row_id where us.session_id = ?;";

		autofree_stmt sqlite3_stmt *stmt = NULL;
		int rc = 0;

		rc = sqlite3_prepare_v2(DATABASE, select_query, -1, &stmt, NULL);
		if (rc != SQLITE_OK) {
			ERR("could not prepare statement!");
			break;
		}

		sqlite3_bind_text(stmt, 1, session.ptr, session.len, NULL);

		if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
			user_id = strdup((char *)sqlite3_column_text(stmt, 0));
		}
	}
	while (0);

	if (user_id == NULL) {
		mg_http_reply(conn, 401, NULL, "");
	} else {
		mg_http_reply(conn, 200, NULL, "{\"id\":\"%s\"}", user_id);
	}

	return 0;
}

// login_from_json: parses a 'Login' request from some JSON input
Login *login_from_json(char *json)
{
	json_error_t error;

	Login *login = calloc(1, sizeof(*login));
	if (login != NULL) {
		json_t *object = json_loads(json, 0, &error);
		if (json_is_object(object)) {
			login->username = strdup(json_string_value(json_object_get(object, "username")));
			login->password = strdup(json_string_value(json_object_get(object, "password")));
		}
		json_decref(object);
	}
	return login;
}

// login_free : frees the login 
static void login_free(Login *login)
{
	if (login) {
		free(login->username);
		free(login->password);
		free(login);
	}
}

static void user_session_free(UserSession *session)
{
	if (session) {
		free(session->session_id);
		free(session->expire_ts);
		free(session);
	}
}

// whoami_free : frees the strings and children, does not free this structure
void whoami_free(WhoAmI *who)
{
	if (who) {
		free(who->username);
		free(who->email);
		free(who);
	}
}
