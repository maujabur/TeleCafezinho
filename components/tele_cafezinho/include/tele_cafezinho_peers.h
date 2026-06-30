#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"
#include "esp_err.h"
#include "tele_cafezinho_mqtt.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t tele_cafezinho_peers_start(void);
esp_err_t tele_cafezinho_peers_apply_message(const tele_cafezinho_signal_msg_t *msg,
                                             const char *own_device_id,
                                             const char *group,
                                             int64_t now_ms);
uint32_t tele_cafezinho_peers_expire(int64_t now_ms);
void tele_cafezinho_peers_clear(void);
uint32_t tele_cafezinho_peers_active_count(void);
cJSON *tele_cafezinho_peers_build_json(int64_t now_ms);

#ifdef __cplusplus
}
#endif
