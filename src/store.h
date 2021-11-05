#ifndef STORE_H
#define STORE_H

#include "objects.h"

// //////////////////////////////////////////////////////////////
// STORE FUNCTIONS
// //////////////////////////////////////////////////////////////

// store_open : initializes the backing store
int store_open(char *fname);

// store_initialize : sets up the backing store ((re)sizes the file, and 
int store_initialize(void);

// store_getlen : returns the length of the table describe in 'type'
s64 store_getlen(int type);

// store_getversion : returns the version
u64 store_getversion();

// store_setversion : sets the store version
void store_setversion(u64 version);

// store_getobj : gets the object with id 'id' and type 'type' from the store
void *store_getobj(int type, u64 id);

// store_addobj : adds an object to the backing store
void *store_addobj(int type);

// store_freeobj : marks the object in the list as free
void store_freeobj(int type, u64 id);

// store_read : reads all of the data into memory, from the store
int store_read(void);

// store_write : writes all of the data in memory, onto the disk file
int store_write(void);

// store_free : this function assumes you've written this data to the disk
void store_free(void);

// lump_first_time_setup : does first time setup for a single lump
void lump_first_time_setup(lump_t *base, int type, size_t recsize, size_t start);

#endif // STORE_H

