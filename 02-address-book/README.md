# Address book (linked list)

A terminal address book in C. Records (name, surname, email, phone number) are kept in a singly linked list and saved to `~/addresses.csv` when the program exits, including on Ctrl+C or `SIGTERM`.

It started as a single file and was split into modules after code review. The second commit adds the Makefile.

## Features

- List all records
- Add a record at the end or at a given position
- Delete a record by position, or delete the whole book
- Find a record by position
- Search by name, surname, email or number (exact match)

## Structure

| File | Responsibility |
|------|----------------|
| [`addressbook.c`](addressbook.c) | Entry point. Loads saved records, starts the menu, saves on exit through `atexit()` and a `SIGINT`/`SIGTERM` handler |
| [`linklist.c`](linklist.c) / [`.h`](linklist.h) | List operations. They return `ListStatus` codes and do no I/O |
| [`menu.c`](menu.c) / [`.h`](menu.h) | The interactive menu. Reads input and turns status codes into messages for the user |
| [`storage.c`](storage.c) / [`.h`](storage.h) | Loads and saves `$HOME/addresses.csv` |

## Design decisions

- **Library code never prints.** Every list function returns a status code (`LIST_OK`, `LIST_ERR_OUT_OF_BOUNDS`, ...) and only the menu module talks to the user.
- **No fixed-length fields.** Strings are allocated to their exact length, so input isn't cut off at 29 characters as it was with the original `char[30]` fields. Menu input is read into a buffer that doubles as needed, and saved files are read with `getline()`.
- **One owner per allocation.** `free_person()` releases a node and all four of its strings, and the list is freed after it's written to disk.

## Build and run

```sh
make          # builds ./addressbook with -Wall -Wextra
./addressbook
make debug    # same, with -g
make clean
```

`CC` isn't hard-coded, so `make CC=<cross-compiler>` builds for another target.
