#include <assert.h>

#define TELE_CAFEZINHO_HOST_TEST 1

#include "tele_cafezinho_status.h"
#include "tele_channels.h"
#include "tele_status.h"

int main(void)
{
    const tele_status_field_t *field = NULL;

    assert(tele_cafezinho_status_register() == ESP_OK);

    field = tele_status_find_field("telecafe.group");
    assert(field != NULL);
    assert((field->channel_flags & TELE_CHANNEL_FLAG_MQTT) != 0);
    assert((field->channel_flags & TELE_CHANNEL_FLAG_WEB) != 0);

    field = tele_status_find_field("telecafe.combined_state");
    assert(field != NULL);
    assert((field->channel_flags & TELE_CHANNEL_FLAG_MQTT) != 0);
    assert((field->channel_flags & TELE_CHANNEL_FLAG_WEB) != 0);

    return 0;
}
