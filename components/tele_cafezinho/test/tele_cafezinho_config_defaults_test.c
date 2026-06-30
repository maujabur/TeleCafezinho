#include <assert.h>
#include <stdint.h>
#include <string.h>

#define TELE_CONFIG_HOST_TEST 1

#include "tele_cafezinho_config.h"
#include "tele_config.h"

int main(void)
{
    char text[64] = {0};
    int32_t i32 = 0;
    uint32_t u32 = 0;

    assert(tele_cafezinho_config_register() == ESP_OK);

    assert(tele_cafezinho_config_get_group(text, sizeof(text)) == ESP_OK);
    assert(strcmp(text, "bancada") == 0);

    memset(text, 0, sizeof(text));
    assert(tele_cafezinho_config_get_signal_source(text, sizeof(text)) == ESP_OK);
    assert(strcmp(text, "gpio") == 0);

    assert(tele_cafezinho_config_get_gpio_num(&i32) == ESP_OK);
    assert(i32 == 4);

    assert(tele_cafezinho_config_get_gpio_active_level(&i32) == ESP_OK);
    assert(i32 == 0);

    assert(tele_cafezinho_config_get_gpio_debounce_ms(&u32) == ESP_OK);
    assert(u32 == 75);

    assert(tele_cafezinho_config_get_signal_ttl_s(&u32) == ESP_OK);
    assert(u32 == 33);

    assert(tele_cafezinho_config_get_publish_interval_s(&u32) == ESP_OK);
    assert(u32 == 7);

    return 0;
}
