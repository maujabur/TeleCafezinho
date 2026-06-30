#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TELE_CAFEZINHO_MQTT_TOPIC_SUFFIX "telecafezinho/signal"
#define TELE_CAFEZINHO_MQTT_SCHEMA "telecafezinho.signal.v1"

typedef struct {
    char schema[32];
    char group[32];
    char device_id[40];
    bool active;
    char source[16];
    uint32_t seq;
    uint32_t ttl_s;
} tele_cafezinho_signal_msg_t;

esp_err_t tele_cafezinho_mqtt_start(void);
esp_err_t tele_cafezinho_mqtt_publish_signal(bool active);
esp_err_t tele_cafezinho_mqtt_build_signal_payload(bool active, char *out, size_t out_size);
esp_err_t tele_cafezinho_mqtt_parse_signal_payload(const char *payload,
                                                   size_t payload_len,
                                                   tele_cafezinho_signal_msg_t *out_msg);

#ifdef __cplusplus
}
#endif
