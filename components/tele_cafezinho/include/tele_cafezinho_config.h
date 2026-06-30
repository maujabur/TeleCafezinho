#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TELE_CAFEZINHO_CONFIG_ID_GROUP "telecafe.group"
#define TELE_CAFEZINHO_CONFIG_ID_SIGNAL_SOURCE "telecafe.signal_source"
#define TELE_CAFEZINHO_CONFIG_ID_GPIO_NUM "telecafe.gpio_num"
#define TELE_CAFEZINHO_CONFIG_ID_GPIO_ACTIVE_LEVEL "telecafe.gpio_active_level"
#define TELE_CAFEZINHO_CONFIG_ID_GPIO_DEBOUNCE_MS "telecafe.gpio_debounce_ms"
#define TELE_CAFEZINHO_CONFIG_ID_SIGNAL_TTL_S "telecafe.signal_ttl_s"
#define TELE_CAFEZINHO_CONFIG_ID_SIGNAL_PUBLISH_INTERVAL_S "telecafe.signal_publish_interval_s"

esp_err_t tele_cafezinho_config_register(void);
esp_err_t tele_cafezinho_config_get_group(char *out, size_t out_size);
esp_err_t tele_cafezinho_config_get_signal_source(char *out, size_t out_size);
esp_err_t tele_cafezinho_config_get_gpio_num(int32_t *out_value);
esp_err_t tele_cafezinho_config_get_gpio_active_level(int32_t *out_value);
esp_err_t tele_cafezinho_config_get_gpio_debounce_ms(uint32_t *out_value);
esp_err_t tele_cafezinho_config_get_signal_ttl_s(uint32_t *out_value);
esp_err_t tele_cafezinho_config_get_publish_interval_s(uint32_t *out_value);

#ifdef __cplusplus
}
#endif
