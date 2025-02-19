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

#define COOKIE_KEY ("session")
#define COOKIE_LEN (32)

extern sqlite3 *DATABASE;

// login_free: frees a login object
static void login_free(Login *login);

static void user_session_free(UserSession *session);

// login_from_json: parses a 'Login' request from some JSON input
Login *login_from_json(char *json);

// is_valid_login: returns true if a password verify succeeds, false if it fails
static int is_valid_login(Login *login)
{
	char *query = "select passwd_verify(?) from users where username = ?;";
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(DATABASE, query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		ERR("%s", sqlite3_errmsg(DATABASE));
		return false;
	}

	sqlite3_bind_text(stmt, 1, login->password, -1, NULL);
	sqlite3_bind_text(stmt, 2, login->username, -1, NULL);

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
	char *insert_query = "insert into user_sessions (user_id) values (?);";
	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(DATABASE, insert_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		ERR("%s", sqlite3_errmsg(DATABASE));
		return false;
	}

	sqlite3_bind_text(stmt, 1, login->username, -1, NULL);

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW) {
		// FAIL
		sqlite3_finalize(stmt);
		return NULL;
	}

	sqlite3_finalize(stmt);

	int64_t rowid = sqlite3_last_insert_rowid(DATABASE);

	char *select_query = "select session_id, expire_ts from user_sessions where user_row_id = ?;";

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

	mg_http_reply(conn, 200, NULL, "{\"session_id\":\"\",\"expire_ts\":\"\"}",
		session->session_id, session->expire_ts);

	user_session_free(session);

	return 0;
}

// user_api_logout: endpoint, POST - /api/v1/user/logout
int user_api_logout(struct mg_connection *conn, struct mg_http_message *hm)
{
	// TODO (brian) delete the user session record, and wipe the cookie in the response
	return 0;
}

// whoami_to_json : converts a WhoAmI structure to a json blob
char *whoami_to_json(WhoAmI *who)
{
	json_t *object;
	json_error_t error;
	char *json;

	object = json_pack(
		"{s:I,s:s,s:s}",
		"id", (json_int_t)who->id,
		"username", who->username,
		"email", who->email
		);

	if (object == NULL) {
		fprintf(stderr, "%s %d\n", error.text, error.position);
	}

	json = json_dumps(object, JSON_SORT_KEYS | JSON_COMPACT);

	json_decref(object);

	return json;
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
