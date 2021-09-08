// Brian Chrzanowski
// 2021-09-08 12:54:30
//
// User Functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "httpserver.h"
#include "sqlite3.h"

#include <sodium.h>

#include "common.h"

#include "user.h"
#include "object.h"

#define COOKIE_KEY ("session")

extern sqlite3 *db;

// user_api_newuser: endpoint, /api/v1/user/create
int user_api_newuser(struct http_request_s *req, struct http_response_s *res)
{
    struct User *user;
    struct http_string_s body;
    int rc;

    body = http_request_body(req);

    user = user_from_json((char *)body.buf, body.len);
    if (user == NULL) {
        ERR("could not create user object from JSON input!");
        return -1;
    }

	user->context = USER_CONTEXT_NEWUSER;

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

	rc = user_makesecret(user);
	if (rc < 0) {
		ERR("couldn't get user secret!");
		user_free(user);
		return -1;
	}

    rc = user_login(user);
    if (rc < 0) {
        ERR("couldn't log the user in!");
        user_free(user);
        return -1;
    }

    user_free(user);

    return 0;
}

// user_api_login: endpoint, /api/v1/user/login
int user_api_login(struct http_request_s *req, struct http_response_s *res)
{
	struct User *user;
	struct http_string_s body;
	int rc;

	body = http_request_body(req);

	user = user_from_json((char *)body.buf, body.len);
	if (user == NULL) {
		ERR("login failed - couldn't load a user object from request body!");
		return -1;
	}

	user->context = USER_CONTEXT_LOGIN;

	if (user_validation(user) < 0) {
		ERR("login failed - user validation error!");
		user_free(user);
		return -1;
	}

	if (user_haspass(user) < 0) {
		ERR("login failed - invalid password!");
		user_free(user);
		return -1;
	}

	rc = user_getsecret(user);
	if (rc < 0) {
		ERR("login failed - getting user secret failed!");
		user_free(user);
		return -1;
	}

	if (user->secret == NULL) {
		user_makesecret(user);
	}

	rc = user_login(user);
	if (rc < 0) {
		ERR("login failed - couldn't upsert the user session!");
		user_free(user);
		return -1;
	}

	user_free(user);

	return 0;
}

// user_api_logout: endpoint, /api/v1/user/logout
int user_api_logout(struct http_request_s *req, struct http_response_s *res)
{
	char *secret;
	int rc;

	secret = user_secret_from_request(req);
	if (secret == NULL) {
		ERR("logout failed - didn't find a cookie!\n");
	}

	rc = user_logout(secret);
	if (rc < 0) {
		ERR("logout failed - couldn't remove user_session record from database!\n");
		free(secret);
		return -1;
	}

	return 0;
}

// user_logout: logs the user out
int user_logout(char *secret)
{
	sqlite3_stmt *stmt;
	char *sql;
	int rc;

	sql = "delete from user_session where session_id = ?;";

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		return -1;
	}

	sqlite3_bind_text(stmt, 1, secret, -1, NULL);

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		ERR("could not delete user_session record!");
		sqlite3_finalize(stmt);
		return -1;
	}

	sqlite3_finalize(stmt);

    return 0;
}

// user_login: generates the secret, inserts into the session table
int user_login(struct User *user)
{
    sqlite3_stmt *stmt;
    char *sql;
    int rc;

    sql =
"insert into user_session (user_id, session_id) values "
"((select id from user where username = ?), ?) "
"on conflict (user_id) do update set "
"expires_ts = (strftime('%Y-%m-%dT%H:%M:%S', 'now', '+7 days')), "
"session_id = excluded.session_id;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        SQLITE_ERRMSG(rc);
		sqlite3_finalize(stmt);
		return -1;
    }

	sqlite3_bind_text(stmt, 1, user->username, -1, NULL);
	sqlite3_bind_text(stmt, 2, user->secret, -1, NULL);

	rc = sqlite3_step(stmt);

	if (rc != SQLITE_DONE) {
		ERR("could not upsert the session record!");
		sqlite3_finalize(stmt);
		return -1;
	}

	sqlite3_finalize(stmt);

	return 0;
}

// user_haspass: return true if the user in this object is legit (has the pass)
int user_haspass(struct User *user)
{
	sqlite3_stmt *stmt;
	char *sql;
	char *hash;
	int rc;

	if (user->username == NULL || user->password == NULL) {
		return -1;
	}

	sql = "select password from user where username = ?;";

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		SQLITE_ERRMSG(rc);
		return -1;
	}

	sqlite3_bind_text(stmt, 1, user->username, -1, NULL);

	rc = sqlite3_step(stmt);

	hash = strdup((char *)sqlite3_column_text(stmt, 0));

	sqlite3_finalize(stmt);

	rc = crypto_pwhash_str_verify(hash, user->password, strlen(user->password)) == -1;

	free(hash);

	return 0;
}

// user_makehash: hashes the password
int user_makehash(struct User *user)
{
	int rc;
    char hash[crypto_pwhash_STRBYTES];

	rc = crypto_pwhash_str(hash, user->password, strlen(user->password), 3, 1 << 20);
	if (rc < 0) {
		return -1;
	}

	user->hash = strdup(hash);

	return 0;
}

// user_insert: handles the inserting of a user
int user_insert(struct User *user)
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

// user_makesecret: generates a secret for the user
int user_makesecret(struct User *user)
{
    unsigned char session_id[32];
    char session_base64[128];
    s32 variant;

	if (user == NULL) {
		return -1;
	}

    sodium_memzero(session_id, sizeof session_id);
	sodium_memzero(session_base64, sizeof session_base64);
    randombytes_buf(session_id, sizeof session_id);

    variant = sodium_base64_VARIANT_URLSAFE_NO_PADDING;

	sodium_bin2base64(session_base64, sizeof session_base64, session_id, sizeof session_id, variant);

	user->secret = strndup(session_base64, 128);

	return 0;
}

// user_getsecret: fetches the user's secret from the database, based on username
int user_getsecret(struct User *user)
{
	sqlite3_stmt *stmt;
	char *sql;
	int rc;

	sql = "select session_id from user_session where user_id in (select id from user where username = ?);";

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		SQLITE_ERRMSG(rc);
		return -1;
	}

	sqlite3_bind_text(stmt, 1, user->username, -1, NULL);

	rc = sqlite3_step(stmt);

	user->secret = strdup((char *)sqlite3_column_text(stmt, 0));

	sqlite3_finalize(stmt);

	return 0;
}

// user_validation: returns true if this is a valid user form
int user_validation(struct User *user)
{
	// TODO (Brian) ensure that this won't fail on login (check ui code)

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

// user_secret_from_request: returns the secret from the user's session
char *user_secret_from_request(struct http_request_s *req)
{
	struct http_string_s header;
	char *s, *e;

	header = http_request_header(req, "cookie");

	// WARN (Brian)
	//
	// THERE BE DRAGONS.
	//
	// WE SURE ARE TRUSTING THE USER TO JUST PASS US SOME DECENT INFORMATION.
	// TOTALLY GONNA WANNA DO THIS AGAIN LATER. NEVER GONNA GIVE YOU UP. (heh)
	 
	s = (char *)header.buf;

	s = strstr(s, "session");
	if (s == NULL) {
		return NULL;
	}

	s = strchr(s, '=');
	if (s == NULL) {
		return NULL;
	}

	e = strchr(s, ';');
	if (e == NULL) {
		return NULL;
	}

	return strndup(s, e - s);
}

// user_from_session: creates a user from the database and the session key
struct User *user_from_session(struct http_request_s *req)
{
	// TODO (Brian) implement
	return NULL;
}

// user_from_json: creates a user from a json string input
struct User *user_from_json(char *s, size_t len)
{
    struct User *user;
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

    user->id = object_gs(object, ".id");
    user->username = object_gs(object, ".username");
    user->password = object_gs(object, ".password");
    user->verify = object_gs(object, ".verify");
    user->email = object_gs(object, ".email");

    object_free(object);

    return user;
}

// user_to_json: converts a user object to json
char *user_to_json(struct User *user)
{
	char *s;
	char *fmt;
	size_t ssize;

	// NOTE (Brian): Pretty reasonable decision to never send the password
	// or hash to a user, over the wire.

	fmt = "{\"username\":\"%s\",\"email\":\"%s\",\"secret\":\"%s\"}";

	ssize = snprintf(NULL, 0, fmt, user->username, user->email, user->secret);

	s = calloc(ssize, sizeof(*s));

	snprintf(s, ssize, fmt, user->username, user->email, user->secret);

	return s;
}

// user_free: frees the user object
void user_free(struct User *user)
{
    if (user) {
        free(user->username);
        free(user->password);
        free(user->verify);
        free(user->email);
        free(user->secret);
        free(user);
    }
}

