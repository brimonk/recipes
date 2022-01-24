// Brian Chrzanowski
// 2021-09-08 12:54:30
//
// User Functions

#include "common.h"

#include "mongoose.h"

#include <sodium.h>
#include <jansson.h>

#include "user.h"
#include "objects.h"
#include "store.h"

#define COOKIE_KEY ("session")
#define COOKIE_LEN (32)

// newuser_verify : returns true if the new user record is valid
int newuser_verify(struct NewUser *newuser);

// newuser_from_json : converts a JSON string into a NewUser object
struct NewUser *newuser_from_json(char *json);

// user_from_session : takes the cookie, scans the user session table, returns user_id
struct user_t *user_from_session(char *cookie);

// newuser_add : adds the new user into the user table
int newuser_add(struct NewUser *newuser);

// user_from_httpmessage : just a shortcut that gets used in multiple places
struct user_t *user_from_httpmessage(struct mg_http_message *hm);

// newuser_free : frees the new user 
void newuser_free(struct NewUser *newuser);

// whoami_free : frees the strings and children, does not free this structure
void whoami_free(struct WhoAmI *who);

// login_free : frees the login 
void login_free(struct Login *login);

// login_from_json : parses a login structure from the input json
struct Login *login_from_json(char *s);

// user_set_cookie : write the header to set the user cookie in 's'
int user_set_cookie(char *s, user_id id, size_t len);

// user_from_credentials : returns a user pointer from the input credentials
struct user_t *user_from_credentials(char *username, char *password);

// user_to_whoami : converts a user structure to a whoami structure
struct WhoAmI *user_to_whoami(struct user_t *user);

// whoami_to_json : converts a WhoAmI structure to a json blob
char *whoami_to_json(struct WhoAmI *who);

// user_api_newuser: endpoint, POST - /api/v1/newuser
int user_api_newuser(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct NewUser *user;
	user_id id;
	char *json;
	char cookie[BUFLARGE];

	json = strndup(hm->body.ptr, hm->body.len);
	if (json == NULL) { // return http error
		ERR("message has no body\n");
		return -1;
	}

	user = newuser_from_json(json);
	if (user == NULL) {
		ERR("couldn't convert body into a newuser record\n");
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

	// TODO (Brian): set the user cookie here to whatever's in the user session
	user_set_cookie(cookie, id, sizeof cookie);

	mg_http_reply(conn, 200, cookie, "{\"id\":%lld}", id);

	newuser_free(user);

	return 0;
}

// user_api_login: endpoint, POST - /api/v1/user/login
int user_api_login(struct mg_connection *conn, struct mg_http_message *hm)
{
    struct Login *login;
    struct user_t *user;
    char *json;

    json = strndup(hm->body.ptr, hm->body.len);
    if (json == NULL) {
        return -1;
    }

    login = login_from_json(json);
    free(json);

    if (login == NULL) {
        return -1;
    }

    user = user_from_credentials(login->username, login->password);
    login_free(login);

    int rc = user == NULL ? -1 : 0;

    if (rc < 0) {
        return -1;
    }

    mg_http_reply(conn, 200, NULL, "");

	return 0;
}

// user_api_logout: endpoint, POST - /api/v1/user/logout
int user_api_logout(struct mg_connection *conn, struct mg_http_message *hm)
{
	return 0;
}

// user_api_whoami: endpoint, GET - /api/v1/user/whoami
int user_api_whoami(struct mg_connection *conn, struct mg_http_message *hm)
{
	struct WhoAmI *who;
	struct user_t *user;
	char *json;

    user = user_from_httpmessage(hm);
    if (user == NULL) {
        return -1;
    }

	who = user_to_whoami(user);
	if (who == NULL) {
		return -1;
	}

	json = whoami_to_json(who);

	mg_http_reply(conn, 200, NULL, json);

	whoami_free(who);

	free(json);

	return 0;
}

// user_from_httpmessage : just a shortcut that gets used in multiple places
struct user_t *user_from_httpmessage(struct mg_http_message *hm)
{
    struct mg_str *mg_cookie;
    struct mg_str token;
	struct user_t *user;
    char *cookie;

    mg_cookie = mg_http_get_header(hm, "Cookie");
    if (mg_cookie == NULL) {
        return NULL;
    }

    token = mg_http_get_header_var(*mg_cookie, mg_str("session"));
    if (token.ptr == NULL) {
        return NULL;
    }

    cookie = strndup(token.ptr, token.len);

    user = user_from_session(cookie);
    if (user == NULL) {
        free(cookie);
        return NULL;
    }

    free(cookie);

    return user;
}

// user_from_credentials : returns a user pointer from the input credentials
struct user_t *user_from_credentials(char *username, char *password)
{
    size_t i;
    struct user_t *user;
    struct string_128_t *s_username;
    struct string_128_t *s_password;
    size_t passlen;
    int rc;

    passlen = strlen(password);

    for (i = 1; i <= store_getlen(RT_USER); i++) {
        user = store_getobj(RT_USER, i);
        if (user == NULL)
            continue;

        s_username = store_getobj(RT_STRING128, user->username);
        if (s_username == NULL)
            continue; // how...?

        if (!streq(s_username->string, username))
            continue;

        s_password = store_getobj(RT_STRING128, user->password);
        if (s_password == NULL)
            continue; // how...?

        rc = crypto_pwhash_str_verify(s_password->string, password, passlen);

        if (rc == 0)
            return user;
    }

    return NULL;
}

// user_to_whoami : converts a user structure to a whoami structure
struct WhoAmI *user_to_whoami(struct user_t *user)
{
	struct WhoAmI *who;
	string_128_t *string;

	if (user == NULL) {
		return NULL;
	}

	who = calloc(1, sizeof(*who));

	who->id = user->base.id;

	string = store_getobj(RT_STRING128, user->username);
	who->username = strndup(string->string, sizeof(string->string));

	string = store_getobj(RT_STRING128, user->email);
	who->email = strndup(string->string, sizeof(string->string));

	return who;
}

// user_from_session : takes the cookie, scans the user session table, returns user_id
struct user_t *user_from_session(char *cookie)
{
	size_t i;
	struct user_t *user;
	string_128_t *secret;

	for (i = 1; i <= store_getlen(RT_USER); i++) {
		if ((user = store_getobj(RT_USER, i)) != NULL) {
			if (user->session_secret != 0 && (secret = store_getobj(RT_STRING128, user->session_secret)) != NULL) {
				if (streq(cookie, secret->string)) {
					return user;
				}
			}
		}
	}

	return NULL;
}

// newuser_verify : returns false if the newuser doesn't pass validation
int newuser_verify(struct NewUser *newuser)
{
	// TODO (Brian)
	//
	// - server side email regex / email checking
	// - server side validation needs to return errors

	if (newuser == NULL)
		return 0;

	if (newuser->username == NULL || strlen(newuser->username) > 128)
		return 0;

	if (newuser->email == NULL || strlen(newuser->email) > 128)
		return 0;

	if (newuser->password == NULL || strlen(newuser->password) > 128)
		return 0;

	if (newuser->verify == NULL || strlen(newuser->verify) > 128)
		return 0;

	if (!streq(newuser->password, newuser->verify))
		return 0;

	return 1;
}

// newuser_add : adds the new user into the user table
int newuser_add(struct NewUser *newuser)
{
	user_t *user_record;
	int rc;

	string_128_t *username;
	string_128_t *email;
	string_128_t *password;
	string_128_t *salt;
	string_128_t *session_secret;

	unsigned char tbuf[BUFSMALL];

	// NOTE (Brian): There's a lot that's going on in here, and I hope I can convey it all to you
	// (me).
	//
	// First, clearly, we store the username and email address in plain text. I feel like there's a
	// universe where you may be able to hash those, but I'm not sure. Especially if you want to put
	// who created the recipe with the recipe, that begins to be kind of hard.
	//
	// After those are stored, we have to deal with the user's password.
	//
	// To adequately handle this with our crypto library, look at this page:
	//
	//     https://doc.libsodium.org/password_hashing
	//
	// To store a User record, you must
	//
	//     - generate random bytes to be used as the salt
	//     - take the salt and the user's plaintext password and derive a key
	//     - store the key
	//
	// You can use the entered password, the key, and the salt to determine if the password is
	// what's in the key, later (?).
	//
	// So, here, we generate the secret, and hash the password to be used as the key for later.
	//
	// However, once we've done that, we need to also add a UserSession record for the user.
	// As of now, we use the OBJECT_FLAG_LOCKED flag on the usersession_t object to denote if the
	// user's session is used.
	//
	// FURTHER NOTES
	//
	// The documentation for libsodium suggests to store all of the parameters for crypto_pwhash
	// together.

	user_record = store_addobj(RT_USER);

	username = store_addobj(RT_STRING128);
	strncpy(username->string, newuser->username, sizeof(username->string));
	user_record->username = username->base.id;

	email = store_addobj(RT_STRING128);
	strncpy(email->string, newuser->email, sizeof(email->string));
	user_record->email = email->base.id;

	// REMOVE ME
	assert(sizeof(salt->string) <= crypto_pwhash_STRBYTES);

	password = store_addobj(RT_STRING128);
	user_record->password = password->base.id;

	salt = store_addobj(RT_STRING128);
	randombytes_buf(salt->string, sizeof(salt->string));
	user_record->salt = salt->base.id;

    printf("need bytes: %d\n", crypto_pwhash_STRBYTES);

    assert(sizeof(password->string) >= crypto_pwhash_STRBYTES);

	rc = crypto_pwhash_str(
        password->string, newuser->password, strlen(newuser->password),
        crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE
    );

	// TODO (Brian): replace with an 'if' that returns some kind of server error
	assert(rc == 0);

	randombytes_buf(tbuf, COOKIE_LEN);

	session_secret = store_addobj(RT_STRING128);
	user_record->session_secret = session_secret->base.id;

	sodium_bin2base64(session_secret->string, sizeof(session_secret->string),
		tbuf, COOKIE_LEN, sodium_base64_VARIANT_URLSAFE_NO_PADDING);

	return user_record->base.id;
}

// user_set_cookie : write the header to set the user cookie in 's'
int user_set_cookie(char *s, user_id id, size_t len)
{
	user_t *user;
	string_128_t *secret;
	string_128_id secret_id;

	user = store_getobj(RT_USER, id);
	if (user == NULL) {
		return 0;
	}

	secret_id = user->session_secret;

	secret = store_getobj(RT_STRING128, secret_id);
	if (secret == NULL) {
		return 0;
	}

	return snprintf(s, len, "Set-Cookie: %s=%s; SameSite=Strict; HttpOnly\r\n", COOKIE_KEY, secret->string);
}

// newuser_from_json : converts a JSON string into a NewUser object
struct NewUser *newuser_from_json(char *s)
{
	struct NewUser *user;
	json_t *root;
	json_error_t error;

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

// login_from_json : parses a login structure from the input json
struct Login *login_from_json(char *s)
{
    struct Login *login;
    json_t *root;
    json_error_t error;

    json_t *username, *password;

    root = json_loads(s, 0, &error);
    if (root == NULL) {
        return NULL;
    }

    if (!json_is_object(root)) {
        return NULL;
    }

    username = json_object_get(root, "username");
    if (!json_is_string(username)) {
        json_decref(root);
        return NULL;
    }

    password = json_object_get(root, "password");
    if (!json_is_string(password)) {
        json_decref(root);
        return NULL;
    }

    login = calloc(1, sizeof(*login));

    login->username = strdup(json_string_value(username));
    login->password = strdup(json_string_value(password));

    json_decref(root);

    return login;
}

// whoami_to_json : converts a WhoAmI structure to a json blob
char *whoami_to_json(struct WhoAmI *who)
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

// newuser_free : frees the new user structure
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

// login_free : frees the login structure
void login_free(struct Login *login)
{
	if (login) {
		free(login->username);
		free(login->password);
		free(login);
	}
}

// whoami_free : frees the strings and children, does not free this structure
void whoami_free(struct WhoAmI *who)
{
	if (who) {
		free(who->username);
		free(who->email);
		free(who);
	}
}

