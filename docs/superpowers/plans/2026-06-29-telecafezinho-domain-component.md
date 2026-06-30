# TeleCafezinho Domain Component Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a product-specific `tele_cafezinho` domain component that detects a local active/inactive signal, publishes it through TeleSystem MQTT shared topics, tracks active peers in the same group, and drives semantic LED indication through `tele_indicator`.

**Architecture:** `components/tele_cafezinho` is a local ESP-IDF component owned by this product repository and contains TeleCafezinho business rules: local signal detection, group membership, peer cache, TTL expiration, combined state, status fields, configuration fields, test commands, and shared-topic payload schema. `tele_mqtt` remains a generic TeleSystem transport; this plan adds only a generic `tele_mqtt_get_device_id()` API to the managed component, then uses `tele_mqtt_subscribe_shared()` and `tele_mqtt_publish_shared()` from TeleCafezinho. LED output goes through semantic events registered in `main/app_indicators.c`; the domain component must not call `status_led` directly.

**Tech Stack:** ESP-IDF C components, local `components/tele_cafezinho`, managed TeleSystem components in `managed_components/`, FreeRTOS task/timer, GPIO input, cJSON, `tele_config`, `tele_status`, `tele_commands`, `tele_indicator`, `tele_mqtt` shared topics, host-compiled C tests with lightweight ESP error stubs where practical.

**Shared MQTT Topic:**

```text
{base_topic}/_shared/telecafezinho/signal
```

**Payload Contract v1:**

```json
{
  "schema": "telecafezinho.signal.v1",
  "group": "default",
  "device_id": "TCafe-A1B2C3",
  "signal": "active",
  "source": "gpio",
  "seq": 42,
  "ttl_s": 20
}
```

**Combined State Rules:**

```text
local_active=false, remote_active_count=0 -> idle
local_active=true,  remote_active_count=0 -> local_active
local_active=false, remote_active_count>0 -> remote_active
local_active=true,  remote_active_count>0 -> mutual_active
```

**Out Of Scope For This Plan:**

- Real temperature sensor driver.
- Temperature threshold and hysteresis runtime behavior.
- Control Center grouping/filtering changes.
- Product-specific payload or TeleCafezinho behavior inside `tele_mqtt`.
- Pairing UI, portal branding, product-specific web pages, OTA changes.
- Multi-group membership per device.

---

## Repository Placement And File Structure

This product repository intentionally adds a local component at `components/tele_cafezinho`. ESP-IDF will discover `components/` automatically from the project root; do not put TeleCafezinho domain code in `managed_components/`, because that directory mirrors TeleSystem-managed dependencies.

`managed_components/tele_mqtt` may be modified in this plan because the project owns the upstream TeleSystem source. Keep that change generic and upstreamable: a public getter for the current MQTT device ID, plus host coverage. Do not add TeleCafezinho topic names, payload fields, group logic, or peer behavior to `tele_mqtt`.

Planned file responsibilities:

- `components/tele_cafezinho/include/tele_cafezinho.h`: public lifecycle and state API used by `main`.
- `components/tele_cafezinho/tele_cafezinho.c`: component orchestration, cached state, combined state calculation, indicator updates, and calls into config/status/MQTT/GPIO/commands helpers.
- `components/tele_cafezinho/tele_cafezinho_config.c`: `tele_config` field registration and typed read helpers.
- `components/tele_cafezinho/tele_cafezinho_status.c`: read-only `tele_status` field registration backed by cached domain state.
- `components/tele_cafezinho/tele_cafezinho_mqtt.c`: TeleCafezinho shared-topic payload build/parse, subscription handler, publish helper, and local active republish scheduling.
- `components/tele_cafezinho/tele_cafezinho_peers.c`: fixed-size peer cache, group filtering, sequence handling, active count, and TTL expiration.
- `components/tele_cafezinho/tele_cafezinho_gpio.c`: first local signal source using GPIO polling/debounce.
- `components/tele_cafezinho/tele_cafezinho_commands.c`: operational and test commands.
- `managed_components/tele_mqtt/include/tele_mqtt.h`: generic `tele_mqtt_get_device_id()` declaration.
- `managed_components/tele_mqtt/tele_mqtt.c`: generic `tele_mqtt_get_device_id()` implementation.
- `main/app_indicators.c`: semantic `telecafe.*` indicator source/events.
- `main/main.c`: boot order wiring, with `tele_cafezinho_start()` before `tele_presence_start()`.

---

### Task 1: Component Skeleton And Public API

**Files:**
- Create: `components/tele_cafezinho/include/tele_cafezinho.h`
- Create: `components/tele_cafezinho/tele_cafezinho.c`
- Create: `components/tele_cafezinho/CMakeLists.txt`
- Modify: `main/CMakeLists.txt`
- Modify: `main/main.c`

- [ ] **Step 1: Create public API**

Define the minimal domain API:

```c
#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TELE_CAFEZINHO_COMBINED_STATE_IDLE = 0,
    TELE_CAFEZINHO_COMBINED_STATE_LOCAL_ACTIVE,
    TELE_CAFEZINHO_COMBINED_STATE_REMOTE_ACTIVE,
    TELE_CAFEZINHO_COMBINED_STATE_MUTUAL_ACTIVE,
} tele_cafezinho_combined_state_t;

esp_err_t tele_cafezinho_start(void);
esp_err_t tele_cafezinho_set_local_signal(bool active);
bool tele_cafezinho_get_local_active(void);
uint32_t tele_cafezinho_get_remote_active_count(void);
tele_cafezinho_combined_state_t tele_cafezinho_get_combined_state(void);
const char *tele_cafezinho_combined_state_to_string(tele_cafezinho_combined_state_t state);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Implement minimal state machine**

Implement local state, remote active count, combined state calculation, and string conversion. Do not add MQTT, GPIO, config, status, commands, or indicators yet.

- [ ] **Step 3: Add component build file**

Create `components/tele_cafezinho/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "tele_cafezinho.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer tele_config tele_status tele_commands tele_mqtt tele_indicator espressif__cjson
)
```

This is a local project component under `components/`, not a managed dependency.

- [ ] **Step 4: Wire component into app build**

Add `tele_cafezinho` to `main/CMakeLists.txt` `REQUIRES`.

- [ ] **Step 5: Start component from app boot**

In `main/main.c`, include the new header and call `tele_cafezinho_start()` after `connectivity_controller_start()` and before `tele_presence_start()`. This lets config/status/commands and shared-topic subscriptions be registered before `tele_mqtt_start()` builds manifests and starts the client.

Recommended first integration:

```c
ESP_ERROR_CHECK(connectivity_controller_start());
ESP_ERROR_CHECK(tele_cafezinho_start());
maybe_start_ca_updater_boot_task();
ESP_ERROR_CHECK(tele_presence_start());
```

`tele_mqtt_subscribe_shared()` already supports registration before the MQTT client is connected; this order relies on that existing contract.

- [ ] **Step 6: Build**

Run:

```bash
idf.py build
```

Expected: build succeeds with no behavior change except a new idle component.

---

### Task 2: Host Test For Combined State

**Files:**
- Create: `components/tele_cafezinho/test/tele_cafezinho_state_test.c`
- Optionally modify: `components/tele_cafezinho/tele_cafezinho.c`

- [ ] **Step 1: Write host test**

Create a host test that verifies:

- initial state is `idle`;
- local active only becomes `local_active`;
- remote active only becomes `remote_active`;
- local and remote active becomes `mutual_active`;
- string conversion returns stable values: `idle`, `local_active`, `remote_active`, `mutual_active`.

If direct peer manipulation is not public, expose private test hooks under `#ifdef TELE_CAFEZINHO_HOST_TEST`.

- [ ] **Step 2: Compile and run host test**

Example command:

```bash
gcc -DTELE_CAFEZINHO_HOST_TEST \
    -Icomponents/tele_cafezinho/include \
    components/tele_cafezinho/test/tele_cafezinho_state_test.c \
    components/tele_cafezinho/tele_cafezinho.c \
    -o /tmp/tele_cafezinho_state_test && /tmp/tele_cafezinho_state_test
```

Expected: exit code 0.

---

### Task 3: Runtime Configuration Fields

**Files:**
- Create: `components/tele_cafezinho/tele_cafezinho_config.c`
- Create or modify: `components/tele_cafezinho/include/tele_cafezinho_config.h`
- Modify: `components/tele_cafezinho/CMakeLists.txt`
- Modify: `components/tele_cafezinho/tele_cafezinho.c`

- [ ] **Step 1: Define config IDs**

Use stable public IDs and short NVS keys:

```c
#define TELE_CAFEZINHO_CONFIG_ID_GROUP "telecafe.group"
#define TELE_CAFEZINHO_CONFIG_ID_SIGNAL_SOURCE "telecafe.signal_source"
#define TELE_CAFEZINHO_CONFIG_ID_GPIO_NUM "telecafe.gpio_num"
#define TELE_CAFEZINHO_CONFIG_ID_GPIO_ACTIVE_LEVEL "telecafe.gpio_active_level"
#define TELE_CAFEZINHO_CONFIG_ID_GPIO_DEBOUNCE_MS "telecafe.gpio_debounce_ms"
#define TELE_CAFEZINHO_CONFIG_ID_SIGNAL_TTL_S "telecafe.signal_ttl_s"
#define TELE_CAFEZINHO_CONFIG_ID_SIGNAL_PUBLISH_INTERVAL_S "telecafe.signal_publish_interval_s"
```

Suggested NVS keys:

```text
tc_group
tc_src
tc_gpio
tc_level
tc_deb
tc_ttl
tc_pubint
```

- [ ] **Step 2: Register fields in `tele_config`**

Defaults:

```text
telecafe.group = "default"
telecafe.signal_source = "gpio"
telecafe.gpio_num = -1
telecafe.gpio_active_level = 1
telecafe.gpio_debounce_ms = 150
telecafe.signal_ttl_s = 20
telecafe.signal_publish_interval_s = 5
```

Expose all fields through MQTT. Expose through WEB only if the current portal settings UI can safely render them. Mark fields requiring GPIO reconfiguration as `REBOOT_REQUIRED` unless runtime reconfiguration is implemented in this plan.

- [ ] **Step 3: Add read helpers**

Implement internal helpers:

```c
esp_err_t tele_cafezinho_config_register(void);
esp_err_t tele_cafezinho_config_get_group(char *out, size_t out_size);
esp_err_t tele_cafezinho_config_get_signal_source(char *out, size_t out_size);
esp_err_t tele_cafezinho_config_get_gpio_num(int32_t *out_value);
esp_err_t tele_cafezinho_config_get_gpio_active_level(int32_t *out_value);
esp_err_t tele_cafezinho_config_get_gpio_debounce_ms(uint32_t *out_value);
esp_err_t tele_cafezinho_config_get_signal_ttl_s(uint32_t *out_value);
esp_err_t tele_cafezinho_config_get_publish_interval_s(uint32_t *out_value);
```

- [ ] **Step 4: Register config at component start**

`tele_cafezinho_start()` must call `tele_cafezinho_config_register()` before reading effective values.

- [ ] **Step 5: Build and inspect manifest**

Run:

```bash
idf.py build
```

Then verify via MQTT after flashing that `{base_topic}/{device_id}/meta/config` includes the `telecafe.*` fields.

---

### Task 4: Read-Only Status Fields

**Files:**
- Create: `components/tele_cafezinho/tele_cafezinho_status.c`
- Create or modify: `components/tele_cafezinho/include/tele_cafezinho_status.h`
- Modify: `components/tele_cafezinho/CMakeLists.txt`
- Modify: `components/tele_cafezinho/tele_cafezinho.c`

- [ ] **Step 1: Register status fields**

Register these fields in `tele_status`:

```text
telecafe.group                  string, group=telecafe, flags=STATE|HEARTBEAT
telecafe.signal_source          string, group=telecafe, flags=STATE|TECHNICAL
telecafe.local_active           bool,   group=telecafe, flags=STATE|HEARTBEAT
telecafe.remote_active_count    u32,    group=telecafe, flags=STATE|HEARTBEAT
telecafe.combined_state         string, group=telecafe, flags=STATE|HEARTBEAT
telecafe.last_remote_device_id  string, group=telecafe, flags=STATE|TECHNICAL
telecafe.signal_seq             u32,    group=telecafe, flags=STATE|TECHNICAL
```

Expose fields through MQTT. Expose through WEB only if the local status API already renders `tele_status` fields safely.

- [ ] **Step 2: Implement non-blocking read callbacks**

Callbacks must not allocate large buffers, block on MQTT, or read GPIO directly. They should return cached state from `tele_cafezinho`.

- [ ] **Step 3: Register status at component start**

`tele_cafezinho_start()` must call `tele_cafezinho_status_register()` after config registration. Because `main/main.c` starts `tele_cafezinho` before `tele_presence_start()`, these fields are registered before `tele_mqtt_start()` builds MQTT state, heartbeat, and manifest payloads.

- [ ] **Step 4: Verify state and heartbeat payloads**

After flashing, verify that `state` and `heartbeat` include the selected `telecafe.*` values. `state` is retained; `heartbeat` is periodic.

---

### Task 5: Generic MQTT Device ID Getter

**Files:**
- Modify: `managed_components/tele_mqtt/include/tele_mqtt.h`
- Modify: `managed_components/tele_mqtt/tele_mqtt.c`
- Modify: `managed_components/tele_mqtt/test/tele_mqtt_shared_topic_test.c`

- [ ] **Step 1: Add public API declaration**

Add this declaration to `managed_components/tele_mqtt/include/tele_mqtt.h` near the other public getters:

```c
esp_err_t tele_mqtt_get_device_id(char *out, size_t out_size);
```

Contract:

- returns `ESP_ERR_INVALID_ARG` when `out == NULL` or `out_size == 0`;
- returns `ESP_ERR_INVALID_STATE` when the device ID has not been generated yet;
- copies the current MQTT device ID into `out` with null termination;
- returns `ESP_ERR_INVALID_SIZE` when `out_size` is too small for the full ID plus null terminator;
- does not expose mutable internal storage.

- [ ] **Step 2: Write host test for the getter**

Extend `managed_components/tele_mqtt/test/tele_mqtt_shared_topic_test.c` with a focused test:

```c
static void test_get_device_id_contract(void)
{
    char out[64] = {0};

    tele_mqtt_host_test_reset();
    assert(tele_mqtt_get_device_id(NULL, sizeof(out)) == ESP_ERR_INVALID_ARG);
    assert(tele_mqtt_get_device_id(out, 0) == ESP_ERR_INVALID_ARG);
    assert(tele_mqtt_get_device_id(out, sizeof(out)) == ESP_ERR_INVALID_STATE);

    tele_mqtt_host_test_set_device_id("TCafe-A1B2C3");
    assert(tele_mqtt_get_device_id(out, sizeof(out)) == ESP_OK);
    assert(strcmp(out, "TCafe-A1B2C3") == 0);
    assert(tele_mqtt_get_device_id(out, 4) == ESP_ERR_INVALID_SIZE);
}
```

Add this host-only helper declaration to `managed_components/tele_mqtt/include/tele_mqtt.h` inside the existing `#ifdef TELE_MQTT_HOST_TEST` section:

```c
void tele_mqtt_host_test_set_device_id(const char *device_id);
```

Call `test_get_device_id_contract()` from `main()` in the same test file.

- [ ] **Step 3: Run test and verify it fails before implementation**

Run:

```bash
gcc -DTELE_MQTT_HOST_TEST -DESP_FAIL=0x106 \
    -Imanaged_components/tele_mqtt/include \
    -Imanaged_components/tele_config/test \
    -Imanaged_components/espressif__cjson/cJSON \
    managed_components/tele_mqtt/test/tele_mqtt_shared_topic_test.c \
    managed_components/tele_mqtt/tele_mqtt.c \
    -o /tmp/tele_mqtt_shared_topic_test && /tmp/tele_mqtt_shared_topic_test
```

The `-DESP_FAIL=0x106` define is only needed because the current host `esp_err.h` stub in `managed_components/tele_config/test/esp_err.h` does not define `ESP_FAIL`; remove the command-line define if the stub is updated.

Expected: FAIL because `tele_mqtt_get_device_id()` is declared but not implemented.

- [ ] **Step 4: Implement getter in `tele_mqtt.c`**

Implement after the existing public getter functions:

```c
esp_err_t tele_mqtt_get_device_id(char *out, size_t out_size)
{
    if (!out || out_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_device_id[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }

    size_t len = strlen(s_device_id);
    if (out_size <= len) {
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(out, s_device_id, len + 1);
    return ESP_OK;
}
```

If `s_device_id` is currently compiled only outside `TELE_MQTT_HOST_TEST`, move its declaration outside the `#ifndef TELE_MQTT_HOST_TEST` block so host tests can exercise the getter without duplicating identity logic.

Add the host helper near the existing host helpers in `tele_mqtt.c`:

```c
#ifdef TELE_MQTT_HOST_TEST
void tele_mqtt_host_test_set_device_id(const char *device_id)
{
    if (!device_id) {
        s_device_id[0] = '\0';
        return;
    }
    snprintf(s_device_id, sizeof(s_device_id), "%s", device_id);
}
#endif
```

Update `tele_mqtt_host_test_reset()` to clear `s_device_id[0]`.

- [ ] **Step 5: Run getter test and build firmware**

Run:

```bash
gcc -DTELE_MQTT_HOST_TEST -DESP_FAIL=0x106 \
    -Imanaged_components/tele_mqtt/include \
    -Imanaged_components/tele_config/test \
    -Imanaged_components/espressif__cjson/cJSON \
    managed_components/tele_mqtt/test/tele_mqtt_shared_topic_test.c \
    managed_components/tele_mqtt/tele_mqtt.c \
    -o /tmp/tele_mqtt_shared_topic_test && /tmp/tele_mqtt_shared_topic_test
```

Expected: exit code 0.

Then run:

```bash
idf.py build
```

Expected: build exits 0 and `tele_mqtt_get_device_id()` is available to local components. Keep the patch generic enough to upstream to TeleSystem.

---

### Task 6: Shared MQTT Payload Build And Parse

**Files:**
- Create: `components/tele_cafezinho/tele_cafezinho_mqtt.c`
- Create or modify: `components/tele_cafezinho/include/tele_cafezinho_mqtt.h`
- Modify: `components/tele_cafezinho/CMakeLists.txt`

- [ ] **Step 1: Implement payload builder**

Build compact JSON for `active` and `inactive` signals:

```c
esp_err_t tele_cafezinho_mqtt_build_signal_payload(bool active,
                                                   char *out,
                                                   size_t out_size);
```

Payload must include:

```text
schema
group
device_id
signal
source
seq
ttl_s
```

Use `tele_mqtt_get_device_id()` from Task 5. If it returns `ESP_ERR_INVALID_STATE`, payload building must fail and log a warning; do not duplicate MAC formatting logic inside `tele_cafezinho`.

- [ ] **Step 2: Implement payload parser**

Parse incoming JSON into:

```c
typedef struct {
    char schema[32];
    char group[32];
    char device_id[40];
    bool active;
    char source[16];
    uint32_t seq;
    uint32_t ttl_s;
} tele_cafezinho_signal_msg_t;
```

Reject:

- malformed JSON;
- missing required fields;
- unknown schema;
- `signal` other than `active` or `inactive`;
- empty `group`;
- empty `device_id`;
- zero `ttl_s` for active messages.

- [ ] **Step 3: Add host test for build/parse**

Create a test that:

- builds an active payload and parses it back;
- builds an inactive payload and parses it back;
- rejects invalid signal values;
- rejects missing group/device_id;
- rejects wrong schema.

Suggested command:

```bash
gcc -DTELE_CAFEZINHO_HOST_TEST \
    -Icomponents/tele_cafezinho/include \
    components/tele_cafezinho/test/tele_cafezinho_mqtt_payload_test.c \
    components/tele_cafezinho/tele_cafezinho_mqtt.c \
    -o /tmp/tele_cafezinho_mqtt_payload_test && /tmp/tele_cafezinho_mqtt_payload_test
```

Use the same cJSON include path used by the repository-managed cJSON component:

```bash
gcc -DTELE_CAFEZINHO_HOST_TEST \
    -Icomponents/tele_cafezinho/include \
    -Imanaged_components/tele_config/test \
    -Imanaged_components/espressif__cjson/cJSON \
    components/tele_cafezinho/test/tele_cafezinho_mqtt_payload_test.c \
    components/tele_cafezinho/tele_cafezinho_mqtt.c \
    managed_components/espressif__cjson/cJSON/cJSON.c \
    -o /tmp/tele_cafezinho_mqtt_payload_test && /tmp/tele_cafezinho_mqtt_payload_test
```

---

### Task 7: Peer Cache And TTL Expiration

**Files:**
- Create: `components/tele_cafezinho/tele_cafezinho_peers.c`
- Create or modify: `components/tele_cafezinho/include/tele_cafezinho_peers.h`
- Modify: `components/tele_cafezinho/CMakeLists.txt`
- Modify: `components/tele_cafezinho/tele_cafezinho.c`

- [ ] **Step 1: Implement peer model**

Use a fixed-size cache to avoid heap churn:

```c
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
```

The cache only stores peers from the same group. Group filtering happens before cache update.

- [ ] **Step 2: Apply incoming messages**

Rules:

- ignore messages from own `device_id`;
- ignore messages from another group;
- ignore old or duplicate `seq` per peer;
- `active` creates/updates peer and sets `expires_at_ms = now + ttl_s`;
- `inactive` marks peer inactive or removes it immediately;
- remote active count is recalculated after each update.

- [ ] **Step 3: Expire stale peers**

Run expiration from a periodic FreeRTOS timer or task every 1 second. Expired active peers must become inactive and trigger combined-state recalculation.

- [ ] **Step 4: Track last remote device**

Store `last_remote_device_id` whenever a valid remote active message is accepted.

- [ ] **Step 5: Host test peer rules**

Test:

- own-device messages ignored;
- other-group messages ignored;
- active peer increments count;
- inactive peer decrements count;
- duplicate/old seq ignored;
- expiration decrements count;
- multiple peers count correctly.

---

### Task 8: Shared Topic Integration

**Files:**
- Modify: `components/tele_cafezinho/tele_cafezinho_mqtt.c`
- Modify: `components/tele_cafezinho/tele_cafezinho.c`

- [ ] **Step 1: Subscribe to shared topic**

At `tele_cafezinho_start()`, register:

```c
ESP_ERROR_CHECK(tele_mqtt_subscribe_shared("telecafezinho/signal",
                                           1,
                                           tele_cafezinho_on_shared_signal,
                                           NULL));
```

The handler must parse payload and update the peer cache. It must not block.

- [ ] **Step 2: Publish local signal transitions**

When `tele_cafezinho_set_local_signal(true)` changes the local state from inactive to active, publish an `active` payload immediately.

When it changes from active to inactive, publish an `inactive` payload immediately.

- [ ] **Step 3: Periodically republish active signal**

While local signal is active, publish an `active` payload every `telecafe.signal_publish_interval_s`.

The publish interval must be lower than `telecafe.signal_ttl_s`. If config is invalid, clamp internally and log a warning.

- [ ] **Step 4: Publish QoS and retain behavior**

Use:

```text
QoS: 1
retain: false
```

Do not use retained shared signal messages in this phase. Retained state remains in the normal per-device `state` topic.

- [ ] **Step 5: Recalculate status after every local or remote update**

Whenever local state changes, peer cache changes, or peer expiration occurs, recalculate combined state and update the indicator.

---

### Task 9: GPIO Input Source

**Files:**
- Create: `components/tele_cafezinho/tele_cafezinho_gpio.c`
- Create or modify: `components/tele_cafezinho/include/tele_cafezinho_gpio.h`
- Modify: `components/tele_cafezinho/CMakeLists.txt`
- Modify: `components/tele_cafezinho/tele_cafezinho.c`

- [ ] **Step 1: Configure GPIO when enabled**

If `telecafe.signal_source == "gpio"` and `telecafe.gpio_num >= 0`, configure GPIO as input with internal pull-up and pull-down disabled in the first version. The hardware must provide the electrical idle level, and `telecafe.gpio_active_level` selects active-high or active-low interpretation.

Recommended first version:

- do not assume external pullups;
- document expected electrical level;
- allow `telecafe.gpio_active_level` to define active high or active low.

- [ ] **Step 2: Implement debounce**

Use either:

- polling task every 20–50 ms with stable-time debounce; or
- GPIO interrupt that only schedules debounce work.

For simplicity and robustness in this phase, prefer polling unless power consumption requires interrupt-driven behavior.

- [ ] **Step 3: Call domain setter**

When debounced GPIO active changes, call:

```c
tele_cafezinho_set_local_signal(active);
```

- [ ] **Step 4: Handle disabled GPIO**

If GPIO source is selected but `gpio_num < 0`, keep local signal inactive and log a clear warning once:

```text
TeleCafezinho GPIO source selected but telecafe.gpio_num is disabled
```

- [ ] **Step 5: Build and bench test**

Flash two devices configured with the same group and verify:

- local GPIO active publishes active;
- remote device sees remote active;
- local inactive publishes inactive;
- remote device returns to idle.

---

### Task 10: LED Indicator Events

**Files:**
- Modify: `main/app_indicators.c`
- Optionally modify: `docs/tele_indicator.md` if adding product event examples is desired later
- Modify: `components/tele_cafezinho/tele_cafezinho.c`

- [ ] **Step 1: Register source**

Add a new source:

```c
{.id = "telecafe", .default_priority = 60},
```

Do not reuse `wifi`, `system`, `battery`, or `output`.

- [ ] **Step 2: Register events**

Add persistent events:

```text
telecafe.local_active   -> pulse/breath orange
telecafe.remote_active  -> pulse/breath green
telecafe.mutual_active  -> pulse/breath red
```

Suggested priorities:

```text
telecafe.remote_active  = 55
telecafe.local_active   = 60
telecafe.mutual_active  = 75
```

Keep `system.error` and `battery.low` higher than TeleCafezinho events.

- [ ] **Step 3: Update indicator from combined state**

In the domain component:

```c
tele_indicator_clear_source("telecafe");

switch (combined_state) {
case TELE_CAFEZINHO_COMBINED_STATE_LOCAL_ACTIVE:
    tele_indicator_raise("telecafe", "telecafe.local_active");
    break;
case TELE_CAFEZINHO_COMBINED_STATE_REMOTE_ACTIVE:
    tele_indicator_raise("telecafe", "telecafe.remote_active");
    break;
case TELE_CAFEZINHO_COMBINED_STATE_MUTUAL_ACTIVE:
    tele_indicator_raise("telecafe", "telecafe.mutual_active");
    break;
case TELE_CAFEZINHO_COMBINED_STATE_IDLE:
default:
    break;
}
```

The domain component must never call `status_led_apply_effect()` directly.

- [ ] **Step 4: Bench test priority behavior**

Verify:

- Wi-Fi connected temporary event does not permanently suppress TeleCafezinho;
- battery low beats TeleCafezinho;
- system error beats all.

---

### Task 11: Commands For Test And Operation

**Files:**
- Create: `components/tele_cafezinho/tele_cafezinho_commands.c`
- Create or modify: `components/tele_cafezinho/include/tele_cafezinho_commands.h`
- Modify: `components/tele_cafezinho/CMakeLists.txt`
- Modify: `components/tele_cafezinho/tele_cafezinho.c`

- [ ] **Step 1: Register commands**

Register these commands through `tele_commands`:

```text
telecafe/simulate_signal
telecafe/clear_peers
telecafe/get_peers
```

Expose through MQTT. Expose through WEB only if useful for the local technical UI.

- [ ] **Step 2: Implement `telecafe/simulate_signal`**

Args:

```json
{
  "active": true,
  "duration_s": 10
}
```

Behavior:

- if `duration_s` is absent or zero, set local simulated state until changed again;
- if `duration_s > 0`, set active and automatically return inactive after duration;
- mark command as mutating.

- [ ] **Step 3: Implement `telecafe/clear_peers`**

Behavior:

- clear peer cache;
- remote active count becomes zero;
- combined state is recalculated;
- mark command as mutating.

- [ ] **Step 4: Implement `telecafe/get_peers`**

Return compact JSON:

```json
{
  "peers": [
    {
      "device_id": "TCafe-A1B2C3",
      "active": true,
      "last_seq": 42,
      "age_ms": 3000,
      "expires_in_ms": 17000
    }
  ]
}
```

Do not include peers from other groups because they should never be stored.

- [ ] **Step 5: Verify command manifests**

After flashing, verify `{base_topic}/{device_id}/meta/commands` includes the three commands with group `telecafe`.

---

### Task 12: Documentation

**Files:**
- Create: `docs/tele_cafezinho.md`
- Modify: `docs/arquitetura_index.md`
- Modify: `README.md` only if the repository branch is intended to include TeleCafezinho as a first-class product profile

- [ ] **Step 1: Document the component boundary**

`docs/tele_cafezinho.md` must state:

- `tele_cafezinho` is product/domain logic;
- `tele_mqtt` only transports shared-topic messages;
- local signal source is initially GPIO;
- temperature source is planned but out of scope for this phase;
- LED goes through `tele_indicator`;
- remote activity uses explicit active/inactive plus periodic active republish and TTL expiration.

- [ ] **Step 2: Document configs**

Include a table:

| Config ID | Type | Default | Notes |
|---|---:|---:|---|
| `telecafe.group` | string | `default` | group membership |
| `telecafe.signal_source` | enum/string | `gpio` | first phase supports `gpio` |
| `telecafe.gpio_num` | i32 | `-1` | disabled when negative |
| `telecafe.gpio_active_level` | i32 | `1` | `1` active-high, `0` active-low |
| `telecafe.gpio_debounce_ms` | u32 | `150` | stable input time |
| `telecafe.signal_ttl_s` | u32 | `20` | remote expiration TTL |
| `telecafe.signal_publish_interval_s` | u32 | `5` | active republish interval |

- [ ] **Step 3: Document status fields**

Document the `telecafe.*` status fields and where they appear: `state`, `heartbeat`, and technical status if applicable.

- [ ] **Step 4: Document MQTT payload**

Include topic and payload examples for active and inactive messages.

- [ ] **Step 5: Document commands**

Document command names, args, and response shape.

- [ ] **Step 6: Link from architecture index**

Add `docs/tele_cafezinho.md` under a product/domain-specific section, not under generic MQTT docs.

---

### Task 13: Version, Build, And End-To-End Verification

**Files:**
- Modify: `main/app_firmware_version.h`

- [ ] **Step 1: Bump firmware version**

Update version label to describe the feature, for example:

```c
#define APP_FIRMWARE_VERSION_SEMVER "0.2.00"
#define APP_FIRMWARE_VERSION_LABEL "TeleCafezinho domain component"
```

Use the project’s real versioning policy if different; this repository is currently at `0.1.00`, so `0.2.00` is the expected next feature version unless a release branch says otherwise.

- [ ] **Step 2: Run host tests**

Run all available TeleCafezinho tests:

```bash
/tmp/tele_cafezinho_state_test
/tmp/tele_cafezinho_mqtt_payload_test
/tmp/tele_cafezinho_peers_test
```

Or rebuild and run them from source as documented in each task.

- [ ] **Step 3: Build firmware**

Run:

```bash
idf.py build
```

Expected: build exits 0.

- [ ] **Step 4: Flash two devices**

Configure both devices with the same group:

```json
{"name":"config/set","cmd_id":"tc-group-1","args":{"id":"telecafe.group","value":"mesa-01"}}
```

Set valid GPIO configs if needed:

```json
{"name":"config/set","cmd_id":"tc-gpio-1","args":{"id":"telecafe.gpio_num","value":4}}
{"name":"config/set","cmd_id":"tc-level-1","args":{"id":"telecafe.gpio_active_level","value":1}}
```

Reboot if any field is marked `reboot_required`.

- [ ] **Step 5: Verify local active**

Activate GPIO on device A.

Expected on A:

```text
telecafe.local_active = true
telecafe.remote_active_count = 0
telecafe.combined_state = local_active
LED = orange pulse/breath
```

Expected MQTT shared message:

```json
{
  "schema": "telecafezinho.signal.v1",
  "group": "mesa-01",
  "device_id": "...A...",
  "signal": "active",
  "source": "gpio",
  "seq": 1,
  "ttl_s": 20
}
```

- [ ] **Step 6: Verify remote active**

Device B receives A’s message.

Expected on B:

```text
telecafe.local_active = false
telecafe.remote_active_count = 1
telecafe.combined_state = remote_active
LED = green pulse/breath
```

- [ ] **Step 7: Verify mutual active**

Activate GPIO on B while A remains active.

Expected on both:

```text
telecafe.local_active = true
telecafe.remote_active_count >= 1
telecafe.combined_state = mutual_active
LED = red pulse/breath
```

- [ ] **Step 8: Verify inactive transition**

Deactivate GPIO on A.

Expected:

- A publishes `inactive`;
- B removes or marks A inactive;
- B returns to `local_active` if its own GPIO is still active, otherwise idle.

- [ ] **Step 9: Verify TTL fallback**

Disconnect power or Wi-Fi from A while active.

Expected on B:

- A remains remote active only until TTL expires;
- after TTL, B recalculates combined state and LED updates.

- [ ] **Step 10: Verify group isolation**

Set A to `mesa-01` and B to `mesa-02`.

Expected:

- both may receive shared messages;
- both ignore messages from the other group;
- remote active count remains zero.

---

## Acceptance Criteria

- `tele_mqtt` is used only through its generic shared-topic API.
- `tele_mqtt` exposes `tele_mqtt_get_device_id()` as a generic upstreamable TeleSystem API, with no TeleCafezinho-specific behavior.
- `tele_cafezinho` contains all product-specific business logic.
- The normal per-device MQTT topic contract remains unchanged.
- `telecafe.*` config fields appear in `meta/config`.
- `telecafe.*` status fields appear in `state`, `heartbeat`, and `meta/status` according to flags.
- Shared topic payloads use `telecafezinho.signal.v1`.
- Devices ignore their own messages.
- Devices ignore messages from other groups.
- Remote peer state expires by TTL if active republish stops.
- LED indication uses `tele_indicator`, never `status_led` directly.
- At least two devices in the same group demonstrate idle, local active, remote active, mutual active, inactive, and TTL fallback behavior.
- `idf.py build` exits 0.

## Notes For Future Plans

Future plans may add:

- temperature sensor source;
- `telecafe.temperature_threshold_c` and `telecafe.temperature_hysteresis_c` runtime behavior;
- Control Center grouping/filtering by `telecafe.group`;
- pairing workflow;
- product-specific portal page;
- manufacturing defaults and provisioning flow for paired devices.
