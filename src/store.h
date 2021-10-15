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

// store_read : reads all of the data into memory, from the store
int store_read(void);

// store_write : writes all of the data in memory, onto the disk file
int store_write(void);

// store_free : this function assumes you've written this data to the disk
void store_free(void);

// lump_first_time_setup : does first time setup for a single lump
void lump_first_time_setup(lump_t *base, int type, size_t recsize, size_t start);

#endif // STORE_H

