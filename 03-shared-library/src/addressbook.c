#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "linklist.h"
#include "storage.h"
#include "menu.h"

static struct Person *address_book = NULL;

static void cleanup(void)
{
    save_addresses(&address_book);
}

static void handle_signal(int sig)
{
    (void)sig;
    printf("\nSignal received - saving address book and exiting...\n");
    exit(0);
}

int main(void)
{
    atexit(cleanup);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    load_saved_addresses(&address_book);
    interactive_menu(&address_book);

    return 0;
}
