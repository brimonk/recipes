#ifndef USER_H
#define USER_H

// Brian Chrzanowski
// 2021-09-08 12:55:50

#include "objects.h"

// NOTE (Brian): we have different structures of data that the user can input at various points.
// At no time, do we ever expose a "User" record to anyone. That's just absurd. You get a cookie,
// and you can ping an endpoint to find out if you're logged in. That's it.

typedef struct UI_NewUser {
    char *username;
    char *email;
    char *password;
    char *verify;
} UI_NewUser;

typedef struct UI_Login {
    char *username;
    char *password;
} UI_Login;

typedef struct UI_WhoAmI {
    char *id;
    char *username;
    char *email;
} UI_WhoAmI;

// UserSession : this data gets concatenated with a ":", and base64 encoded, and stored in a cookie
// This is what we use to determine if a user is who they say they are, and so on.
typedef struct UI_UserSession {
    char *id;
    char *secret;
} UI_UserSession;

// user_api_newuser: endpoint, /api/v1/user/create
int user_api_newuser(struct mg_connection *conn, struct mg_http_message *hm);

// user_api_login: endpoint, /api/v1/user/login
int user_api_login(struct mg_connection *conn, struct mg_http_message *hm);

// user_api_logout: endpoint, /api/v1/user/logout
int user_api_logout(struct mg_connection *conn, struct mg_http_message *hm);

// user_api_whoami: endpoint, /api/v1/user/whoami
int user_api_whoami(struct mg_connection *conn, struct mg_http_message *hm);

#endif // USER_H

