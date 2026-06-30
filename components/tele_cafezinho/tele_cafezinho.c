#include "tele_cafezinho.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef TELE_CAFEZINHO_HOST_TEST
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "tele_cafezinho_commands.h"
#include "tele_cafezinho_config.h"
#include "tele_cafezinho_gpio.h"
#include "tele_cafezinho_mqtt.h"
#include "tele_cafezinho_peers.h"
#include "tele_cafezinho_status.h"
#include "tele_indicator.h"
#else
#include "tele_cafezinho_peers.h"
#endif

#ifndef TELE_CAFEZINHO_HOST_TEST
static const char *TAG = "tele-cafezinho";
#endif
static bool s_started;
static bool s_local_active;
static uint32_t s_remote_active_count;
static uint32_t s_signal_seq;
static char s_group[32] = "default";
static char s_signal_source[16] = "gpio";
static char s_last_remote_device_id[40];

static tele_cafezinho_combined_state_t calculate_combined_state(void)
{
    if (s_local_active && s_remote_active_count > 0) {
        return TELE_CAFEZINHO_COMBINED_STATE_MUTUAL_ACTIVE;
    }
    if (s_local_active) {
        return TELE_CAFEZINHO_COMBINED_STATE_LOCAL_ACTIVE;
    }
    if (s_remote_active_count > 0) {
        return TELE_CAFEZINHO_COMBINED_STATE_REMOTE_ACTIVE;
    }
    return TELE_CAFEZINHO_COMBINED_STATE_IDLE;
}

#ifndef TELE_CAFEZINHO_HOST_TEST
static void update_indicator(void)
{
    tele_cafezinho_combined_state_t state = calculate_combined_state();
    esp_err_t err = tele_indicator_clear_source("telecafe");
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "failed to clear telecafe indicator: %s", esp_err_to_name(err));
    }

    switch (state) {
    case TELE_CAFEZINHO_COMBINED_STATE_LOCAL_ACTIVE:
        err = tele_indicator_raise("telecafe", "telecafe.local_active");
        break;
    case TELE_CAFEZINHO_COMBINED_STATE_REMOTE_ACTIVE:
        err = tele_indicator_raise("telecafe", "telecafe.remote_active");
        break;
    case TELE_CAFEZINHO_COMBINED_STATE_MUTUAL_ACTIVE:
        err = tele_indicator_raise("telecafe", "telecafe.mutual_active");
        break;
    case TELE_CAFEZINHO_COMBINED_STATE_IDLE:
    default:
        err = ESP_OK;
        break;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "failed to raise telecafe indicator: %s", esp_err_to_name(err));
    }
}

static esp_err_t load_cached_config(void)
{
    esp_err_t err = tele_cafezinho_config_get_group(s_group, sizeof(s_group));
    if (err != ESP_OK) {
        return err;
    }
    err = tele_cafezinho_config_get_signal_source(s_signal_source, sizeof(s_signal_source));
    if (err != ESP_OK) {
        return err;
    }
    return ESP_OK;
}
#else
static void update_indicator(void)
{
}
#endif

esp_err_t tele_cafezinho_start(void)
{
    if (s_started) {
        return ESP_OK;
    }
#ifndef TELE_CAFEZINHO_HOST_TEST
    ESP_RETURN_ON_ERROR(tele_cafezinho_config_register(), TAG, "config register failed");
    ESP_RETURN_ON_ERROR(load_cached_config(), TAG, "config load failed");
    ESP_RETURN_ON_ERROR(tele_cafezinho_status_register(), TAG, "status register failed");
    ESP_RETURN_ON_ERROR(tele_cafezinho_commands_register(), TAG, "commands register failed");
    ESP_RETURN_ON_ERROR(tele_cafezinho_peers_start(), TAG, "peers start failed");
    ESP_RETURN_ON_ERROR(tele_cafezinho_mqtt_start(), TAG, "mqtt integration start failed");
    ESP_RETURN_ON_ERROR(tele_cafezinho_gpio_start(), TAG, "gpio start failed");
#endif
    s_started = true;
    update_indicator();
    return ESP_OK;
}

esp_err_t tele_cafezinho_set_local_signal(bool active)
{
    bool changed = s_local_active != active;
    s_local_active = active;
#ifndef TELE_CAFEZINHO_HOST_TEST
    if (changed) {
        esp_err_t err = tele_cafezinho_mqtt_publish_signal(active);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "local signal publish failed: %s", esp_err_to_name(err));
        }
    }
#endif
    update_indicator();
    return ESP_OK;
}

bool tele_cafezinho_get_local_active(void)
{
    return s_local_active;
}

uint32_t tele_cafezinho_get_remote_active_count(void)
{
    return s_remote_active_count;
}

const char *tele_cafezinho_get_group(void)
{
    return s_group;
}

const char *tele_cafezinho_get_signal_source(void)
{
    return s_signal_source;
}

const char *tele_cafezinho_get_last_remote_device_id(void)
{
    return s_last_remote_device_id;
}

uint32_t tele_cafezinho_get_signal_seq(void)
{
    return s_signal_seq;
}

uint32_t tele_cafezinho_next_signal_seq(void)
{
    s_signal_seq++;
    return s_signal_seq;
}

void tele_cafezinho_set_remote_active_count(uint32_t count)
{
    s_remote_active_count = count;
    update_indicator();
}

void tele_cafezinho_set_last_remote_device_id(const char *device_id)
{
    if (!device_id) {
        s_last_remote_device_id[0] = '\0';
        return;
    }
    snprintf(s_last_remote_device_id, sizeof(s_last_remote_device_id), "%s", device_id);
}

void tele_cafezinho_clear_peers(void)
{
    tele_cafezinho_peers_clear();
    tele_cafezinho_set_remote_active_count(0);
    s_last_remote_device_id[0] = '\0';
}

cJSON *tele_cafezinho_build_peers_json(void)
{
#ifdef TELE_CAFEZINHO_HOST_TEST
    return tele_cafezinho_peers_build_json(0);
#else
    return tele_cafezinho_peers_build_json(esp_timer_get_time() / 1000);
#endif
}

tele_cafezinho_combined_state_t tele_cafezinho_get_combined_state(void)
{
    return calculate_combined_state();
}

const char *tele_cafezinho_combined_state_to_string(tele_cafezinho_combined_state_t state)
{
    switch (state) {
    case TELE_CAFEZINHO_COMBINED_STATE_LOCAL_ACTIVE:
        return "local_active";
    case TELE_CAFEZINHO_COMBINED_STATE_REMOTE_ACTIVE:
        return "remote_active";
    case TELE_CAFEZINHO_COMBINED_STATE_MUTUAL_ACTIVE:
        return "mutual_active";
    case TELE_CAFEZINHO_COMBINED_STATE_IDLE:
    default:
        return "idle";
    }
}

#ifdef TELE_CAFEZINHO_HOST_TEST
void tele_cafezinho_host_test_reset(void)
{
    s_started = false;
    s_local_active = false;
    s_remote_active_count = 0;
    s_signal_seq = 0;
    snprintf(s_group, sizeof(s_group), "%s", "default");
    snprintf(s_signal_source, sizeof(s_signal_source), "%s", "gpio");
    s_last_remote_device_id[0] = '\0';
}

void tele_cafezinho_host_test_set_remote_active_count(uint32_t count)
{
    s_remote_active_count = count;
}
#endif
