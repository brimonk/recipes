// Brian Chrzanowski
//
// Just like the original authors of SQLite, I claim no real copyright to this code. In place of a
// legal notice are a blessing and a prayer.
// 
//     May you do good and not evil.
//     May you find forgiveness for yourself and forgive others.
//     May you share freely, never taking more than you give.
// 
//     Lord Jesus Christ, Son of God, have mercy on me, a sinner.
// 
//  This SQLite extension implements functions that facilitate password hashing, creation, and
//  storage using libsodium. They are named in a 1:1 format with their corresponding libsodium 
//  functions, but because I'm a little silly, I've changed up the names a little bit.
// 
//  The following SQL functions are implemented:
// 
//    - crypto_pwhash_str

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include <sodium.h>

#if !defined(SQLITE_ASCII) && !defined(SQLITE_EBCDIC)
# define SQLITE_ASCII 1
#endif

static void sqlite3_passwd_verify(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    assert(argc == 2);

    switch (sqlite3_value_type(argv[0])) {
        case SQLITE_TEXT: {
            const unsigned char *hash = sqlite3_value_text(argv[0]);
            const unsigned char *passwd = sqlite3_value_text(argv[1]);

            int rc = crypto_pwhash_str_verify(
                (const char *)hash,
                (const char *const)passwd,
                strlen((const char *)passwd)
            );

            sqlite3_result_int(context, rc == 0 ? 1 : 0);
            break;
        }

        default: {
            return;
        }
    }
}

// sqlite3_passwd: allows calling from
static void sqlite3_passwd(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    assert(argc == 1);

    switch (sqlite3_value_type(argv[0])) {
        case SQLITE_TEXT: {
            const unsigned char *passwd = sqlite3_value_text(argv[0]);
            char out[crypto_pwhash_STRBYTES] = { 0 };

            if (crypto_pwhash_str(
                out,
                (const char *const)passwd,
                strlen((const char *)passwd),
                crypto_pwhash_OPSLIMIT_MODERATE,
                crypto_pwhash_MEMLIMIT_MODERATE
            ) == -1) {
                // OOM - probably panic here or something...
                sqlite3_result_error_nomem(context);
                return;
            }

            sqlite3_result_text(context, out, -1, NULL);
            break;
        }

        default: {
            return;
        }
    }
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_passwd_init(sqlite3 *db, char **errmsg, const sqlite3_api_routines *api)
{
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(api);
    (void)errmsg;  /* Unused parameter */

    if (sodium_init() < 0) {
        exit(1); // TODO how do we log some kind of a failure?
    }

    rc = sqlite3_create_function(
        db, "passwd", 1, SQLITE_UTF8|SQLITE_INNOCUOUS, 0, sqlite3_passwd, 0, 0
    );
    if (rc != SQLITE_OK) {
        exit(1);
    }

    rc = sqlite3_create_function(
        db, "passwd_verify", 2, SQLITE_UTF8|SQLITE_INNOCUOUS, 0, sqlite3_passwd_verify, 0, 0
    );
    if (rc != SQLITE_OK) {
        exit(1);
    }

    return rc;
}
