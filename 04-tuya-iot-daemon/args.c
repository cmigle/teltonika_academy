#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "args.h"

void parse_arguments(int argc, char **argv, struct arguments *args)
{
    memset(args, 0, sizeof(*args));
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--device-id") == 0) && i + 1 < argc)
            args->device_id = argv[++i];
        else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--product-id") == 0) && i + 1 < argc)
            args->product_id = argv[++i];
        else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--device-secret") == 0) && i + 1 < argc)
            args->device_secret = argv[++i];
        else if (strcmp(argv[i], "--daemon") == 0)
            args->daemon_mode = 1;
        else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: tuya_daemon -d DEVICE_ID -p PRODUCT_ID -s SECRET [--daemon]\n");
            exit(0);
        }
    }
    if (!args->device_id || !args->product_id || !args->device_secret) {
        fprintf(stderr, "Error: --device-id, --product-id and --device-secret are required\n");
        exit(1);
    }
}