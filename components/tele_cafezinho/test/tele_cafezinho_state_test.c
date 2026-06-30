#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define TELE_CAFEZINHO_HOST_TEST 1
#include "tele_cafezinho.h"

static void test_initial_state_is_idle(void)
{
    tele_cafezinho_host_test_reset();

    assert(!tele_cafezinho_get_local_active());
    assert(tele_cafezinho_get_remote_active_count() == 0);
    assert(tele_cafezinho_get_combined_state() == TELE_CAFEZINHO_COMBINED_STATE_IDLE);
    assert(strcmp(tele_cafezinho_combined_state_to_string(TELE_CAFEZINHO_COMBINED_STATE_IDLE),
                  "idle") == 0);
}

static void test_local_active_only(void)
{
    tele_cafezinho_host_test_reset();

    assert(tele_cafezinho_set_local_signal(true) == ESP_OK);

    assert(tele_cafezinho_get_local_active());
    assert(tele_cafezinho_get_remote_active_count() == 0);
    assert(tele_cafezinho_get_combined_state() == TELE_CAFEZINHO_COMBINED_STATE_LOCAL_ACTIVE);
    assert(strcmp(tele_cafezinho_combined_state_to_string(TELE_CAFEZINHO_COMBINED_STATE_LOCAL_ACTIVE),
                  "local_active") == 0);
}

static void test_remote_active_only(void)
{
    tele_cafezinho_host_test_reset();

    tele_cafezinho_host_test_set_remote_active_count(2);

    assert(!tele_cafezinho_get_local_active());
    assert(tele_cafezinho_get_remote_active_count() == 2);
    assert(tele_cafezinho_get_combined_state() == TELE_CAFEZINHO_COMBINED_STATE_REMOTE_ACTIVE);
    assert(strcmp(tele_cafezinho_combined_state_to_string(TELE_CAFEZINHO_COMBINED_STATE_REMOTE_ACTIVE),
                  "remote_active") == 0);
}

static void test_mutual_active(void)
{
    tele_cafezinho_host_test_reset();

    assert(tele_cafezinho_set_local_signal(true) == ESP_OK);
    tele_cafezinho_host_test_set_remote_active_count(1);

    assert(tele_cafezinho_get_combined_state() == TELE_CAFEZINHO_COMBINED_STATE_MUTUAL_ACTIVE);
    assert(strcmp(tele_cafezinho_combined_state_to_string(TELE_CAFEZINHO_COMBINED_STATE_MUTUAL_ACTIVE),
                  "mutual_active") == 0);
}

int main(void)
{
    test_initial_state_is_idle();
    test_local_active_only();
    test_remote_active_only();
    test_mutual_active();
    return 0;
}
