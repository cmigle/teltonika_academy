#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <libubus.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>

#include "cJSON.h"
#include "tuya_cacert.h"
#include "tuya_error_code.h"
#include "system_interface.h"
#include "mqtt_client_interface.h"
#include "tuyalink_core.h"
#include "tuya_client.h"

#define ACTION_CODE_LIST_ESP_DEVICES "list_esp_devices"
#define ACTION_CODE_SET_ESP_PIN      "set_esp_pin"
#define ACTION_CODE_GET_ESP_SENSOR   "get_esp_sensor"

static tuya_mqtt_context_t client_instance;
static struct ubus_context *ubus_context;

struct ubus_reply {
    char *json;
};

static void ubus_reply_cb(struct ubus_request *request, int type,
                          struct blob_attr *msg)
{
    struct ubus_reply *reply = request->priv;

    (void) type;
    free(reply->json);
    reply->json = msg == NULL ? strdup("{}") : blobmsg_format_json(msg, true);
}
static int call_esp_ubus(const char *method, const char *arguments,
                         char **reply_json)
{
    uint32_t object_id;
    struct blob_buf request_buffer = { 0 };
    struct ubus_reply reply = { 0 };
    int result;

    if (ubus_context == NULL) {
        syslog(LOG_ERR, "call_esp_ubus: no ubus connection");
        return -1;
    }

    result = ubus_lookup_id(ubus_context, "esp", &object_id);
    if (result != 0) {
        syslog(LOG_ERR, "call_esp_ubus: ubus_lookup_id(\"esp\") failed: %s",
               ubus_strerror(result));
        return result;
    }

    blob_buf_init(&request_buffer, 0);
    if (arguments != NULL && arguments[0] != '\0' &&
        !blobmsg_add_json_from_string(&request_buffer, arguments)) {
        syslog(LOG_ERR, "call_esp_ubus: invalid argument JSON: %s", arguments);
        blob_buf_free(&request_buffer);
        return -1;
    }

    syslog(LOG_INFO, "call_esp_ubus: invoking esp '%s' args=%s",
           method, arguments != NULL ? arguments : "{}");
    result = ubus_invoke(ubus_context, object_id, method,
                         request_buffer.head, ubus_reply_cb, &reply, 5000);
    blob_buf_free(&request_buffer);
    if (result != 0) {
        syslog(LOG_ERR, "call_esp_ubus: ubus_invoke(esp,%s) failed: %s",
               method, ubus_strerror(result));
        free(reply.json);
        return result;
    }

    *reply_json = reply.json != NULL ? reply.json : strdup("{}");
    return *reply_json == NULL ? -1 : 0;
}

static int action_is_supported(const char *action_code)
{
    return action_code != NULL &&
           (strcmp(action_code, ACTION_CODE_LIST_ESP_DEVICES) == 0 ||
            strcmp(action_code, ACTION_CODE_SET_ESP_PIN) == 0 ||
            strcmp(action_code, ACTION_CODE_GET_ESP_SENSOR) == 0);
}

static int json_string_param(const cJSON *params, const char *name,
                             const char **value)
{
    cJSON *item = cJSON_GetObjectItem(params, name);
    if (!cJSON_IsString(item) || item->valuestring == NULL)
        return -1;
    *value = item->valuestring;
    return 0;
}

static int json_pin_param(const cJSON *params, int *pin)
{
    cJSON *item = cJSON_GetObjectItem(params, "pin");
    if (!cJSON_IsNumber(item))
        return -1;
    *pin = item->valueint;
    return 0;
}

static int handle_action_list_esp_devices(const cJSON *params, cJSON **result)
{
    char *reply = NULL;

    (void) params;
    if (call_esp_ubus("devices", "{}", &reply) != 0)
        return -1;
    *result = cJSON_Parse(reply);
    free(reply);
    return *result == NULL ? -1 : 0;
}

static int handle_action_set_esp_pin(const cJSON *params, cJSON **result)
{
    const char *port;
    cJSON *state_item = cJSON_GetObjectItem(params, "state");
    int pin;
    char arguments[256];
    char *reply = NULL;
    const char *method;

    if (json_string_param(params, "port", &port) != 0 ||
        json_pin_param(params, &pin) != 0 || !cJSON_IsBool(state_item))
        return -2;

    method = cJSON_IsTrue(state_item) ? "on" : "off";
    snprintf(arguments, sizeof(arguments),
             "{\"port\":\"%s\",\"pin\":%d}", port, pin);
    if (call_esp_ubus(method, arguments, &reply) != 0)
        return -1;
    *result = cJSON_Parse(reply);
    free(reply);
    return *result == NULL ? -1 : 0;
}

static int handle_action_get_esp_sensor(const cJSON *params, cJSON **result)
{
    const char *port;
    const char *model;
    const char *sensor;
    int pin;
    char arguments[384];
    char *reply = NULL;

    if (json_string_param(params, "port", &port) != 0 ||
        json_pin_param(params, &pin) != 0 ||
        json_string_param(params, "model", &model) != 0 ||
        json_string_param(params, "sensor", &sensor) != 0) {
        char *dump = cJSON_PrintUnformatted(params);
        syslog(LOG_ERR, "get_esp_sensor: missing/invalid inputParams "
               "(need port,pin,model,sensor), got: %s", dump ? dump : "?");
        free(dump);
        return -2;
    }

    snprintf(arguments, sizeof(arguments),
             "{\"port\":\"%s\",\"pin\":%d,\"model\":\"%s\",\"sensor\":\"%s\"}",
             port, pin, model, sensor);
    if (call_esp_ubus("get", arguments, &reply) != 0)
        return -1;
    *result = cJSON_Parse(reply);
    free(reply);
    return *result == NULL ? -1 : 0;
}

static int dispatch_action(const char *action_code, const cJSON *input_params,
                           cJSON **result)
{
    if (strcmp(action_code, ACTION_CODE_LIST_ESP_DEVICES) == 0)
        return handle_action_list_esp_devices(input_params, result);
    if (strcmp(action_code, ACTION_CODE_SET_ESP_PIN) == 0)
        return handle_action_set_esp_pin(input_params, result);
    if (strcmp(action_code, ACTION_CODE_GET_ESP_SENSOR) == 0)
        return handle_action_get_esp_sensor(input_params, result);
    return -2;
}

static int send_action_response(const tuyalink_message_t *request,
                                int code, const char *error, cJSON *result)
{
    cJSON *response = cJSON_CreateObject();
    char *payload;
    char topic[TUYA_MQTT_TOPIC_MAXLEN + TUYA_MQTT_DEVICE_ID_MAXLEN + 64];
    int ret;

    if (response == NULL)
        return -1;
    cJSON_AddStringToObject(response, "msgId", request->msgid != NULL ?
                            request->msgid : "");
    cJSON_AddNumberToObject(response, "time", (double) time(NULL) * 1000);
    cJSON_AddNumberToObject(response, "code", code);
    if (error != NULL)
        cJSON_AddStringToObject(response, "error", error);
    if (result != NULL)
        cJSON_AddItemToObject(response, "data", result);

    payload = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);
    if (payload == NULL)
        return -1;

    snprintf(topic, sizeof(topic), "tylink/%s/thing/action/execute_response",
             client_instance.config.device_id);
    ret = mqtt_client_publish(client_instance.mqtt_client, topic,
                              (const uint8_t *) payload, strlen(payload),
                              MQTT_QOS_1);
    free(payload);
    return ret == MQTT_STATUS_SUCCESS ? 0 : -1;
}

// Parses an ACTION_EXECUTE payload (msg->data_string) and dispatches it to
// the matching action handler.
static void handle_action_execute(const tuyalink_message_t *msg)
{
    if (msg->data_string == NULL) {
        syslog(LOG_WARNING, "action execute received with no data");
        send_action_response(msg, 1, "Missing action data", NULL);
        return;
    }

    syslog(LOG_INFO, "action execute payload: %s", msg->data_string);

    cJSON *root = cJSON_Parse(msg->data_string);
    if (root == NULL) {
        syslog(LOG_ERR, "failed to parse action JSON: %s", msg->data_string);
        send_action_response(msg, 1, "Invalid action JSON", NULL);
        return;
    }

    cJSON *action_code = cJSON_GetObjectItem(root, "actionCode");
    cJSON *input_params = cJSON_GetObjectItem(root, "inputParams");

    if (!cJSON_IsString(action_code) || input_params == NULL ||
        !cJSON_IsObject(input_params)) {
        syslog(LOG_ERR, "action JSON missing actionCode/inputParams: %s", msg->data_string);
        send_action_response(msg, 1, "Missing actionCode or inputParams", NULL);
        cJSON_Delete(root);
        return;
    }

    if (!action_is_supported(action_code->valuestring)) {
        syslog(LOG_WARNING, "unrecognized actionCode: %s", action_code->valuestring);
        send_action_response(msg, 1, "Unsupported action", NULL);
        cJSON_Delete(root);
        return;
    }

    syslog(LOG_INFO, "dispatching actionCode=%s", action_code->valuestring);

    cJSON *result = NULL;
    int action_result = dispatch_action(action_code->valuestring, input_params,
                                        &result);
    syslog(LOG_INFO, "actionCode=%s dispatch result=%d",
           action_code->valuestring, action_result);
    send_action_response(msg, action_result == 0 ? 0 : 1,
                         action_result == -2 ? "Invalid action parameters" :
                         action_result == 0 ? NULL : "ESP UBUS request failed",
                         result);
    cJSON_Delete(root);
}

// ----------------- MQTT callbacks -----------------

static void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
    // data model get response can exceed the SDK's fixed RX buffer and force a disconnect
    (void) user_data;
    char topic[128];

    // The bundled SDK only subscribes to the auto_subscribe channel and relies
    // on Tuya's server-side "subscription-free" forwarding for everything else.
    // If that is not active for this project, downlinks never reach us - so
    // subscribe to this device's whole topic tree explicitly.
    snprintf(topic, sizeof(topic), "tylink/%s/#", context->config.device_id);
    mqtt_client_subscribe(context->mqtt_client, topic, 1);
    syslog(LOG_INFO, "Tuya MQTT connected; subscribed to %s", topic);

    // Downlink round-trip probe: ask Tuya for the time. The reply arrives on
    // tylink/<id>/ext/time/response (message type 32). If we never see it,
    // Tuya is not delivering anything downstream to this device.
    if (tuyalink_time_get(context, "{}") == OPRT_OK)
        syslog(LOG_INFO, "sent ext/time/request probe");
    else
        syslog(LOG_ERR, "failed to send ext/time/request probe");
}

static void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
    (void) context;
    (void) user_data;
    syslog(LOG_WARNING, "Tuya MQTT disconnected");
}

static void on_messages(tuya_mqtt_context_t *context, void *user_data, const tuyalink_message_t *msg)
{
    (void) context;
    (void) user_data;

    syslog(LOG_INFO, "received message id=%s type=%d code=%d", msg->msgid, msg->type, msg->code);

    switch (msg->type) {
        case THING_TYPE_ACTION_EXECUTE:
            handle_action_execute(msg);
            break;
        default:
            break;
    }
}

static int mqtt_context_is_initialized(void)
{
    return client_instance.mqtt_client != NULL;
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
        .timeout_ms = 10000,
        .on_connected = on_connected,
        .on_disconnect = on_disconnect,
        .on_messages = on_messages
    });

    if (ret != OPRT_OK) {
        syslog(LOG_ERR, "tuya_mqtt_init failed, code=%d", ret);
        return -1;
    }

    ubus_context = ubus_connect(NULL);
    if (ubus_context == NULL) {
        syslog(LOG_ERR, "failed to connect to ubus");
        tuya_mqtt_deinit(&client_instance);
        client_instance.mqtt_client = NULL;
        return -1;
    }

    ret = tuya_mqtt_connect(&client_instance);
    if (ret != OPRT_OK) {
        syslog(LOG_ERR, "tuya_mqtt_connect failed, code=%d", ret);
        return -1;
    }

    return 0;
}

int tuya_client_stop(void)
{
    if (ubus_context != NULL) {
        ubus_free(ubus_context);
        ubus_context = NULL;
    }
    if (!mqtt_context_is_initialized())
        return 0;

    (void) tuya_mqtt_disconnect(&client_instance);
    (void) tuya_mqtt_deinit(&client_instance);
    client_instance.mqtt_client = NULL;
    return 0;
}

void tuya_client_loop(void)
{
    tuya_mqtt_loop(&client_instance);
}