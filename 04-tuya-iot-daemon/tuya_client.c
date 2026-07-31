#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "tuya_cacert.h"
#include "tuya_log.h"
#include "tuya_error_code.h"
#include "system_interface.h"
#include "mqtt_client_interface.h"
#include "tuyalink_core.h"
#include "tuya_client.h"

// FIX: identifier/parameter name must match EXACTLY what was defined in the
// Tuya IoT Platform's Function Definition for the Action.
#define ACTION_CODE_SAVE_TEXT "save_text"
#define ACTION_PARAM_TEXT     "text"
#define ACTION_LOG_PATH       "/tmp/tuya_action.log"

static tuya_mqtt_context_t client_instance;

// ----------------- Action handling -----------------

// Parses an ACTION_EXECUTE payload (msg->data_string), pulls out the text
// parameter, and appends it to /tmp/tuya_action.log.
static void handle_action_execute(const tuyalink_message_t *msg)
{
    if (msg->data_string == NULL) {
        TY_LOGW("action execute received with no data");
        return;
    }

    cJSON *root = cJSON_Parse(msg->data_string);
    if (root == NULL) {
        TY_LOGE("failed to parse action JSON: %s", msg->data_string);
        return;
    }

    cJSON *action_code = cJSON_GetObjectItem(root, "actionCode");
    cJSON *input_params = cJSON_GetObjectItem(root, "inputParams");

    if (action_code == NULL || input_params == NULL) {
        TY_LOGE("action JSON missing actionCode/inputParams: %s", msg->data_string);
        cJSON_Delete(root);
        return;
    }

    if (strcmp(action_code->valuestring, ACTION_CODE_SAVE_TEXT) != 0) {
        TY_LOGW("unrecognized actionCode: %s", action_code->valuestring);
        cJSON_Delete(root);
        return;
    }

    cJSON *text_param = cJSON_GetObjectItem(input_params, ACTION_PARAM_TEXT);
    if (text_param == NULL || text_param->valuestring == NULL) {
        TY_LOGE("action missing input parameter");
        cJSON_Delete(root);
        return;
    }

    FILE *log_file = fopen(ACTION_LOG_PATH, "a");
    if (log_file != NULL) {
        fprintf(log_file, "%s\n", text_param->valuestring);
        fclose(log_file);
        TY_LOGI("action saved text: %s", text_param->valuestring);
    } else {
        TY_LOGE("failed to open action log file");
    }

    cJSON_Delete(root);
}

// ----------------- MQTT callbacks -----------------

static void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
    (void) context;
    (void) user_data;
    TY_LOGI("Tuya MQTT connected");
}

static void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
    (void) context;
    (void) user_data;
    TY_LOGW("Tuya MQTT disconnected");
}

static void on_messages(tuya_mqtt_context_t *context, void *user_data, const tuyalink_message_t *msg)
{
    (void) context;
    (void) user_data;

    TY_LOGI("received message id=%s type=%d code=%d", msg->msgid, msg->type, msg->code);

    switch (msg->type) {
        case THING_TYPE_ACTION_EXECUTE:
            handle_action_execute(msg);
            break;
        default:
            break;
    }
}

// ----------------- public API -----------------

int tuya_client_start(const char *product_id, const char *device_id, const char *device_secret)
{
    (void) product_id; // not required by tuya_mqtt_config_t directly - kept for API symmetry with args.h

    int ret = tuya_mqtt_init(&client_instance, &(const tuya_mqtt_config_t) {
        .host = "m1.tuyacn.com",
        .port = 8883,
        .cacert = (const uint8_t *) tuya_cacert_pem,
        .cacert_len = sizeof(tuya_cacert_pem),
        .device_id = device_id,
        .device_secret = device_secret,
        .keepalive = 100,
        .timeout_ms = 2000,
        .on_connected = on_connected,
        .on_disconnect = on_disconnect,
        .on_messages = on_messages
    });

    if (ret != OPRT_OK) {
        TY_LOGE("tuya_mqtt_init failed, code=%d", ret);
        return -1;
    }

    ret = tuya_mqtt_connect(&client_instance);
    if (ret != OPRT_OK) {
        TY_LOGE("tuya_mqtt_connect failed, code=%d", ret);
        return -1;
    }

    return 0;
}

void tuya_client_loop(void)
{
    tuya_mqtt_loop(&client_instance);
}

int tuya_client_report_system_info(const struct system_info *info)
{
    cJSON *root = cJSON_CreateObject();
    long long now_ms = (long long) time(NULL) * 1000;

    cJSON *total_ram = cJSON_CreateObject();
    cJSON_AddNumberToObject(total_ram, "value", (double) (info->total_ram_bytes / 1024));
    cJSON_AddNumberToObject(total_ram, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "total_ram", total_ram);

    cJSON *free_ram = cJSON_CreateObject();
    cJSON_AddNumberToObject(free_ram, "value", (double) (info->free_ram_bytes / 1024));
    cJSON_AddNumberToObject(free_ram, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "free_ram", free_ram);

    cJSON *uptime = cJSON_CreateObject();
    cJSON_AddNumberToObject(uptime, "value", (double) info->uptime_seconds);
    cJSON_AddNumberToObject(uptime, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "uptime", uptime);

    cJSON *cpu_usage = cJSON_CreateObject();
    cJSON_AddNumberToObject(cpu_usage, "value", info->cpu_usage_percent);
    cJSON_AddNumberToObject(cpu_usage, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "cpu_usage", cpu_usage);

    cJSON *names = cJSON_CreateArray();
    cJSON *ips = cJSON_CreateArray();
    cJSON *netmasks = cJSON_CreateArray();
    cJSON *tx_bytes = cJSON_CreateArray();
    cJSON *rx_bytes = cJSON_CreateArray();

    for (int i = 0; i < info->interface_count; i++) {
        const struct iface_info *iface = &info->interfaces[i];
        cJSON_AddItemToArray(names, cJSON_CreateString(iface->name));
        cJSON_AddItemToArray(ips, cJSON_CreateString(iface->ip));
        cJSON_AddItemToArray(netmasks, cJSON_CreateString(iface->netmask));
        cJSON_AddItemToArray(tx_bytes, cJSON_CreateNumber((double) iface->tx_bytes));
        cJSON_AddItemToArray(rx_bytes, cJSON_CreateNumber((double) iface->rx_bytes));
    }

    cJSON *if_names = cJSON_CreateObject();
    cJSON_AddItemToObject(if_names, "value", names);
    cJSON_AddNumberToObject(if_names, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "if_names", if_names);

    cJSON *if_ips = cJSON_CreateObject();
    cJSON_AddItemToObject(if_ips, "value", ips);
    cJSON_AddNumberToObject(if_ips, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "if_ips", if_ips);

    cJSON *if_netmasks = cJSON_CreateObject();
    cJSON_AddItemToObject(if_netmasks, "value", netmasks);
    cJSON_AddNumberToObject(if_netmasks, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "if_netmasks", if_netmasks);

    cJSON *if_tx = cJSON_CreateObject();
    cJSON_AddItemToObject(if_tx, "value", tx_bytes);
    cJSON_AddNumberToObject(if_tx, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "if_tx_bytes", if_tx);

    cJSON *if_rx = cJSON_CreateObject();
    cJSON_AddItemToObject(if_rx, "value", rx_bytes);
    cJSON_AddNumberToObject(if_rx, "time", (double) now_ms);
    cJSON_AddItemToObject(root, "if_rx_bytes", if_rx);

    char *json_string = cJSON_PrintUnformatted(root);
    int ret = -1;
    if (json_string != NULL) {
        ret = tuyalink_thing_property_report(&client_instance, NULL, json_string);
        if (ret != OPRT_OK) {
            TY_LOGE("tuyalink_thing_property_report failed, code=%d", ret);
        }
        free(json_string);
    }

    cJSON_Delete(root);
    return ret;
}
