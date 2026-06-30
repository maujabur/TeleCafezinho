#include "tele_cafezinho_commands.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cJSON.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "tele_cafezinho.h"
#include "tele_channels.h"
#include "tele_commands.h"

static const char *TAG = "tele-cafe-cmd";
static esp_timer_handle_t s_simulation_timer;

static const tele_command_arg_t s_simulate_args[] = {
    {
        .id = "active",
        .type = TELE_COMMAND_ARG_BOOL,
        .required = true,
    },
    {
        .id = "duration_s",
        .type = TELE_COMMAND_ARG_U32,
        .required = false,
        .min.u32 = 0,
        .max.u32 = 86400,
    },
};

static void simulation_timer_cb(void *ctx)
{
    (void)ctx;
    (void)tele_cafezinho_set_local_signal(false);
}

static esp_err_t ensure_simulation_timer(void)
{
    if (s_simulation_timer) {
        return ESP_OK;
    }

    const esp_timer_create_args_t args = {
        .callback = simulation_timer_cb,
        .name = "tcafe_sim",
    };
    return esp_timer_create(&args, &s_simulation_timer);
}

static esp_err_t handle_simulate_signal(const cJSON *args, const char **out_error)
{
    const cJSON *active = cJSON_GetObjectItemCaseSensitive(args, "active");
    const cJSON *duration_s = cJSON_GetObjectItemCaseSensitive(args, "duration_s");
    uint32_t duration = 0;

    if (!cJSON_IsBool(active)) {
        *out_error = "invalid_active";
        return ESP_ERR_INVALID_ARG;
    }
    if (duration_s && !cJSON_IsNumber(duration_s)) {
        *out_error = "invalid_duration_s";
        return ESP_ERR_INVALID_ARG;
    }
    if (duration_s) {
        duration = (uint32_t)duration_s->valuedouble;
    }

    if (s_simulation_timer) {
        (void)esp_timer_stop(s_simulation_timer);
    }

    ESP_RETURN_ON_ERROR(tele_cafezinho_set_local_signal(cJSON_IsTrue(active)),
                        TAG,
                        "set simulated signal failed");

    if (cJSON_IsTrue(active) && duration > 0) {
        ESP_RETURN_ON_ERROR(ensure_simulation_timer(), TAG, "simulation timer create failed");
        ESP_RETURN_ON_ERROR(esp_timer_start_once(s_simulation_timer,
                                                 (uint64_t)duration * 1000000ULL),
                            TAG,
                            "simulation timer start failed");
    }
    return ESP_OK;
}

static esp_err_t handle_command(const char *cmd_name,
                                const cJSON *args,
                                cJSON **out_result,
                                const char **out_error,
                                uint32_t channel_flags,
                                void *ctx)
{
    (void)channel_flags;
    (void)ctx;

    if (strcmp(cmd_name, "telecafe/simulate_signal") == 0) {
        return handle_simulate_signal(args, out_error);
    }
    if (strcmp(cmd_name, "telecafe/clear_peers") == 0) {
        tele_cafezinho_clear_peers();
        return ESP_OK;
    }
    if (strcmp(cmd_name, "telecafe/get_peers") == 0) {
        if (!out_result) {
            return ESP_ERR_INVALID_ARG;
        }
        *out_result = tele_cafezinho_build_peers_json();
        if (!*out_result) {
            *out_error = "no_memory";
            return ESP_ERR_NO_MEM;
        }
        return ESP_OK;
    }

    *out_error = "unknown_command";
    return ESP_ERR_NOT_FOUND;
}

static const tele_command_t s_commands[] = {
    {
        .name = "telecafe/simulate_signal",
        .label = "Simulate TeleCafezinho signal",
        .description = "Sets a simulated local active signal for testing.",
        .group = "telecafe",
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
        .flags = TELE_COMMAND_FLAG_MUTATING,
        .args = s_simulate_args,
        .arg_count = sizeof(s_simulate_args) / sizeof(s_simulate_args[0]),
        .handler = handle_command,
    },
    {
        .name = "telecafe/clear_peers",
        .label = "Clear TeleCafezinho peers",
        .description = "Clears the remote peer cache.",
        .group = "telecafe",
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
        .flags = TELE_COMMAND_FLAG_MUTATING,
        .handler = handle_command,
    },
    {
        .name = "telecafe/get_peers",
        .label = "Get TeleCafezinho peers",
        .description = "Returns the remote peer cache.",
        .group = "telecafe",
        .channel_flags = TELE_CHANNEL_FLAG_MQTT,
        .flags = 0,
        .handler = handle_command,
    },
};

esp_err_t tele_cafezinho_commands_register(void)
{
    return tele_commands_register(s_commands, sizeof(s_commands) / sizeof(s_commands[0]));
}
