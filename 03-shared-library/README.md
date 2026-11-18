# Linked list as a shared library

The address book from [02-address-book](../02-address-book), with the linked list code moved into a shared library, `liblinklist.so`. The program links against it dynamically.

## Layout

```
03-shared-library/
├── Makefile          # builds lib/ then src/, plus install and uninstall
├── lib/
│   ├── Makefile      # -fPIC objects, linked with -shared into liblinklist.so
│   ├── linklist.c
│   └── linklist.h    # public API
└── src/
    ├── Makefile      # links addressbook with -llinklist
    ├── addressbook.c
    ├── menu.c / menu.h
    └── storage.c / storage.h
```

The top-level Makefile calls `make` in `lib/` and `src/`. Because `src` depends on `lib`, the library is always built first.

## Build and run

```sh
make
./src/addressbook
```

`src/addressbook` is linked with `-Wl,-rpath,'$ORIGIN/../lib'`, so it finds the library in `../lib` relative to itself. It runs from the build tree without setting `LD_LIBRARY_PATH`, and the same path still works after installing to `$PREFIX/bin` and `$PREFIX/lib`.

```sh
sudo make install     # library, header and binary under /usr/local, then ldconfig
sudo make uninstall
make clean
```

`DESTDIR` is supported for staged installs and packaging.

## Library API

Declared in [`lib/linklist.h`](lib/linklist.h). Every function that can fail returns a `ListStatus` code, and the library does no I/O.

```c
struct Person *create_node(char *name, char *surname, char *email, char *number);
struct Person *create_address_node(char *address_line);   /* "name,surname,email,number" */
void free_person(struct Person *person);

int add_to_list(struct Person **list, struct Person *node);
int add_address_by_position(struct Person **list, struct Person *node, int position);
int delete_address_by_position(struct Person **list, int position);
int delete_list(struct Person **list);

struct Person *find_address_by_position(struct Person *list, int position);
int find_address(struct Person *list, const char *search_term,
                 struct Person *results[], int max_results);
```

## Static vs. shared

A static library (`.a`) is copied into the executable at link time. A shared library (`.so`) stays a separate file, and the dynamic linker loads it when the program starts, so several programs can share one copy and the library can be updated without relinking them. The cost is that the `.so` has to be present and findable at run time, which is why this build sets an rpath and the install target runs `ldconfig`.
