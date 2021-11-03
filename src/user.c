// Brian Chrzanowski
// 2021-09-08 12:54:30
//
// User Functions

#include "common.h"

#include "httpserver.h"

#include <sodium.h>
#include <jansson.h>

#include "user.h"
#include "objects.h"

#define COOKIE_KEY ("session")

// newuser_verify : 
int newuser_verify(struct NewUser *newuser);

// newuser_from_json : converts a JSON string into a NewUser object
struct NewUser *newuser_from_json(char *json);

// newuser_free : frees the new user 
void newuser_free(struct NewUser *newuser);

// user_api_newuser: endpoint, POST - /api/v1/user/newuser
int user_api_newuser(struct http_request_s *req, struct http_response_s *res)
{
    struct NewUser *user;
    user_id id;
	struct http_string_s body;
    char *json;
	char tbuf[BUFLARGE];

	body = http_request_body(req);

	json = strndup(body.buf, body.len);
	if (json == NULL) { // return http error
		return -1;
	}

	user = newuser_from_json(json);
    if (user == NULL) {
        return -1;
    }

    free(json);

    if (newuser_verify(user) < 0) {
        ERR("user record invalid!\n");
        newuser_free(user);
        return -1;
    }

    id = newuser_add(user);
    if (id < 0) {
        ERR("couldn't save the user to the disk!\n");
        newuser_free(user);
        return -1;
    }

    snprintf(tbuf, sizeof tbuf, "{\"id\":%lld}", id);

    http_response_status(res, 200);

    http_response_body(res, tbuf, strlen(tbuf));

    http_respond(req, res);

    newuser_free(user);

    return 0;
}

// user_api_login: endpoint, POST - /api/v1/user/login
int user_api_login(struct http_request_s *req, struct http_response_s *res)
{
    return 0;
}

// user_api_logout: endpoint, POST - /api/v1/user/logout
int user_api_logout(struct http_request_s *req, struct http_response_s *res)
{
    return 0;
}

// newuser_verify : 
int newuser_verify(struct NewUser *newuser)
{
    return 0;
}

// newuser_add : 
int newuser_add(struct NewUser *newuser)
{
    return 0;
}

// newuser_from_json : converts a JSON string into a NewUser object
struct NewUser *newuser_from_json(char *s)
{
    struct NewUser *user;
    json_t *root;
    json_error_t error;
    size_t i;

    root = json_loads(s, 0, &error);
    if (root == NULL) {
        return NULL;
    }

    if (!json_is_object(root)) {
        json_decref(root);
        return NULL;
    }

    json_t *username, *email, *password, *verify;

    // get all of the regular values first
	username = json_object_get(root, "username");
	if (!json_is_string(username)) {
		json_decref(root);
		return NULL;
	}

	email = json_object_get(root, "email");
	if (!json_is_string(email)) {
		json_decref(root);
		return NULL;
	}

	password = json_object_get(root, "password");
	if (!json_is_string(password)) {
		json_decref(root);
		return NULL;
	}

	verify = json_object_get(root, "verify");
	if (!json_is_string(verify)) {
		json_decref(root);
		return NULL;
	}

    // now that we have all of the properties

    user = calloc(1, sizeof(*user));
    if (user == NULL) {
        json_decref(root);
        return NULL;
    }

    user->username = strdup(json_string_value(username));
    user->email = strdup(json_string_value(email));
    user->password = strdup(json_string_value(password));
    user->verify = strdup(json_string_value(verify));

    json_decref(root);

    return user;
}

// newuser_free : frees the new user 
void newuser_free(struct NewUser *newuser)
{
    if (newuser) {
        free(newuser->username);
        free(newuser->email);
        free(newuser->password);
        free(newuser->verify);
        free(newuser);
    }
}

// login_free : frees the login 
void login_free(struct Login *login)
{
    if (login) {
        free(login->username);
        free(login->password);
        free(login);
    }
}

