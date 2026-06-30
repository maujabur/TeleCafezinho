# TeleCafezinho Domain Component

`tele_cafezinho` is product/domain logic for TeleCafezinho. It owns local signal detection, group membership, peer state, combined state, status/config fields, test commands, and LED intent.

`tele_mqtt` remains a generic transport. TeleCafezinho uses shared topics through `tele_mqtt_subscribe_shared()` and `tele_mqtt_publish_shared()`, and reads the generic MQTT device ID through `tele_mqtt_get_device_id()`.

The first local signal source is GPIO. Temperature sensing, thresholds, and hysteresis are planned future work. LED output goes through `tele_indicator`; this component never calls `status_led` directly.

Remote activity uses explicit `active` and `inactive` messages. Active devices periodically republish their active signal, and receivers expire stale peers by TTL if messages stop.

## Config

| Config ID | Type | Default | Notes |
|---|---:|---:|---|
| `telecafe.group` | string | `default` | Group membership. |
| `telecafe.signal_source` | string | `gpio` | First phase supports `gpio`. |
| `telecafe.gpio_num` | i32 | `-1` | Disabled when negative. |
| `telecafe.gpio_active_level` | i32 | `1` | `1` active-high, `0` active-low. |
| `telecafe.gpio_debounce_ms` | u32 | `150` | Stable input time. |
| `telecafe.signal_ttl_s` | u32 | `20` | Remote expiration TTL. |
| `telecafe.signal_publish_interval_s` | u32 | `5` | Active republish interval. |

## Status

The component registers these MQTT status fields:

| Status ID | Type | Notes |
|---|---:|---|
| `telecafe.group` | string | Current group. |
| `telecafe.signal_source` | string | Current local source. |
| `telecafe.local_active` | bool | Local signal state. |
| `telecafe.remote_active_count` | u32 | Active peers in the same group. |
| `telecafe.combined_state` | string | `idle`, `local_active`, `remote_active`, or `mutual_active`. |
| `telecafe.last_remote_device_id` | string | Last accepted remote active peer. |
| `telecafe.signal_seq` | u32 | Last local publish sequence. |

## MQTT Shared Payload

Topic:

```text
{base_topic}/_shared/telecafezinho/signal
```

Active example:

```json
{
  "schema": "telecafezinho.signal.v1",
  "group": "mesa-01",
  "device_id": "TCafe-A1B2C3",
  "signal": "active",
  "source": "gpio",
  "seq": 42,
  "ttl_s": 20
}
```

Inactive uses the same schema with `"signal": "inactive"`.

## Commands

`telecafe/simulate_signal`

```json
{"active": true, "duration_s": 10}
```

Sets a simulated local signal. If `duration_s` is absent or zero, the simulated state remains until changed again.

`telecafe/clear_peers`

Clears the remote peer cache and recalculates combined state.

`telecafe/get_peers`

Returns:

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
