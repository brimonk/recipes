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
#include "store.h"

#define COOKIE_KEY ("session")
#define COOKIE_LEN 32

// newuser_verify : returns true if the new user record is valid
int newuser_verify(struct NewUser *newuser);

// set_user_cookie : prints the user cookie on the response
int set_user_cookie(struct http_response_s *res, user_id id);

// newuser_from_json : converts a JSON string into a NewUser object
struct NewUser *newuser_from_json(char *json);

// newuser_add : adds the new user into the user table
int newuser_add(struct NewUser *newuser);

// newuser_free : frees the new user 
void newuser_free(struct NewUser *newuser);

// user_api_newuser: endpoint, POST - /api/v1/newuser
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

    snprintf(tbuf, sizeof tbuf, "{\"id\":%lld}", id);

    http_response_status(res, 200);

    set_user_cookie(res, id);

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

    session_secret = store_addobj(RT_STRING128);
    randombytes_buf(session_secret->string, COOKIE_LEN);
    user_record->session_secret = session_secret->base.id;

    return user_record->base.id;
}

// set_user_cookie : prints the user cookie on the response
int set_user_cookie(struct http_response_s *res, user_id id)
{
    struct user_t *user;
    struct string_128_t *session_secret;
    char gbuf[COOKIE_LEN];
    char pbuf[BUFLARGE];
    char cbuf[BUFLARGE + BUFLARGE];
    size_t len;
    int encoding_variant;

    user = store_getobj(RT_USER, id);
    if (user == NULL) {
        return 0;
    }

    session_secret = store_getobj(RT_STRING128, user->session_secret);
    if (session_secret == NULL) {
        return 0;
    }

    memset(gbuf, 0, sizeof gbuf);
    memset(pbuf, 0, sizeof pbuf);
    memset(cbuf, 0, sizeof cbuf);

    memcpy(gbuf, session_secret->string, COOKIE_LEN);

    randombytes_buf(gbuf, sizeof gbuf);

    encoding_variant = sodium_base64_VARIANT_URLSAFE_NO_PADDING;

    len = sodium_base64_ENCODED_LEN(COOKIE_LEN, encoding_variant);
    printf("%ld bytes needed!\n", len);

    assert(len <= sizeof(pbuf));

    sodium_bin2base64(pbuf, len, (unsigned char *)gbuf, sizeof gbuf, encoding_variant);

    // now we finally get to call the cookie thing
    snprintf(cbuf, sizeof cbuf, "%s=%s; Path=/", COOKIE_KEY, pbuf);

    printf("trying to respond with this: %s\n", cbuf);

    http_response_header(res, "Set-Cookie", cbuf);

    return 1;
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

