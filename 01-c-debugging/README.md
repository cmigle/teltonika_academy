# C debugging exercises

Eleven small C programs that were handed out with deliberate bugs. The task was to find each bug, explain why it happens and fix it. Each fix is commented in the source next to the line it changes.

Topics: pointers, stack and heap memory, buffer sizing, who owns and frees an allocation, file I/O error handling.

## Bugs found and fixed

| File | Bug | Fix |
|------|-----|-----|
| [`01_exit_status.c`](01_exit_status.c) | Returned `1` after running correctly, so the shell treated it as a failure | Return `0` |
| [`02_buffer_overflow.c`](02_buffer_overflow.c) | `strcpy` wrote a 38-byte string into a 20-byte stack buffer | Buffer sized for the string and its null terminator |
| [`03_uninitialized_pointer.c`](03_uninitialized_pointer.c) | `strcpy` into a pointer that never had memory behind it | Allocate with `malloc`, check for `NULL`, `free` when done |
| [`04_double_free.c`](04_double_free.c) | Buffer freed inside `text_printer()` and again in `main()`; missing `<string.h>` and function prototype | Free only in the function that allocated it |
| [`05_off_by_one.c`](05_off_by_one.c) | One-byte buffer for `sprintf("%c")`, which also writes a null byte; `strcat` onto an uninitialised buffer; a leak on every loop iteration | Two-byte buffer, start from an empty string, `free` every iteration |
| [`06_null_pointer_write.c`](06_null_pointer_write.c) | String copied into memory that was never allocated | `mem_alloc()` returns a heap buffer for 43 characters plus the terminator, checked for `NULL` |
| [`07_pointer_dereference.c`](07_pointer_dereference.c) | Printed the pointer instead of the value it points to | Dereference before printing |
| [`08_pass_by_pointer.c`](08_pass_by_pointer.c) | The function changed a copy, so `main()` never saw the new value | Pass `&var` and write through the pointer |
| [`09_file_copy.c`](09_file_copy.c) | Output file reopened (and truncated) for every line; `fopen` results unchecked | Open both files once, check both, close after the loop |
| [`10_csv_linked_list.c`](10_csv_linked_list.c) | Wrong pointer type passed to `delete_list()`; `strtok` expected a comma after the last field; malformed lines not checked; a blank line skipped cleanup | Correct types, read the last field up to `\n`, skip incomplete lines, `break` so the list is still freed |
| [`11_bounded_input.c`](11_bounded_input.c) | User input read with no size limit | Fixed 256-byte buffer read with a width limit (`%255[^\n]`) |

## Build and run

```sh
make                    # builds every example with -Wall -Wextra
./08_pass_by_pointer
./09_file_copy          # copies original_file.txt to copied_file.txt
./10_csv_linked_list    # loads addresses.csv into a linked list and prints it
make clean
```

All eleven programs compile without warnings under `-Wall -Wextra`.
