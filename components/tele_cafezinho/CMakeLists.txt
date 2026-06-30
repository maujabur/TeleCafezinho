idf_component_register(
    SRCS
        "tele_cafezinho.c"
        "tele_cafezinho_commands.c"
        "tele_cafezinho_config.c"
        "tele_cafezinho_gpio.c"
        "tele_cafezinho_mqtt.c"
        "tele_cafezinho_peers.c"
        "tele_cafezinho_status.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_timer tele_config tele_status tele_commands tele_mqtt tele_indicator espressif__cjson
)
