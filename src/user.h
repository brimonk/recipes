#ifndef USER_H
#define USER_H

// Brian Chrzanowski
// 2021-09-08 12:55:50

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
int user_api_newuser(struct mg_connection *conn, struct mg_http_message *hm);

// user_api_login: endpoint, /api/v1/user/login
int user_api_login(struct mg_connection *conn, struct mg_http_message *hm);

// user_api_logout: endpoint, /api/v1/user/logout
int user_api_logout(struct mg_connection *conn, struct mg_http_message *hm);

#endif // USER_H

