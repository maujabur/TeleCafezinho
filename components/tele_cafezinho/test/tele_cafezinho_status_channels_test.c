#include <assert.h>
#include <string.h>

#define TELE_CAFEZINHO_HOST_TEST 1

#include "cJSON.h"
#include "tele_cafezinho_status.h"
#include "tele_channels.h"
#include "tele_status.h"

int main(void)
{
    cJSON *root = NULL;
    char *text = NULL;
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
    assert((field->flags & TELE_STATUS_FLAG_TECHNICAL) != 0);

    root = cJSON_CreateObject();
    assert(root != NULL);
    assert(tele_status_add_fields_to_json(root, TELE_CHANNEL_FLAG_WEB, TELE_STATUS_FLAG_STATE) == ESP_OK);
    text = cJSON_PrintUnformatted(root);
    assert(text != NULL);
    assert(strstr(text, "\"telecafe.group\":\"default\"") != NULL);
    assert(strstr(text, "\"telecafe.signal_source\":\"gpio\"") != NULL);
    assert(strstr(text, "\"telecafe.combined_state\":\"idle\"") != NULL);
    cJSON_free(text);
    cJSON_Delete(root);

    root = cJSON_CreateObject();
    assert(root != NULL);
    assert(tele_status_add_fields_to_json(root, TELE_CHANNEL_FLAG_MQTT, TELE_STATUS_FLAG_TECHNICAL) == ESP_OK);
    text = cJSON_PrintUnformatted(root);
    assert(text != NULL);
    assert(strstr(text, "\"telecafe.group\":\"default\"") != NULL);
    assert(strstr(text, "\"telecafe.local_active\":false") != NULL);
    assert(strstr(text, "\"telecafe.remote_active_count\":0") != NULL);
    assert(strstr(text, "\"telecafe.combined_state\":\"idle\"") != NULL);
    cJSON_free(text);
    cJSON_Delete(root);

    return 0;
}
