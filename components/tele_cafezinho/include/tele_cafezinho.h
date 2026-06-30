#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TELE_CAFEZINHO_COMBINED_STATE_IDLE = 0,
    TELE_CAFEZINHO_COMBINED_STATE_LOCAL_ACTIVE,
    TELE_CAFEZINHO_COMBINED_STATE_REMOTE_ACTIVE,
    TELE_CAFEZINHO_COMBINED_STATE_MUTUAL_ACTIVE,
} tele_cafezinho_combined_state_t;

esp_err_t tele_cafezinho_start(void);
esp_err_t tele_cafezinho_set_local_signal(bool active);
bool tele_cafezinho_get_local_active(void);
uint32_t tele_cafezinho_get_remote_active_count(void);
tele_cafezinho_combined_state_t tele_cafezinho_get_combined_state(void);
const char *tele_cafezinho_combined_state_to_string(tele_cafezinho_combined_state_t state);
const char *tele_cafezinho_get_group(void);
const char *tele_cafezinho_get_signal_source(void);
const char *tele_cafezinho_get_last_remote_device_id(void);
uint32_t tele_cafezinho_get_signal_seq(void);
uint32_t tele_cafezinho_next_signal_seq(void);
void tele_cafezinho_set_remote_active_count(uint32_t count);
void tele_cafezinho_set_last_remote_device_id(const char *device_id);
void tele_cafezinho_clear_peers(void);
cJSON *tele_cafezinho_build_peers_json(void);

#ifdef TELE_CAFEZINHO_HOST_TEST
void tele_cafezinho_host_test_reset(void);
void tele_cafezinho_host_test_set_remote_active_count(uint32_t count);
#endif

#ifdef __cplusplus
}
#endif
