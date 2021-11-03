#ifndef USER_H
#define USER_H

// Brian Chrzanowski
// 2021-09-08 12:55:50

#include "httpserver.h"

#include "objects.h"

// NOTE (Brian): we have different structures of data that the user can input at various points.
// At no time, do we ever expose a "User" record to anyone. That's just absurd. You get a cookie,
// and you can ping an endpoint to find out if you're logged in. That's it.

// NewUser : the results of the "new user" form submission
struct NewUser {
    char *username;
    char *email;
    char *password;
    char *verify;
};

// Login : the results of the Login Form
struct Login {
    char *username;
    char *password;
};

// UserSession : this data gets concatenated with a ":", and base64 encoded, and stored in a cookie
// This is what we use to determine if a user is who they say they are, and so on.
struct UserSession {
    user_id id;
    char *secret;
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

