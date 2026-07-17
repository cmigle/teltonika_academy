/* Fixes concerning this file
  * Fixed memory leak in save_addresses() by properly freeing nodes after writing them to the file.
  * Replaced the fixed char line[128] buffer with getline(), so a saved record of any
    length can be read back in full instead of being cut off on reload.
*/

// FIX 8: must be defined before any system header is included, so it has
// to come first - needed to expose getline() from stdio.h
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linklist.h"
#include "storage.h"

#define ADDRESSES_FILENAME "addresses.csv"

// FIX 2: Builds "$HOME/addresses.csv" into `buffer`. 
// Returns 0 on success, -1 if HOME isn't set (so we never pass NULL into snprintf).
static int build_addresses_path(char *buffer, size_t size)
{
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        return -1;
    }
    snprintf(buffer, size, "%s/%s", home_dir, ADDRESSES_FILENAME);
    return 0;
}

// FIX 8: getline() allocates and grows its own buffer as needed - no more
// truncating a long saved line the way the old fixed char line[128] did.
static void load_addresses(FILE *file, struct Person **list)
{
    char *line = NULL;
    size_t line_cap = 0;
    ssize_t line_len;

    while ((line_len = getline(&line, &line_cap, file)) != -1) {
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
        }
        if (line[0] == '\0') {
            // skip blank lines, keep reading the rest of the file
            continue;
        }
        struct Person *person = create_address_node(line);
        if (person != NULL) {
            add_to_list(list, person);
        }
    }

    free(line);
}

void load_saved_addresses(struct Person **list)
{
    char file_path[256];
    if (build_addresses_path(file_path, sizeof(file_path)) != 0) {
        fprintf(stderr, "Could not determine home directory (HOME is not set).\n");
        return;
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        // Not finding the file is expected on first run - continue with an
        // empty address book rather than treating this as an error.
        printf("No address book found at %s. Starting with an empty address book.\n", file_path);
        return;
    }

    load_addresses(file, list);
    fclose(file);
}

void save_addresses(struct Person **list)
{
    if (*list == NULL) {
        printf("Address book is empty. Nothing to save.\n");
        return;
    }

    char file_path[256];
    if (build_addresses_path(file_path, sizeof(file_path)) != 0) {
        fprintf(stderr, "Could not determine home directory (HOME is not set). Address book not saved.\n");
        return;
    }

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        printf("Error opening file for writing: %s\n", file_path);
        return;
    }

    struct Person *temp = *list;
    while (temp != NULL) {
        fprintf(file, "%s,%s,%s,%s\n", temp->name, temp->surname, temp->email, temp->number);
        struct Person *next_node = temp->next;
        // FIX 6: free_person instead of free() - also frees name/surname/email/number
        free_person(temp);
        temp = next_node;
    }
    *list = NULL;

    printf("Address book saved to %s\n", file_path);
    fclose(file);
}
