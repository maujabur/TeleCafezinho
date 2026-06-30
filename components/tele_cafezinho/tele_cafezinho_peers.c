#include "tele_cafezinho_peers.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tele_cafezinho.h"

#ifndef TELE_CAFEZINHO_HOST_TEST
#include "esp_log.h"
#include "esp_timer.h"
#endif

#ifndef CONFIG_TELE_CAFEZINHO_MAX_PEERS
#define CONFIG_TELE_CAFEZINHO_MAX_PEERS 16
#endif

typedef struct {
    bool used;
    char device_id[40];
    uint32_t last_seq;
    bool active;
    int64_t last_seen_ms;
    int64_t expires_at_ms;
} tele_cafezinho_peer_t;

#ifndef TELE_CAFEZINHO_HOST_TEST
static const char *TAG = "tele-cafe-peers";
static esp_timer_handle_t s_expire_timer;
#endif
static tele_cafezinho_peer_t s_peers[CONFIG_TELE_CAFEZINHO_MAX_PEERS];

static tele_cafezinho_peer_t *find_peer(const char *device_id)
{
    for (size_t i = 0; i < CONFIG_TELE_CAFEZINHO_MAX_PEERS; ++i) {
        if (s_peers[i].used && strcmp(s_peers[i].device_id, device_id) == 0) {
            return &s_peers[i];
        }
    }
    return NULL;
}

static tele_cafezinho_peer_t *find_free_peer(void)
{
    for (size_t i = 0; i < CONFIG_TELE_CAFEZINHO_MAX_PEERS; ++i) {
        if (!s_peers[i].used) {
            return &s_peers[i];
        }
    }
    return NULL;
}

uint32_t tele_cafezinho_peers_active_count(void)
{
    uint32_t count = 0;
    for (size_t i = 0; i < CONFIG_TELE_CAFEZINHO_MAX_PEERS; ++i) {
        if (s_peers[i].used && s_peers[i].active) {
            count++;
        }
    }
    return count;
}

void tele_cafezinho_peers_clear(void)
{
    memset(s_peers, 0, sizeof(s_peers));
}

esp_err_t tele_cafezinho_peers_apply_message(const tele_cafezinho_signal_msg_t *msg,
                                             const char *own_device_id,
                                             const char *group,
                                             int64_t now_ms)
{
    tele_cafezinho_peer_t *peer = NULL;

    if (!msg || !own_device_id || !group) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strcmp(msg->device_id, own_device_id) == 0 || strcmp(msg->group, group) != 0) {
        return ESP_OK;
    }

    peer = find_peer(msg->device_id);
    if (peer && msg->seq <= peer->last_seq) {
        return ESP_OK;
    }

    if (!peer && msg->active) {
        peer = find_free_peer();
        if (!peer) {
            return ESP_ERR_NO_MEM;
        }
        memset(peer, 0, sizeof(*peer));
        peer->used = true;
        snprintf(peer->device_id, sizeof(peer->device_id), "%s", msg->device_id);
    }
    if (!peer) {
        return ESP_OK;
    }

    peer->last_seq = msg->seq;
    peer->last_seen_ms = now_ms;
    if (msg->active) {
        peer->active = true;
        peer->expires_at_ms = now_ms + ((int64_t)msg->ttl_s * 1000);
    } else {
        memset(peer, 0, sizeof(*peer));
    }
    return ESP_OK;
}

uint32_t tele_cafezinho_peers_expire(int64_t now_ms)
{
    for (size_t i = 0; i < CONFIG_TELE_CAFEZINHO_MAX_PEERS; ++i) {
        if (s_peers[i].used && s_peers[i].active && now_ms >= s_peers[i].expires_at_ms) {
            s_peers[i].active = false;
        }
    }
    return tele_cafezinho_peers_active_count();
}

cJSON *tele_cafezinho_peers_build_json(int64_t now_ms)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *peers = NULL;

    if (!root) {
        return NULL;
    }
    peers = cJSON_AddArrayToObject(root, "peers");
    if (!peers) {
        cJSON_Delete(root);
        return NULL;
    }

    for (size_t i = 0; i < CONFIG_TELE_CAFEZINHO_MAX_PEERS; ++i) {
        if (!s_peers[i].used) {
            continue;
        }
        cJSON *item = cJSON_CreateObject();
        if (!item || !cJSON_AddItemToArray(peers, item)) {
            cJSON_Delete(item);
            cJSON_Delete(root);
            return NULL;
        }
        cJSON_AddStringToObject(item, "device_id", s_peers[i].device_id);
        cJSON_AddBoolToObject(item, "active", s_peers[i].active);
        cJSON_AddNumberToObject(item, "last_seq", s_peers[i].last_seq);
        cJSON_AddNumberToObject(item, "age_ms", (double)(now_ms - s_peers[i].last_seen_ms));
        cJSON_AddNumberToObject(item,
                                "expires_in_ms",
                                (double)(s_peers[i].expires_at_ms > now_ms ?
                                         s_peers[i].expires_at_ms - now_ms : 0));
    }
    return root;
}

#ifndef TELE_CAFEZINHO_HOST_TEST
static void expire_timer_cb(void *ctx)
{
    (void)ctx;
    uint32_t count = tele_cafezinho_peers_expire(esp_timer_get_time() / 1000);
    tele_cafezinho_set_remote_active_count(count);
}

esp_err_t tele_cafezinho_peers_start(void)
{
    if (s_expire_timer) {
        return ESP_OK;
    }

    const esp_timer_create_args_t args = {
        .callback = expire_timer_cb,
        .name = "tcafe_peers",
    };
    esp_err_t err = esp_timer_create(&args, &s_expire_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to create peer expiration timer: %s", esp_err_to_name(err));
        return err;
    }
    return esp_timer_start_periodic(s_expire_timer, 1000000);
}
#else
esp_err_t tele_cafezinho_peers_start(void)
{
    return ESP_OK;
}
#endif
