#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define TELE_CAFEZINHO_HOST_TEST 1
#include "tele_cafezinho.h"
#include "tele_cafezinho_mqtt.h"

static char s_device_id[40] = "TCafe-A1B2C3";

esp_err_t tele_mqtt_get_device_id(char *out, size_t out_size)
{
    size_t len = strlen(s_device_id);
    if (!out || out_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (out_size <= len) {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(out, s_device_id, len + 1);
    return ESP_OK;
}

esp_err_t tele_mqtt_publish_shared(const char *topic_suffix,
                                   const char *payload,
                                   int qos,
                                   bool retain)
{
    (void)topic_suffix;
    (void)payload;
    (void)qos;
    (void)retain;
    return ESP_OK;
}

static void test_build_and_parse_active(void)
{
    char payload[256] = {0};
    tele_cafezinho_signal_msg_t msg = {0};

    tele_cafezinho_host_test_reset();
    assert(tele_cafezinho_mqtt_build_signal_payload(true, payload, sizeof(payload)) == ESP_OK);
    assert(tele_cafezinho_mqtt_parse_signal_payload(payload, strlen(payload), &msg) == ESP_OK);
    assert(strcmp(msg.schema, TELE_CAFEZINHO_MQTT_SCHEMA) == 0);
    assert(strcmp(msg.group, "default") == 0);
    assert(strcmp(msg.device_id, "TCafe-A1B2C3") == 0);
    assert(msg.active);
    assert(strcmp(msg.source, "gpio") == 0);
    assert(msg.seq == 1);
    assert(msg.ttl_s == 20);
}

static void test_build_and_parse_inactive(void)
{
    char payload[256] = {0};
    tele_cafezinho_signal_msg_t msg = {0};

    assert(tele_cafezinho_mqtt_build_signal_payload(false, payload, sizeof(payload)) == ESP_OK);
    assert(tele_cafezinho_mqtt_parse_signal_payload(payload, strlen(payload), &msg) == ESP_OK);
    assert(!msg.active);
    assert(msg.seq == 2);
}

static void test_rejects_invalid_payloads(void)
{
    tele_cafezinho_signal_msg_t msg = {0};
    const char *wrong_schema =
        "{\"schema\":\"wrong\",\"group\":\"default\",\"device_id\":\"dev\",\"signal\":\"active\",\"source\":\"gpio\",\"seq\":1,\"ttl_s\":20}";
    const char *missing_group =
        "{\"schema\":\"telecafezinho.signal.v1\",\"group\":\"\",\"device_id\":\"dev\",\"signal\":\"active\",\"source\":\"gpio\",\"seq\":1,\"ttl_s\":20}";
    const char *bad_signal =
        "{\"schema\":\"telecafezinho.signal.v1\",\"group\":\"default\",\"device_id\":\"dev\",\"signal\":\"maybe\",\"source\":\"gpio\",\"seq\":1,\"ttl_s\":20}";

    assert(tele_cafezinho_mqtt_parse_signal_payload("{", 1, &msg) == ESP_ERR_INVALID_ARG);
    assert(tele_cafezinho_mqtt_parse_signal_payload(wrong_schema, strlen(wrong_schema), &msg) ==
           ESP_ERR_INVALID_ARG);
    assert(tele_cafezinho_mqtt_parse_signal_payload(missing_group, strlen(missing_group), &msg) ==
           ESP_ERR_INVALID_ARG);
    assert(tele_cafezinho_mqtt_parse_signal_payload(bad_signal, strlen(bad_signal), &msg) ==
           ESP_ERR_INVALID_ARG);
}

int main(void)
{
    test_build_and_parse_active();
    test_build_and_parse_inactive();
    test_rejects_invalid_payloads();
    return 0;
}
