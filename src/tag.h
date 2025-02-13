#ifndef TAG_H
#define TAG_H

#include "common.h"
#include "objects.h"

#include "mongoose.h"

// tag_api_getlist : endpoint, GET - /api/v1/tags
int tag_api_getlist(struct mg_connection *conn, struct mg_http_message *hm);

#endif


