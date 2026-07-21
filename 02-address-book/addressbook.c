/* Fixes concerning this file
  * Added cleanup() function to save the address book before exiting
  * Added handle_signal() function to catch Ctrl+C and other termination signals
  * Added atexit(cleanup) to register the cleanup function to be called on exit
  * Added signal(SIGINT, handle_signal) and signal(SIGTERM, handle_signal) to register the signal handler for termination signals
  * Changed the main function to call load_saved_addresses()
  * Moved interactive_menu()
*/

#include <stdio.h>
#include <stdlib.h>

// FIX 1:added so no memory leak occurs when the program is terminated with Ctrl+C
#include <signal.h>

#include "linklist.h"
#include "storage.h"
#include "menu.h"

// FIX 1: kept as a global (not local to main) so cleanup() never references a stack
// frame that no longer exists.
static struct Person *address_book = NULL;

// FIX 1: Cleanup function to save the address book before exiting
static void cleanup(void)
{
    save_addresses(&address_book);
}

// FIX: Signal handler to catch Ctrl+C and other termination signals
static void handle_signal(int sig)
{
    (void)sig;
    printf("\nSignal received - saving address book and exiting...\n");
    exit(0);
}

// This is the program's entry point only - setup, then hand off to the
// other modules. All the actual logic lives in linklist.c, storage.c, and
// menu.c.
int main(void)
{
    atexit(cleanup);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    load_saved_addresses(&address_book);
    interactive_menu(&address_book);

    return 0;
}
