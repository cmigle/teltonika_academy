#ifndef ARGS_H
#define ARGS_H

// Where parsed command-line values end up.
struct arguments {
    char *device_id;
    char *device_secret;
    char *product_id;
    int daemon_mode;
};

// Parses argv by hand (musl libc on OpenWrt has no argp). Prints usage and
// exits if a required argument is missing.
void parse_arguments(int argc, char **argv, struct arguments *args);

#endif
