#include "tele_cafezinho_status.h"

#include <stdbool.h>
#include <stdint.h>

#include "tele_cafezinho.h"
#include "tele_channels.h"
#include "tele_status.h"

#define TELE_CAFEZINHO_CHANNEL_FLAGS (TELE_CHANNEL_FLAG_MQTT | TELE_CHANNEL_FLAG_WEB)

static const char *read_group(void *ctx)
{
    (void)ctx;
    return tele_cafezinho_get_group();
}

static const char *read_signal_source(void *ctx)
{
    (void)ctx;
    return tele_cafezinho_get_signal_source();
}

static bool read_local_active(void *ctx)
{
    (void)ctx;
    return tele_cafezinho_get_local_active();
}

static uint32_t read_remote_active_count(void *ctx)
{
    (void)ctx;
    return tele_cafezinho_get_remote_active_count();
}

static const char *read_combined_state(void *ctx)
{
    (void)ctx;
    return tele_cafezinho_combined_state_to_string(tele_cafezinho_get_combined_state());
}

static const char *read_last_remote_device_id(void *ctx)
{
    (void)ctx;
    return tele_cafezinho_get_last_remote_device_id();
}

static uint32_t read_signal_seq(void *ctx)
{
    (void)ctx;
    return tele_cafezinho_get_signal_seq();
}

static const tele_status_field_t s_fields[] = {
    {
        .id = "telecafe.group",
        .label = "TeleCafezinho group",
        .description = "TeleCafezinho group membership.",
        .group = "telecafe",
        .type = TELE_STATUS_TYPE_STRING,
        .channel_flags = TELE_CAFEZINHO_CHANNEL_FLAGS,
        .flags = TELE_STATUS_FLAG_STATE | TELE_STATUS_FLAG_HEARTBEAT | TELE_STATUS_FLAG_TECHNICAL,
        .read.string = read_group,
    },
    {
        .id = "telecafe.signal_source",
        .label = "TeleCafezinho signal source",
        .description = "Local active signal source.",
        .group = "telecafe",
        .type = TELE_STATUS_TYPE_STRING,
        .channel_flags = TELE_CAFEZINHO_CHANNEL_FLAGS,
        .flags = TELE_STATUS_FLAG_STATE | TELE_STATUS_FLAG_TECHNICAL,
        .read.string = read_signal_source,
    },
    {
        .id = "telecafe.local_active",
        .label = "TeleCafezinho local active",
        .description = "Whether the local signal is active.",
        .group = "telecafe",
        .type = TELE_STATUS_TYPE_BOOL,
        .channel_flags = TELE_CAFEZINHO_CHANNEL_FLAGS,
        .flags = TELE_STATUS_FLAG_STATE | TELE_STATUS_FLAG_HEARTBEAT | TELE_STATUS_FLAG_TECHNICAL,
        .read.boolean = read_local_active,
    },
    {
        .id = "telecafe.remote_active_count",
        .label = "TeleCafezinho remote active count",
        .description = "Count of active remote peers in the same group.",
        .group = "telecafe",
        .type = TELE_STATUS_TYPE_U32,
        .channel_flags = TELE_CAFEZINHO_CHANNEL_FLAGS,
        .flags = TELE_STATUS_FLAG_STATE | TELE_STATUS_FLAG_HEARTBEAT | TELE_STATUS_FLAG_TECHNICAL,
        .read.u32 = read_remote_active_count,
    },
    {
        .id = "telecafe.combined_state",
        .label = "TeleCafezinho combined state",
        .description = "Combined local and remote activity state.",
        .group = "telecafe",
        .type = TELE_STATUS_TYPE_STRING,
        .channel_flags = TELE_CAFEZINHO_CHANNEL_FLAGS,
        .flags = TELE_STATUS_FLAG_STATE | TELE_STATUS_FLAG_HEARTBEAT | TELE_STATUS_FLAG_TECHNICAL,
        .read.string = read_combined_state,
    },
    {
        .id = "telecafe.last_remote_device_id",
        .label = "TeleCafezinho last remote device",
        .description = "Last accepted remote active peer.",
        .group = "telecafe",
        .type = TELE_STATUS_TYPE_STRING,
        .channel_flags = TELE_CAFEZINHO_CHANNEL_FLAGS,
        .flags = TELE_STATUS_FLAG_STATE | TELE_STATUS_FLAG_TECHNICAL,
        .read.string = read_last_remote_device_id,
    },
    {
        .id = "telecafe.signal_seq",
        .label = "TeleCafezinho signal sequence",
        .description = "Last local signal sequence number.",
        .group = "telecafe",
        .type = TELE_STATUS_TYPE_U32,
        .channel_flags = TELE_CAFEZINHO_CHANNEL_FLAGS,
        .flags = TELE_STATUS_FLAG_STATE | TELE_STATUS_FLAG_TECHNICAL,
        .read.u32 = read_signal_seq,
    },
};

esp_err_t tele_cafezinho_status_register(void)
{
    return tele_status_register_fields(s_fields, sizeof(s_fields) / sizeof(s_fields[0]));
}
