#ifndef STORAGE_H
#define STORAGE_H

#include "linklist.h"

// Loads addresses.csv from the user's home directory into `list`, if it
// exists. If it doesn't exist (or HOME isn't set), the address book is left
// empty and a status message is printed - this is expected, not an error.
void load_saved_addresses(struct Person **list);

// Overwrites addresses.csv in the user's home directory with the current
// state of the address book, freeing each node right after it's written.
void save_addresses(struct Person **list);

#endif
