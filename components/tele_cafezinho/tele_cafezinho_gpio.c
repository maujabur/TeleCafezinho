#include "tele_cafezinho_gpio.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tele_cafezinho.h"
#include "tele_cafezinho_config.h"

static const char *TAG = "tele-cafe-gpio";
static bool s_started;

static void gpio_poll_task(void *ctx)
{
    const int gpio_num = (int)(intptr_t)ctx;
    int32_t active_level = 1;
    uint32_t debounce_ms = 150;
    bool last_raw_active = false;
    bool debounced_active = false;
    TickType_t changed_at = xTaskGetTickCount();

    (void)tele_cafezinho_config_get_gpio_active_level(&active_level);
    (void)tele_cafezinho_config_get_gpio_debounce_ms(&debounce_ms);

    while (true) {
        bool raw_active = gpio_get_level((gpio_num_t)gpio_num) == active_level;
        TickType_t now = xTaskGetTickCount();

        if (raw_active != last_raw_active) {
            last_raw_active = raw_active;
            changed_at = now;
        }

        if (raw_active != debounced_active &&
            pdTICKS_TO_MS(now - changed_at) >= debounce_ms) {
            debounced_active = raw_active;
            (void)tele_cafezinho_set_local_signal(debounced_active);
        }

        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

esp_err_t tele_cafezinho_gpio_start(void)
{
    char source[16] = {0};
    int32_t gpio_num = -1;
    esp_err_t err = ESP_OK;

    if (s_started) {
        return ESP_OK;
    }

    err = tele_cafezinho_config_get_signal_source(source, sizeof(source));
    if (err != ESP_OK) {
        return err;
    }
    if (strcmp(source, "gpio") != 0) {
        ESP_LOGW(TAG, "unsupported signal source: %s", source);
        s_started = true;
        return ESP_OK;
    }

    err = tele_cafezinho_config_get_gpio_num(&gpio_num);
    if (err != ESP_OK) {
        return err;
    }
    if (gpio_num < 0) {
        ESP_LOGW(TAG, "TeleCafezinho GPIO source selected but telecafe.gpio_num is disabled");
        s_started = true;
        return ESP_OK;
    }

    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << gpio_num,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));

    BaseType_t ok = xTaskCreate(gpio_poll_task,
                                "tcafe_gpio",
                                3072,
                                (void *)(intptr_t)gpio_num,
                                tskIDLE_PRIORITY + 1,
                                NULL);
    if (ok != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    s_started = true;
    return ESP_OK;
}
