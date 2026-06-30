#include "tele_cafezinho_config.h"

#include <string.h>

#include "tele_channels.h"
#include "tele_config.h"

static const tele_config_field_t s_fields[] = {
    {
        .id = TELE_CAFEZINHO_CONFIG_ID_GROUP,
        .nvs_key = "tc_group",
        .type = TELE_CONFIG_TYPE_STRING,
        .default_value.string = "default",
        .min_len = 1,
        .max_len = 31,
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
    },
    {
        .id = TELE_CAFEZINHO_CONFIG_ID_SIGNAL_SOURCE,
        .nvs_key = "tc_src",
        .type = TELE_CONFIG_TYPE_STRING,
        .default_value.string = "gpio",
        .min_len = 1,
        .max_len = 15,
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
        .flags = TELE_CONFIG_FLAG_REBOOT_REQUIRED,
    },
    {
        .id = TELE_CAFEZINHO_CONFIG_ID_GPIO_NUM,
        .nvs_key = "tc_gpio",
        .type = TELE_CONFIG_TYPE_I32,
        .default_value.i32 = -1,
        .min.i32 = -1,
        .max.i32 = 48,
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
        .flags = TELE_CONFIG_FLAG_REBOOT_REQUIRED,
    },
    {
        .id = TELE_CAFEZINHO_CONFIG_ID_GPIO_ACTIVE_LEVEL,
        .nvs_key = "tc_level",
        .type = TELE_CONFIG_TYPE_I32,
        .default_value.i32 = 1,
        .min.i32 = 0,
        .max.i32 = 1,
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
        .flags = TELE_CONFIG_FLAG_REBOOT_REQUIRED,
    },
    {
        .id = TELE_CAFEZINHO_CONFIG_ID_GPIO_DEBOUNCE_MS,
        .nvs_key = "tc_deb",
        .type = TELE_CONFIG_TYPE_U32,
        .default_value.u32 = 150,
        .min.u32 = 20,
        .max.u32 = 5000,
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
        .flags = TELE_CONFIG_FLAG_REBOOT_REQUIRED,
    },
    {
        .id = TELE_CAFEZINHO_CONFIG_ID_SIGNAL_TTL_S,
        .nvs_key = "tc_ttl",
        .type = TELE_CONFIG_TYPE_U32,
        .default_value.u32 = 20,
        .min.u32 = 2,
        .max.u32 = 3600,
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
    },
    {
        .id = TELE_CAFEZINHO_CONFIG_ID_SIGNAL_PUBLISH_INTERVAL_S,
        .nvs_key = "tc_pubint",
        .type = TELE_CONFIG_TYPE_U32,
        .default_value.u32 = 5,
        .min.u32 = 1,
        .max.u32 = 3600,
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
    },
};

esp_err_t tele_cafezinho_config_register(void)
{
    return tele_config_register_fields(s_fields, sizeof(s_fields) / sizeof(s_fields[0]));
}

static esp_err_t get_string_config(const char *id, char *out, size_t out_size)
{
    tele_config_value_t value = {0};

    if (!out || out_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(out, 0, out_size);
    return tele_config_get_effective(id, &value, out, out_size, NULL);
}

static esp_err_t get_i32_config(const char *id, int32_t *out_value)
{
    tele_config_value_t value = {0};
    esp_err_t err = ESP_OK;

    if (!out_value) {
        return ESP_ERR_INVALID_ARG;
    }
    err = tele_config_get_effective(id, &value, NULL, 0, NULL);
    if (err == ESP_OK) {
        *out_value = value.i32;
    }
    return err;
}

static esp_err_t get_u32_config(const char *id, uint32_t *out_value)
{
    tele_config_value_t value = {0};
    esp_err_t err = ESP_OK;

    if (!out_value) {
        return ESP_ERR_INVALID_ARG;
    }
    err = tele_config_get_effective(id, &value, NULL, 0, NULL);
    if (err == ESP_OK) {
        *out_value = value.u32;
    }
    return err;
}

esp_err_t tele_cafezinho_config_get_group(char *out, size_t out_size)
{
    return get_string_config(TELE_CAFEZINHO_CONFIG_ID_GROUP, out, out_size);
}

esp_err_t tele_cafezinho_config_get_signal_source(char *out, size_t out_size)
{
    return get_string_config(TELE_CAFEZINHO_CONFIG_ID_SIGNAL_SOURCE, out, out_size);
}

esp_err_t tele_cafezinho_config_get_gpio_num(int32_t *out_value)
{
    return get_i32_config(TELE_CAFEZINHO_CONFIG_ID_GPIO_NUM, out_value);
}

esp_err_t tele_cafezinho_config_get_gpio_active_level(int32_t *out_value)
{
    return get_i32_config(TELE_CAFEZINHO_CONFIG_ID_GPIO_ACTIVE_LEVEL, out_value);
}

esp_err_t tele_cafezinho_config_get_gpio_debounce_ms(uint32_t *out_value)
{
    return get_u32_config(TELE_CAFEZINHO_CONFIG_ID_GPIO_DEBOUNCE_MS, out_value);
}

esp_err_t tele_cafezinho_config_get_signal_ttl_s(uint32_t *out_value)
{
    return get_u32_config(TELE_CAFEZINHO_CONFIG_ID_SIGNAL_TTL_S, out_value);
}

esp_err_t tele_cafezinho_config_get_publish_interval_s(uint32_t *out_value)
{
    return get_u32_config(TELE_CAFEZINHO_CONFIG_ID_SIGNAL_PUBLISH_INTERVAL_S, out_value);
}
