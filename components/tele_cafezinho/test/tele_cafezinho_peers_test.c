#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define TELE_CAFEZINHO_HOST_TEST 1
#include "tele_cafezinho_peers.h"

static tele_cafezinho_signal_msg_t msg(const char *group,
                                       const char *device_id,
                                       bool active,
                                       uint32_t seq,
                                       uint32_t ttl_s)
{
    tele_cafezinho_signal_msg_t value = {
        .active = active,
        .seq = seq,
        .ttl_s = ttl_s,
    };
    snprintf(value.schema, sizeof(value.schema), "%s", TELE_CAFEZINHO_MQTT_SCHEMA);
    snprintf(value.group, sizeof(value.group), "%s", group);
    snprintf(value.device_id, sizeof(value.device_id), "%s", device_id);
    snprintf(value.source, sizeof(value.source), "%s", "gpio");
    return value;
}

static void test_filters_and_counts_peers(void)
{
    tele_cafezinho_peers_clear();
    tele_cafezinho_signal_msg_t current = {0};

    current = msg("mesa-01", "self", true, 1, 20);
    assert(tele_cafezinho_peers_apply_message(&current,
                                              "self",
                                              "mesa-01",
                                              1000) == ESP_OK);
    assert(tele_cafezinho_peers_active_count() == 0);

    current = msg("mesa-02", "peer-a", true, 1, 20);
    assert(tele_cafezinho_peers_apply_message(&current,
                                              "self",
                                              "mesa-01",
                                              1000) == ESP_OK);
    assert(tele_cafezinho_peers_active_count() == 0);

    current = msg("mesa-01", "peer-a", true, 1, 20);
    assert(tele_cafezinho_peers_apply_message(&current,
                                              "self",
                                              "mesa-01",
                                              1000) == ESP_OK);
    assert(tele_cafezinho_peers_active_count() == 1);

    current = msg("mesa-01", "peer-a", true, 1, 20);
    assert(tele_cafezinho_peers_apply_message(&current,
                                              "self",
                                              "mesa-01",
                                              2000) == ESP_OK);
    assert(tele_cafezinho_peers_active_count() == 1);

    current = msg("mesa-01", "peer-b", true, 1, 20);
    assert(tele_cafezinho_peers_apply_message(&current,
                                              "self",
                                              "mesa-01",
                                              2000) == ESP_OK);
    assert(tele_cafezinho_peers_active_count() == 2);

    current = msg("mesa-01", "peer-a", false, 2, 20);
    assert(tele_cafezinho_peers_apply_message(&current,
                                              "self",
                                              "mesa-01",
                                              3000) == ESP_OK);
    assert(tele_cafezinho_peers_active_count() == 1);
}

static void test_expiration(void)
{
    tele_cafezinho_peers_clear();
    tele_cafezinho_signal_msg_t current = msg("mesa-01", "peer-a", true, 1, 2);

    assert(tele_cafezinho_peers_apply_message(&current,
                                              "self",
                                              "mesa-01",
                                              1000) == ESP_OK);
    assert(tele_cafezinho_peers_active_count() == 1);
    assert(tele_cafezinho_peers_expire(2999) == 1);
    assert(tele_cafezinho_peers_expire(3000) == 0);
    assert(tele_cafezinho_peers_active_count() == 0);
}

int main(void)
{
    test_filters_and_counts_peers();
    test_expiration();
    return 0;
}
