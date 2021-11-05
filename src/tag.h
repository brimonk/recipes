#ifndef TAG_H
#define TAG_H

// NOTE (Brian): Tags are a little strange within the system.
//
// The whole system was originally designed with tags _just_ being stored as a string. However, Emma
// wants to have a type ahead to add tags into the system. A thing, that I haven't really optimized
// for, is performing data migrations. So, what I think I want to do for the moment is basically
// aggregate a unique list of tags, and we'll store those.
//
// Long term, what we probably want to do is actually migrate all of the string based items to a
// hashtable, such that duplicate strings don't exist within the system. But, at some point, we'll
// have to worry about a data migration, and that'll be a little bit involved, so for the moment,
// I'm hoping that this will work for quite a while.
//
// That being said, storing everything into a hashtable is kind of quite expensive. Mostly because
// when the hashtable needs to be resized, we have to rehash (I think) everything in the hashtable.
//
// 2021-11-05 13:01:19

#include "common.h"
#include "objects.h"

// Tag : it's just some text, that gets normalized to lower case
struct Tag {
	tag_id id;
	char *normalized;
	char *value;
};

// TagResultRecord : one result from a TagSearch
struct TagResultRecord {
    tag_id id;
	char value[128];
	char normalized[128];
};

// TagResultRecords : an aggregation of results from a search
struct TagResultRecords {
	struct TagResultRecord *records;
	size_t records_len, records_cap;
};

// tag_api_getlist : endpoint, GET - /api/v1/tags
int tag_api_getlist(struct http_request_s *req, struct http_response_s *res);

#endif


