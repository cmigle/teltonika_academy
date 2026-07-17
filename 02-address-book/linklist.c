/* Fixes concerning this file
  * Fixed long inputs overflowing the fixed-size fields in the Person struct by switching
    from fixed char[30] arrays + strncpy to char* fields + exact-size heap allocation.
  * Find address by position now correctly returns the head of the list when position is 1,
    instead of returning NULL.
  * Linklist operations now handle edge cases properly - with status codes.
  * Added free_person() so deleting a node also frees its 4 string fields, not just the
    struct itself, now that they're separate allocations.
*/

#include <stdlib.h>
#include <string.h>
#include "linklist.h"

#define DELIMETER ","

// FIX 6: Allocates exactly strlen(src) + 1 bytes and copies src into it.
// This is what makes fields truly unbounded - the allocation always
// matches the actual input length, nothing is ever truncated. Replaces
// the old fixed-size safe_copy/strncpy approach.
static char* dup_string(const char *src)
{
    size_t len = strlen(src) + 1;
    char *copy = malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, src, len);
    return copy;
}

// FIX 6: frees a Person's 4 string allocations before freeing the struct.
void free_person(struct Person *person)
{
    if (person == NULL) {
        return;
    }
    free(person->name);
    free(person->surname);
    free(person->email);
    free(person->number);
    free(person);
}

struct Person* create_node(char *name, char *surname, char *email, char *number)
{
    struct Person *person = malloc(sizeof(struct Person));
    if (person == NULL) {
        return NULL;
    }

    person->name = dup_string(name);
    person->surname = dup_string(surname);
    person->email = dup_string(email);
    person->number = dup_string(number);
    person->next = NULL;

    // FIX 6: if any single dup_string failed (out of memory), unwind
    // everything allocated so far instead of leaving a half-built node.
    if (person->name == NULL || person->surname == NULL ||
        person->email == NULL || person->number == NULL) {
        free_person(person);
        return NULL;
    }

    return person;
}

struct Person* create_address_node(char *address_line)
{
    char *name = strtok(address_line, DELIMETER);
    char *surname = strtok(NULL, DELIMETER);
    char *email = strtok(NULL, DELIMETER);
    char *number = strtok(NULL, ",\n");

    if (name == NULL || surname == NULL || email == NULL || number == NULL) {
        return NULL;
    }

    return create_node(name, surname, email, number);
}

int add_to_list(struct Person **list, struct Person *person)
{
    if (person == NULL) {
        return LIST_ERR_NULL_ARG;
    }

    struct Person *temp = *list;
    if (temp == NULL) {
        *list = person;
        return LIST_OK;
    }

    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = person;
    return LIST_OK;
}

int add_address_by_position(struct Person **list, struct Person *node, int position)
{
    if (node == NULL) {
        return LIST_ERR_NULL_ARG;
    }
    if (position < 1) {
        return LIST_ERR_INVALID_POSITION;
    }

    if (position == 1) {
        node->next = *list;
        *list = node;
        return LIST_OK;
    }

    struct Person *temp = *list;
    for (int i = 0; i < position - 1 && temp != NULL; i++) {
        temp = temp->next;
    }

    if (temp == NULL) {
        return LIST_ERR_OUT_OF_BOUNDS;
    }

    node->next = temp->next;
    temp->next = node;
    return LIST_OK;
}

int delete_address_by_position(struct Person **list, int position)
{
    if (position < 1) {
        return LIST_ERR_INVALID_POSITION;
    }
    if (*list == NULL) {
        return LIST_ERR_EMPTY;
    }

    struct Person *temp = *list;

    if (position == 1) {
        *list = temp->next;
        // FIX 6: free_person instead of free() - also frees name/surname/email/number
        free_person(temp);
        return LIST_OK;
    }

    for (int i = 0; temp != NULL && i < position - 1; i++) {
        temp = temp->next;
    }

    if (temp == NULL || temp->next == NULL) {
        return LIST_ERR_OUT_OF_BOUNDS;
    }

    struct Person *to_delete = temp->next;
    temp->next = to_delete->next;
    free_person(to_delete);
    return LIST_OK;
}

int delete_list(struct Person **list)
{
    if (*list == NULL) {
        return LIST_ERR_EMPTY;
    }

    struct Person *to_delete = *list;
    while (*list != NULL) {
        *list = (*list)->next;
        free_person(to_delete);
        to_delete = *list;
    }
    return LIST_OK;
}

// ADDITIONAL FIX: loop now advances position - 1 times, not position times, so position 1 correctly returns the head.
struct Person* find_address_by_position(struct Person *list, int position)
{
    if (position < 1) {
        return NULL;
    }

    struct Person *temp = list;
    for (int i = 0; temp != NULL && i < position - 1; i++) {
        temp = temp->next;
    }

    return temp; // NULL if out of bounds
}

int find_address(struct Person *list, const char *search_term, struct Person *results[], int max_results)
{
    if (search_term == NULL || results == NULL || max_results <= 0) {
        return LIST_ERR_NULL_ARG;
    }

    int count = 0;
    struct Person *temp = list;
    while (temp != NULL && count < max_results) {
        if (strcmp(temp->name, search_term) == 0 ||
            strcmp(temp->surname, search_term) == 0 ||
            strcmp(temp->email, search_term) == 0 ||
            strcmp(temp->number, search_term) == 0) {
            results[count] = temp;
            count++;
        }
        temp = temp->next;
    }

    return count;
}
