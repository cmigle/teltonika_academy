/* Fixes concerning this file
  * Linklist operations now handle edge cases properly - with status codes.
  * Person struct fields changed from fixed char[30] arrays to char* so
    names/surnames/emails/numbers of any length can be stored, not just up
    to 29 characters.
*/

#ifndef LINKLIST_H
#define LINKLIST_H

// FIX 5: Status codes returned by linked list operations.
typedef enum {
    LIST_OK = 0,
    LIST_ERR_NULL_ARG = -1,
    LIST_ERR_INVALID_POSITION = -2,
    LIST_ERR_OUT_OF_BOUNDS = -3,
    LIST_ERR_ALLOC_FAILED = -4,
    LIST_ERR_EMPTY = -5
} ListStatus;

// Address book saves this information - name, surname, email, phone number
// FIX 6: fields are now char* (heap-allocated to the exact input length)
// instead of fixed char[30] arrays, so there is no truncation limit.
struct Person {
    char *name;
    char *surname;
    char *email;
    char *number;
    struct Person *next;
};

// create a new node for the address book. Copies each string into its own
// exactly-sized allocation. Returns NULL if malloc fails.
struct Person* create_node(char *name, char *surname, char *email, char *number);

// create a node from a raw "name,surname,email,number" CSV line.
// Returns NULL if the line is malformed or malloc fails.
struct Person* create_address_node(char *address_line);

// FIX 6: frees a single node, including its dynamically allocated string
// fields. Needed now that Person owns 4 separate allocations, not just
// itself. Safe to call with NULL.
void free_person(struct Person *person);

// add new address to the book (to the end). Returns a ListStatus code.
int add_to_list(struct Person **list, struct Person *node);

// add new address indicating position where to put it (1-indexed).
// Returns a ListStatus code.
int add_address_by_position(struct Person **list, struct Person *node, int position);

// delete address indicating its position in the book (1-indexed).
// Returns a ListStatus code.
int delete_address_by_position(struct Person **list, int position);

// delete whole address book. Returns LIST_ERR_EMPTY if it was already
// empty, otherwise LIST_OK.
int delete_list(struct Person **list);

// find address by position (1-indexed). Returns a pointer to the matching
// node, or NULL if the position is invalid/out of bounds.
struct Person* find_address_by_position(struct Person *list, int position);

// find addresses by name, surname, email, or phone number (strict/full
// match). Fills `results` (capacity `max_results`) with pointers to
// matches and returns how many were found (0 if none), or a negative
// ListStatus code on bad arguments.
int find_address(struct Person *list, const char *search_term, struct Person *results[], int max_results);

#endif
