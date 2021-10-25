// Brian Chrzanowski
// 2021-09-08 12:54:30
//
// User Functions

#include "common.h"

#include "httpserver.h"

#include <sodium.h>

#include "user.h"
#include "objects.h"

#define COOKIE_KEY ("session")

// user_api_newuser: endpoint, POST - /api/v1/user/newuser
int user_api_newuser(struct http_request_s *req, struct http_response_s *res)
{
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

// newuserdto_free : frees the new user dto
void newuser_dto_free(struct NewUserDto *dto)
{
    if (dto) {
        free(dto->username);
        free(dto->email);
        free(dto->password);
        free(dto->verify);
        free(dto);
    }
}

// logindto_free : frees the login dto
void login_dto_free(struct LoginDto *dto)
{
    if (dto) {
        free(dto->username);
        free(dto->password);
        free(dto);
    }
}

