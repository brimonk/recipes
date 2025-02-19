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

#include <sodium.h>
#include <jansson.h>

#include "user.h"
#include "objects.h"

#define COOKIE_KEY ("session")
#define COOKIE_LEN (32)

// user_from_session : takes the cookie, scans the user session table, returns user_id
struct user_t *user_from_session(char *cookie);

// whoami_free : frees the strings and children, does not free this structure
void whoami_free(WhoAmI *who);

// user_set_cookie : write the header to set the user cookie in 's'
int user_set_cookie(char *s, char *id, size_t len);

// user_to_whoami : converts a user structure to a whoami structure
WhoAmI *user_to_whoami(struct user_t *user);

// whoami_to_json : converts a WhoAmI structure to a json blob
char *whoami_to_json(WhoAmI *who);

// user_api_login: endpoint, POST - /api/v1/user/login
int user_api_login(struct mg_connection *conn, struct mg_http_message *hm)
{
	return 0;
}

// user_api_logout: endpoint, POST - /api/v1/user/logout
int user_api_logout(struct mg_connection *conn, struct mg_http_message *hm)
{
	return 0;
}

// user_api_whoami: endpoint, /api/v1/user/whoami
int user_api_whoami(struct mg_connection *conn, struct mg_http_message *hm)
{
	WhoAmI *who;
	struct user_t *user;
	struct mg_str *mg_cookie;
	struct mg_str token;
	char *cookie;
	char *json;

	mg_cookie = mg_http_get_header(hm, "Cookie");
	if (mg_cookie == NULL) {
		return -1; // cookie must be present
	}

	token = mg_http_get_header_var(*mg_cookie, mg_str("session"));
	cookie = strndup(token.ptr, token.len);

	user = user_from_session(cookie);
	if (user == NULL) {
		free(cookie);
		return -1;
	}

	who = user_to_whoami(user);
	if (user == NULL) {
		free(cookie);
		return -1;
	}

	json = whoami_to_json(who);

	mg_http_reply(conn, 200, NULL, json);

	whoami_free(who);
	free(cookie);
	free(json);

	return 0;
}

// user_to_whoami : converts a user structure to a whoami structure
WhoAmI *user_to_whoami(struct user_t *user)
{
	// NOT IMPLEMENTED (Brian)
	return NULL;
}

// user_from_session : takes the cookie, scans the user session table, returns user_id
struct user_t *user_from_session(char *cookie)
{
	// NOT IMPLEMENTED (Brian)
	return NULL;
}

// newuser_add : adds the new user into the user table
int newuser_add(void)
{
	// NOT IMPLEMENTED
	return -1;
#if 0
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

	rc = crypto_pwhash(
		(unsigned char *)password->string, sizeof(password->string),
		newuser->password, strlen(newuser->password),
		(unsigned char *)salt->string,
		crypto_pwhash_OPSLIMIT_INTERACTIVE,
		crypto_pwhash_MEMLIMIT_INTERACTIVE,
		crypto_pwhash_ALG_DEFAULT
		);

	// TODO (Brian): replace with an 'if' that returns some kind of server error
	assert(rc == 0);

	randombytes_buf(tbuf, COOKIE_LEN);

	session_secret = store_addobj(RT_STRING128);
	user_record->session_secret = session_secret->base.id;

	sodium_bin2base64(session_secret->string, sizeof(session_secret->string),
		tbuf, COOKIE_LEN, sodium_base64_VARIANT_URLSAFE_NO_PADDING);

	return user_record->base.id;
#endif
}

// user_set_cookie : write the header to set the user cookie in 's'
int user_set_cookie(char *s, char *id, size_t len)
{
	// NOT IMPLEMENTED (Brian)
	return -1;
#if 0
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
#endif
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

// login_free : frees the login 
void login_free(Login *login)
{
	if (login) {
		free(login->username);
		free(login->password);
		free(login);
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
