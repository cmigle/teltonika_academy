#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include "args.h"

const char *argp_program_version = "tuya_daemon 1.0";
const char *argp_program_bug_address = "<migle@example.com>";
static char doc[] = "Tuya IoT cloud monitoring daemon.";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"device-id",     'i', "DEVICE_ID",     0, "Tuya Device ID (required)", 0},
    {"device-secret", 's', "DEVICE_SECRET", 0, "Tuya Device Secret (required)", 0},
    {"product-id",    'p', "PRODUCT_ID",    0, "Tuya Product ID (required)", 0},
    {"daemon",        'd', 0,               0, "Run as a background daemon (optional)", 0},
    {0, 0, 0, 0, 0, 0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key) {
        case 'i':
            arguments->device_id = arg;
            break;
        case 's':
            arguments->device_secret = arg;
            break;
        case 'p':
            arguments->product_id = arg;
            break;
        case 'd':
            arguments->daemon_mode = 1;
            break;
        case ARGP_KEY_END:
            if (arguments->device_id == NULL) {
                argp_error(state, "missing required --device-id");
            }
            if (arguments->device_secret == NULL) {
                argp_error(state, "missing required --device-secret");
            }
            if (arguments->product_id == NULL) {
                argp_error(state, "missing required --product-id");
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

void parse_arguments(int argc, char **argv, struct arguments *args)
{
    args->device_id = NULL;
    args->device_secret = NULL;
    args->product_id = NULL;
    args->daemon_mode = 0;

    argp_parse(&argp, argc, argv, 0, 0, args);
}
