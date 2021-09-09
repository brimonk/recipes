#ifndef USER_H
#define USER_H

// Brian Chrzanowski
// 2021-09-08 12:55:50

#include "httpserver.h"

#include "object.h"

enum {
	USER_CONTEXT_NONE,
	USER_CONTEXT_NEWUSER,
	USER_CONTEXT_LOGIN,
	USER_CONTEXT_NORMAL,
	USER_CONTEXT_TOTAL
};

struct User {
	char *id;

    char *username;
    char *email;

	// NOTE (Brian): lots of things are happening with passwords here.
	//
	// 'password' - /api/newuser, /api/login
	// 'verify'   - /api/newuser
	// 'hash'     - not a submittable field, internal to the database only
	//
	// When creating a new user, a user will provide (because they must)
	// - password
	// - verify
	// And if they match, validation passes.
	//
	// When loggin in, a user must provide
	// - password
	// This password is checked against the hashed value from the database. If
	// they match after a call to 'crypto_pwhash_str_verify', they can login.
	//
	// For normal requests and things, the only user information that comes
	// across the wire is the user's session, in a cookie. So, for normal
	// requests that would like to verify if a user has access to a thing,
	// they'll need to call 'user_from_session', which will query the database
	// to pull out the user object using the current session id.
	//
	// To help functions denote at what the current user object is for, you can
	// set the 'context' int to a USER_CONTEXT_* value.

    char *password;
    char *verify;
	char *hash;

	char *secret;

	int context;
};

// user_api_newuser: endpoint, /api/v1/user/create
int user_api_newuser(struct http_request_s *req, struct http_response_s *res);

// user_api_login: endpoint, /api/v1/user/login
int user_api_login(struct http_request_s *req, struct http_response_s *res);

// user_api_logout: endpoint, /api/v1/user/logout
int user_api_logout(struct http_request_s *req, struct http_response_s *res);

// user_from_json: creates a user from a json string input
struct User *user_from_json(char *s, size_t len);

// user_to_json: converts a user object to json
char *user_to_json(struct User *user);

// user_free: frees the user object
void user_free(struct User *user);

// user_newuser: creates a new user from a user structure
int user_newuser(struct User *user);

// user_validation: returns true if this is a valid user form
int user_validation(struct User *user);

// user_insert: handles the inserting of a user
int user_insert(struct User *user);

// user_login: logs the user in
int user_login(struct User *user);

// user_makesecret: generates a secret for the user
int user_makesecret(struct User *user);

// user_getsecret: fetches the user's secret from the database, based on username
int user_getsecret(struct User *user);

// user_logout: logs the user out, using nothing but the secret
int user_logout(char *secret);

// user_haspass: return true if the user in this object is legit
int user_haspass(struct User *user);

// user_getcookie: sets the user cookie on the response
char *user_getcookie(struct User *user);

// user_secret_from_request: returns the secret from the user's session
char *user_secret_from_request(struct http_request_s *req);

#endif // USER_H

