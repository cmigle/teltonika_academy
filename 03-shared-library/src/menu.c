#include "linklist.h"
#include "menu.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_SEARCH_RESULTS 50

// ----------------- DYNAMIC INPUT READING -----------------
static char* read_dynamic_token(void)
{
    int c;

    while ((c = getchar()) != EOF && isspace(c)) 
    {
        // skip leading whitespace (spaces, tabs, leftover newlines)
    }
    if (c == EOF) 
    {
        return NULL;
    }

    size_t capacity = 16;
    size_t length = 0;
    char *buffer = malloc(capacity);
    if (buffer == NULL) {
        return NULL;
    }

    while (c != EOF && !isspace(c)) 
    {
        if (length + 1 >= capacity) 
        {
            capacity *= 2;
            char *bigger = realloc(buffer, capacity);
            if (bigger == NULL) 
            {
                free(buffer);
                return NULL;
            }
            buffer = bigger;
        }
        buffer[length++] = (char) c;
        c = getchar();
    }
    buffer[length] = '\0';
    return buffer;
}

static void display_list(struct Person *list)
{
    if(list == NULL) {
        printf("Address book is empty.\n");
        return;
    }
    
    printf("Address Book:\n");

    int index = 1;

    for (struct Person *temp = list; temp != NULL; temp = temp->next, index++) 
    {
        printf("%d. %s %s %s %s\n", index, temp->name, temp->surname, temp->email, temp->number);
    }

}

static void display_person_at_position(int position, struct Person *person)
{
    if (person == NULL) 
    {
        printf("Invalid position. Position is out of bounds.\n");
        return;
    }
    printf("Address at position %d: %s %s %s %s\n", position, person->name, person->surname, person->email, person->number);
}

static void display_search_results(struct Person *results[], int count)
{
    if (count <= 0) {
        printf("No matching address found.\n");
        return;
    }
    for (int i = 0; i < count; i++) {
        printf("%s %s %s %s\n",
               results[i]->name, results[i]->surname, results[i]->email, results[i]->number);
    }
}

static void report_status(int status)
{
    switch (status) {
        case LIST_OK:
            break;
        case LIST_ERR_NULL_ARG:
            printf("Could not complete the action - missing data.\n");
            break;
        case LIST_ERR_INVALID_POSITION:
            printf("Invalid position. Position must be a positive integer.\n");
            break;
        case LIST_ERR_OUT_OF_BOUNDS:
            printf("Invalid position. Position is out of bounds.\n");
            break;
        case LIST_ERR_ALLOC_FAILED:
            printf("Failed to allocate memory.\n");
            break;
        case LIST_ERR_EMPTY:
            printf("Address book is empty.\n");
            break;
        default:
            printf("Unknown error (code %d).\n", status);
    }
}

// ----------------- INPUT -----------------

static int read_menu_choice(void)
{
    int choice;
    if (scanf("%d", &choice) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return -1;
    }
    return choice;
}

// ----------------- MAIN MENU LOOP -----------------

void interactive_menu(struct Person **list)
{
    int choice;
    int position;

    do {
        printf("\nAddress Book Menu:\n");
        printf("1. Display all records in the address book\n");
        printf("2. Add new address to the book\n");
        printf("3. Add new address indicating the position where to put it\n");
        printf("4. Delete address indicating its position in the book\n");
        printf("5. Delete whole address book\n");
        printf("6. Find address by position\n");
        printf("7. Find address by search term\n");
        printf("8. Exit\n\n");
        printf("Enter your choice: ");

        choice = read_menu_choice();
        if (choice == -1) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        printf("\n");

        switch (choice) {

            case 1: {
                display_list(*list);
                break;
            }

            case 2: {
                printf("Enter name, surname, email, and number (separated by spaces): ");
                // FIX 7: dynamic reads instead of scanf("%29s", ...) - no length cap
                char *name = read_dynamic_token();
                char *surname = read_dynamic_token();
                char *email = read_dynamic_token();
                char *number = read_dynamic_token();

                if (name == NULL || surname == NULL || email == NULL || number == NULL) {
                    printf("Failed to read input.\n");
                    free(name); free(surname); free(email); free(number);
                    break;
                }

                struct Person *new_person = create_node(name, surname, email, number);
                free(name); free(surname); free(email); free(number);

                if (new_person == NULL) {
                    printf("Failed to create record (out of memory).\n");
                    break;
                }
                int status = add_to_list(list, new_person);
                if (status == LIST_OK) {
                    printf("Added %s %s to the address book.\n", new_person->name, new_person->surname);
                } else {
                    report_status(status);
                }
                break;
            }

            case 3: {
                printf("Enter name, surname, email, and number (separated by spaces): ");
                char *name = read_dynamic_token();
                char *surname = read_dynamic_token();
                char *email = read_dynamic_token();
                char *number = read_dynamic_token();

                if (name == NULL || surname == NULL || email == NULL || number == NULL) {
                    printf("Failed to read input.\n");
                    free(name); free(surname); free(email); free(number);
                    break;
                }

                printf("Enter position to add: ");
                scanf("%d", &position);

                struct Person *new_person_pos = create_node(name, surname, email, number);
                free(name); free(surname); free(email); free(number);

                if (new_person_pos == NULL) {
                    printf("Failed to create record (out of memory).\n");
                    break;
                }
                int status = add_address_by_position(list, new_person_pos, position);
                if (status == LIST_OK) {
                    printf("Added record at position %d.\n", position);
                } else {
                    report_status(status);
                }
                break;
            }

            case 4: {
                printf("Enter position to delete: ");
                scanf("%d", &position);
                int status = delete_address_by_position(list, position);
                if (status == LIST_OK) {
                    printf("Deleted address at position %d.\n", position);
                } else {
                    report_status(status);
                }
                break;
            }

            case 5: {
                int status = delete_list(list);
                if (status == LIST_OK) {
                    printf("Address book deleted.\n");
                } else {
                    report_status(status);
                }
                break;
            }

            case 6: {
                printf("Enter position to find: ");
                scanf("%d", &position);
                struct Person *person = find_address_by_position(*list, position);
                display_person_at_position(position, person);
                break;
            }

            case 7: {
                printf("Enter search term (name/surname/email/number is accepted): ");
                // FIX 7: dynamic read instead of scanf("%29s", search_term)
                char *search_term = read_dynamic_token();
                if (search_term == NULL) {
                    printf("Failed to read input.\n");
                    break;
                }
                struct Person *results[MAX_SEARCH_RESULTS];
                int count = find_address(*list, search_term, results, MAX_SEARCH_RESULTS);
                display_search_results(results, count);
                free(search_term);
                break;
            }

            case 8: {
                printf("Exiting...\n");
                break;
            }

            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 8);

}
