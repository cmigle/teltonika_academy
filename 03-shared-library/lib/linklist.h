#ifndef LINKLIST_H
#define LINKLIST_H

typedef enum {
    LIST_OK = 0,
    LIST_ERR_NULL_ARG = -1,
    LIST_ERR_INVALID_POSITION = -2,
    LIST_ERR_OUT_OF_BOUNDS = -3,
    LIST_ERR_ALLOC_FAILED = -4,
    LIST_ERR_EMPTY = -5
} ListStatus;

struct Person {
    char *name;
    char *surname;
    char *email;
    char *number;
    struct Person *next;
};

struct Person* create_node(char *name, char *surname, char *email, char *number);

struct Person* create_address_node(char *address_line);

void free_person(struct Person *person);

int add_to_list(struct Person **list, struct Person *node);

int add_address_by_position(struct Person **list, struct Person *node, int position);

int delete_address_by_position(struct Person **list, int position);

int delete_list(struct Person **list);

struct Person* find_address_by_position(struct Person *list, int position);

int find_address(struct Person *list, const char *search_term, struct Person *results[], int max_results);


#endif