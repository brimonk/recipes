// Brian Chrzanowski
// 2021-10-14 22:35:28
//
// All of our storage logic lives here.

#include "common.h"
#include "objects.h"
#include "store.h"

#include <unistd.h>
#include <sys/types.h>

// global handle, might have to deal with syncing if we end up in multi-threaded land
handle_t handle;

// store_open : initializes the backing store
int store_open(char *fname)
{
    int rc;

    memset(&handle, 0, sizeof handle);

    handle.fname = fname;

    if (access(handle.fname, F_OK) == 0) {
        rc = store_read();
        if (rc < 0) {
            ERR("couldn't load from file!\n");
            exit(1);
        }
    } else {
        rc = store_initialize();
        if (rc < 0) {
            ERR("couldn't setup file header!\n");
            exit(1);
        }
        rc = store_read();
        if (rc < 0) {
            ERR("couldn't load from file!\n");
            exit(1);
        }
    }

    return 0;
}

// store_getobj : gets the object with id 'id' and type 'type' from the store
void *store_getobj(int type, u64 id)
{
    lump_t *lump;
    objectbase_t *base;

    // NOTE (Brian): the fact that this assertion exists is probably what's going to make
    // record accesses pretty slow. So, in some instances, it might make sense for this to be
    // just the return function.

    assert(0 <= type && type < RT_TOTAL);

    lump = &handle.header.lumps[type];

    if (lump->used < id) {
        return NULL;
    }

    base = (void *)(((unsigned char *)handle.ptrs[type]) + lump->recsize * id);

    if (base->flags ^ OBJECT_FLAG_USED) {
        return NULL;
    }

    return (void *)base;
}

// store_addobj : adds an object to the backing store
void *store_addobj(int type)
{
    lump_t *lump;
    objectbase_t *base;
    size_t size, used;
    size_t i;

    // TODO (Brian) : this probably needs the be audited for all of the cases.

    assert(0 <= type && type < RT_TOTAL);

    lump = &handle.header.lumps[type];
    base = NULL;

    // first, we scan the table for unused objects we can zero out and reuse
    for (i = 0; i < lump->used; i++) {
        base = (void *)(((unsigned char *)handle.ptrs[type]) + lump->recsize * i);
        if (base->flags ^ OBJECT_FLAG_USED) {
            base->id = i;
            base->flags |= OBJECT_FLAG_USED;
            return (void *)base;
        }
        base = NULL;
    }

    if (lump->used >= lump->allocated) { // reallocation logic
        if (lump->allocated < BUFLARGE) {
            lump->allocated *= 2;
        } else {
            lump->allocated += BUFLARGE;
        }

        size = lump->allocated * lump->recsize;
        used = lump->used * lump->recsize;

        handle.ptrs[type] = realloc(handle.ptrs[type], size);

        memset(((u8 *)handle.ptrs[type]) + used, 0, size - used);
    }

    base = (void *)(((unsigned char *)handle.ptrs[type]) + lump->recsize * lump->used);

    base->id = lump->used++;
    base->flags |= OBJECT_FLAG_USED;

    return base;
}

// store_freeobj : marks the object in the list as free
void store_freeobj(int type, u64 id)
{
    lump_t *lump;
    objectbase_t *base;

	// NOTE (Brian): at some point, we certainly want to mark these slots as free. Right now, we
	// just mark them as unused. That honestly might be the source of the Pesto bug, I'm not quite
	// sure.

    assert(0 <= type && type < RT_TOTAL);

    lump = &handle.header.lumps[type];

    base = (void *)(((unsigned char *)handle.ptrs[type]) + lump->recsize * id);

    base->flags &= ~(OBJECT_FLAG_USED);
}

// store_getlen : returns the length of the table describe in 'type'
s64 store_getlen(int type)
{
    lump_t *lump;

    assert(0 <= type && type < RT_TOTAL);

    lump = &handle.header.lumps[type];

    return lump->used;
}

// store_initialize : sets up the backing store ((re)sizes the file, and 
int store_initialize(void)
{
    storeheader_t header;
    lump_t *lump;
    size_t i, size;

    // NOTE (Brian): this actually sets up the file for reading / writing
    // After this runs, we perform the regular "store_read" operation to read
    // from the backing store.

    strncpy(header.magic, OBJECT_MAGIC, sizeof(header.magic));
    header.version = OBJECT_VERSION;

    size = 0x1000;

    size += DEFAULT_RECORD_CNT * sizeof(user_t);
    size += DEFAULT_RECORD_CNT * sizeof(recipe_t);
    size += DEFAULT_RECORD_CNT * sizeof(string_128_t);
    size += DEFAULT_RECORD_CNT * sizeof(string_256_t);

    header.size = size;

    lump = header.lumps + RT_USER; // assumed that RT_USER is the first

    lump_first_time_setup(lump, RT_USER,       sizeof(user_t), sizeof(header));
    lump_first_time_setup(lump, RT_RECIPE,     sizeof(recipe_t), 0);
    lump_first_time_setup(lump, RT_STEP,       sizeof(step_t), 0);
    lump_first_time_setup(lump, RT_INGREDIENT, sizeof(ingredient_t), 0);
    lump_first_time_setup(lump, RT_TAG,        sizeof(tag_t), 0);
    lump_first_time_setup(lump, RT_STRING128,  sizeof(string_128_t), 0);
    lump_first_time_setup(lump, RT_STRING256,  sizeof(string_256_t), 0);

    for (i = 0, size = sizeof(header); i < RT_TOTAL; i++) {
        size += header.lumps[i].recsize * header.lumps[i].allocated;
    }

    header.size = size;

    memcpy(&handle.header, &header, sizeof(handle.header));

    FILE *fp;

    truncate(handle.fname, size);

    fp = fopen(handle.fname, "wb");
    if (fp == NULL) {
        ERR("couldn't write to file!\n");
        return -1;
    }

    fwrite(&header, sizeof(header), 1, fp);

    fclose(fp);

    return 0;
}

// store_read : reads all of the data into memory, from the store
int store_read(void)
{
    storeheader_t *header;
    FILE *fp;
    size_t i;

    fp = fopen(handle.fname, "rb");
    if (fp == NULL) {
        ERR("couldn't write to file!\n");
        return -1;
    }

    header = &handle.header;

    fread(&handle.header, sizeof(handle.header), 1, fp);

    for (i = 0; i < RT_TOTAL; i++) {
        handle.ptrs[i] = calloc(header->lumps[i].allocated, header->lumps[i].recsize);
        fread(handle.ptrs[i], header->lumps[i].recsize, header->lumps[i].allocated, fp);
    }

    fclose(fp);

    return 0;
}

// store_write : writes all of the data in memory, onto the disk file
int store_write(void)
{
    storeheader_t *header;
    FILE *fp;
    size_t i;

    fp = fopen(handle.fname, "wb");
    if (fp == NULL) {
        ERR("couldn't write to file!\n");
        return -1;
    }

    header = &handle.header;

    fwrite(header, sizeof(*header), 1, fp);

    // iterate over the lumps, writing out every one
    for (i = 0; i < RT_TOTAL; i++) {
        fwrite(handle.ptrs[i], header->lumps[i].recsize, header->lumps[i].allocated, fp);
    }

    fclose(fp);

    return 0;
}

// lump_first_time_setup : does first time setup for a single lump
void lump_first_time_setup(lump_t *base, int type, size_t recsize, size_t start)
{
    lump_t *lump;

    lump = base + type;

    lump->type = type;
    lump->recsize = recsize;
    lump->allocated = DEFAULT_RECORD_CNT;
    lump->used = 0;
    lump->start = start > 0 ? start :
        ((lump - 1)->start + (lump - 1)->recsize * (lump - 1)->allocated);
}

// store_free : this function assumes you've written this data to the disk
void store_free(void)
{
    size_t i;

    for (i = 0; i < RT_TOTAL; i++) {
        free(handle.ptrs[i]);
    }
}

