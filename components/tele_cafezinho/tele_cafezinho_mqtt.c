#include "tele_cafezinho_mqtt.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "tele_cafezinho.h"
#include "tele_cafezinho_config.h"
#include "tele_cafezinho_peers.h"
#include "tele_mqtt.h"

#ifndef TELE_CAFEZINHO_HOST_TEST
#include "esp_log.h"
#include "esp_timer.h"
#endif

#ifndef TELE_CAFEZINHO_HOST_TEST
static const char *TAG = "tele-cafe-mqtt";
static esp_timer_handle_t s_republish_timer;
#endif

static esp_err_t copy_json_string(const cJSON *root,
                                  const char *name,
                                  char *out,
                                  size_t out_size,
                                  bool require_non_empty)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);

    if (!cJSON_IsString(item) || !item->valuestring || !out || out_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (require_non_empty && item->valuestring[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(item->valuestring) >= out_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    snprintf(out, out_size, "%s", item->valuestring);
    return ESP_OK;
}

static esp_err_t get_effective_group(char *out, size_t out_size)
{
#ifdef TELE_CAFEZINHO_HOST_TEST
    if (!out || out_size <= strlen("default")) {
        return ESP_ERR_INVALID_ARG;
    }
    snprintf(out, out_size, "%s", "default");
    return ESP_OK;
#else
    return tele_cafezinho_config_get_group(out, out_size);
#endif
}

static esp_err_t get_effective_source(char *out, size_t out_size)
{
#ifdef TELE_CAFEZINHO_HOST_TEST
    if (!out || out_size <= strlen("gpio")) {
        return ESP_ERR_INVALID_ARG;
    }
    snprintf(out, out_size, "%s", "gpio");
    return ESP_OK;
#else
    return tele_cafezinho_config_get_signal_source(out, out_size);
#endif
}

static esp_err_t get_effective_ttl(uint32_t *out_ttl_s)
{
#ifdef TELE_CAFEZINHO_HOST_TEST
    if (!out_ttl_s) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_ttl_s = 20;
    return ESP_OK;
#else
    return tele_cafezinho_config_get_signal_ttl_s(out_ttl_s);
#endif
}

esp_err_t tele_cafezinho_mqtt_build_signal_payload(bool active, char *out, size_t out_size)
{
    char group[32] = {0};
    char source[16] = {0};
    char device_id[40] = {0};
    uint32_t ttl_s = 0;
    uint32_t seq = 0;
    cJSON *root = NULL;
    char *text = NULL;
    esp_err_t err = ESP_OK;

    if (!out || out_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    err = get_effective_group(group, sizeof(group));
    if (err != ESP_OK) {
        return err;
    }
    err = get_effective_source(source, sizeof(source));
    if (err != ESP_OK) {
        return err;
    }
    err = get_effective_ttl(&ttl_s);
    if (err != ESP_OK) {
        return err;
    }
    err = tele_mqtt_get_device_id(device_id, sizeof(device_id));
    if (err != ESP_OK) {
        return err;
    }

    seq = tele_cafezinho_next_signal_seq();
    root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }
    if (!cJSON_AddStringToObject(root, "schema", TELE_CAFEZINHO_MQTT_SCHEMA) ||
        !cJSON_AddStringToObject(root, "group", group) ||
        !cJSON_AddStringToObject(root, "device_id", device_id) ||
        !cJSON_AddStringToObject(root, "signal", active ? "active" : "inactive") ||
        !cJSON_AddStringToObject(root, "source", source) ||
        !cJSON_AddNumberToObject(root, "seq", seq) ||
        !cJSON_AddNumberToObject(root, "ttl_s", ttl_s)) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!text) {
        return ESP_ERR_NO_MEM;
    }
    if (strlen(text) >= out_size) {
        cJSON_free(text);
        return ESP_ERR_INVALID_SIZE;
    }
    snprintf(out, out_size, "%s", text);
    cJSON_free(text);
    return ESP_OK;
}

esp_err_t tele_cafezinho_mqtt_parse_signal_payload(const char *payload,
                                                   size_t payload_len,
                                                   tele_cafezinho_signal_msg_t *out_msg)
{
    cJSON *root = NULL;
    const cJSON *signal = NULL;
    const cJSON *seq = NULL;
    const cJSON *ttl_s = NULL;
    esp_err_t err = ESP_OK;

    if (!payload || payload_len == 0 || !out_msg) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(out_msg, 0, sizeof(*out_msg));

    root = cJSON_ParseWithLength(payload, payload_len);
    if (!root) {
        return ESP_ERR_INVALID_ARG;
    }

    err = copy_json_string(root, "schema", out_msg->schema, sizeof(out_msg->schema), true);
    if (err == ESP_OK && strcmp(out_msg->schema, TELE_CAFEZINHO_MQTT_SCHEMA) != 0) {
        err = ESP_ERR_INVALID_ARG;
    }
    if (err == ESP_OK) {
        err = copy_json_string(root, "group", out_msg->group, sizeof(out_msg->group), true);
    }
    if (err == ESP_OK) {
        err = copy_json_string(root, "device_id", out_msg->device_id, sizeof(out_msg->device_id), true);
    }
    if (err == ESP_OK) {
        err = copy_json_string(root, "source", out_msg->source, sizeof(out_msg->source), true);
    }

    signal = cJSON_GetObjectItemCaseSensitive(root, "signal");
    if (err == ESP_OK && (!cJSON_IsString(signal) || !signal->valuestring)) {
        err = ESP_ERR_INVALID_ARG;
    }
    if (err == ESP_OK && strcmp(signal->valuestring, "active") == 0) {
        out_msg->active = true;
    } else if (err == ESP_OK && strcmp(signal->valuestring, "inactive") == 0) {
        out_msg->active = false;
    } else if (err == ESP_OK) {
        err = ESP_ERR_INVALID_ARG;
    }

    seq = cJSON_GetObjectItemCaseSensitive(root, "seq");
    ttl_s = cJSON_GetObjectItemCaseSensitive(root, "ttl_s");
    if (err == ESP_OK && (!cJSON_IsNumber(seq) || !cJSON_IsNumber(ttl_s))) {
        err = ESP_ERR_INVALID_ARG;
    }
    if (err == ESP_OK) {
        out_msg->seq = (uint32_t)seq->valuedouble;
        out_msg->ttl_s = (uint32_t)ttl_s->valuedouble;
        if (out_msg->active && out_msg->ttl_s == 0) {
            err = ESP_ERR_INVALID_ARG;
        }
    }

    cJSON_Delete(root);
    return err;
}

esp_err_t tele_cafezinho_mqtt_publish_signal(bool active)
{
    char payload[256] = {0};
    esp_err_t err = tele_cafezinho_mqtt_build_signal_payload(active, payload, sizeof(payload));
    if (err != ESP_OK) {
        return err;
    }
    return tele_mqtt_publish_shared(TELE_CAFEZINHO_MQTT_TOPIC_SUFFIX, payload, 1, false);
}

#ifndef TELE_CAFEZINHO_HOST_TEST
static esp_err_t on_shared_signal(const char *topic, const char *payload, size_t payload_len, void *ctx)
{
    (void)topic;
    (void)ctx;
    tele_cafezinho_signal_msg_t msg = {0};
    char own_device_id[40] = {0};
    const char *group = tele_cafezinho_get_group();
    esp_err_t err = tele_cafezinho_mqtt_parse_signal_payload(payload, payload_len, &msg);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "invalid TeleCafezinho shared payload: %s", esp_err_to_name(err));
        return ESP_OK;
    }
    err = tele_mqtt_get_device_id(own_device_id, sizeof(own_device_id));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "cannot process shared signal without MQTT device_id: %s", esp_err_to_name(err));
        return ESP_OK;
    }

    err = tele_cafezinho_peers_apply_message(&msg,
                                             own_device_id,
                                             group,
                                             esp_timer_get_time() / 1000);
    if (err == ESP_OK) {
        tele_cafezinho_set_remote_active_count(tele_cafezinho_peers_active_count());
        if (msg.active && strcmp(msg.device_id, own_device_id) != 0 &&
            strcmp(msg.group, group) == 0) {
            tele_cafezinho_set_last_remote_device_id(msg.device_id);
        }
    }
    return ESP_OK;
}

static void republish_timer_cb(void *ctx)
{
    (void)ctx;
    if (tele_cafezinho_get_local_active()) {
        esp_err_t err = tele_cafezinho_mqtt_publish_signal(true);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "active republish failed: %s", esp_err_to_name(err));
        }
    }
}

esp_err_t tele_cafezinho_mqtt_start(void)
{
    uint32_t interval_s = 5;
    uint32_t ttl_s = 20;
    esp_err_t err = tele_mqtt_subscribe_shared(TELE_CAFEZINHO_MQTT_TOPIC_SUFFIX,
                                               1,
                                               on_shared_signal,
                                               NULL);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    if (s_republish_timer) {
        return ESP_OK;
    }
    (void)tele_cafezinho_config_get_publish_interval_s(&interval_s);
    (void)tele_cafezinho_config_get_signal_ttl_s(&ttl_s);
    if (interval_s >= ttl_s && ttl_s > 1) {
        interval_s = ttl_s - 1;
    }

    const esp_timer_create_args_t args = {
        .callback = republish_timer_cb,
        .name = "tcafe_pub",
    };
    err = esp_timer_create(&args, &s_republish_timer);
    if (err != ESP_OK) {
        return err;
    }
    return esp_timer_start_periodic(s_republish_timer, (uint64_t)interval_s * 1000000ULL);
}
#else
esp_err_t tele_cafezinho_mqtt_start(void)
{
    return ESP_OK;
}
#endif
