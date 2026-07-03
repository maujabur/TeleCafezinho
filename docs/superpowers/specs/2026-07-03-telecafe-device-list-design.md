# TeleCafe Device List Design

## Goal

Improve `tools/mqtt_desktop/mqtt_control_center.py` so the generic MQTT Control Center can show TeleCafezinho group context directly in the **Dispositivos** area while keeping the list flat.

The first version must let operators:

- see each device group in the device list;
- sort devices by group;
- search by group;
- see a compact TeleCafe status column for sensor/indication state;
- configure the group source, compact column name, and compact column fields through `tools/mqtt_desktop/config.json`.

## Current Context

The desktop app already keeps a `DeviceInfo` object per MQTT device, extracts metadata such as firmware and session ID from received payloads, and renders a flat `ttk.Treeview` with sortable columns:

`estado | device_id | idade | fw | resumo`

The app also already reads `tools/mqtt_desktop/config.json`, falling back to `config.example.json`, and uses top-level settings for MQTT connection, heartbeat timeout, and technical status refresh.

TeleCafezinho publishes status/config fields named `telecafe.*`. The group is `telecafe.group`, and the main state fields are:

- `telecafe.combined_state`
- `telecafe.local_active`
- `telecafe.remote_active_count`
- `telecafe.last_remote_device_id`

## Configuration

Add an optional `telecafe` section to `tools/mqtt_desktop/config.json` and `config.example.json`.

Default configuration:

```json
{
  "telecafe": {
    "group_field": "telecafe.group",
    "column_name": "telecafe",
    "summary_fields": [
      "telecafe.combined_state",
      "telecafe.local_active",
      "telecafe.remote_active_count"
    ]
  }
}
```

The app must behave as if this default exists when `telecafe` or any nested key is absent.

`group_field` names the payload field used as the device group. The default is `telecafe.group`.

`column_name` names the compact device-list column. The default is `telecafe`. The older `summary_column_name` key remains accepted as a compatibility alias.

`summary_fields` lists field IDs to read and summarize in the compact column. In the first version this is a list of strings only. Object-based field formatting is out of scope.

Invalid config values should fall back safely:

- empty or non-string `group_field` falls back to `telecafe.group`;
- empty or non-string `column_name` falls back to `telecafe`;
- missing, empty, or non-list `summary_fields` falls back to the default field list;
- non-string entries in `summary_fields` are ignored.

## Data Extraction

Add `group` metadata to each `DeviceInfo`, defaulting internally to an empty string. UI rendering should display empty groups as `sem grupo`.

Add helper logic to resolve configured field IDs from device payloads. The helper should support exact field IDs such as `telecafe.group` and should read from known payload sources in a stable priority order that favors live status over retained or command snapshots:

1. `heartbeat`
2. `state`
3. `device.last_get_state_result`
4. `cmd/out.result`, when it is a dictionary
5. `device.last_technical_status_result`

The field resolver should handle both flattened keys like `"telecafe.group"` and nested dictionaries such as:

```json
{
  "telecafe": {
    "group": "mesa-01"
  }
}
```

When a message arrives, update the cached device group through the configured `group_field`. The group should also be available when only retained `state` or `heartbeat` data exists.

## Device List UI

Keep the current flat list. Add columns so the list becomes:

`estado | grupo | device_id | idade | fw | <column_name> | resumo`

Recommended widths:

- `grupo`: wide enough for names like `mesa-01`;
- compact TeleCafe column: wide enough for values like `combined_state=idle`;
- reduce `resumo` width only as needed to fit.

The `grupo` column must:

- display the resolved group, or `sem grupo` when unavailable;
- sort lexicographically by group, with ungrouped devices last;
- participate in text search;
- appear in device row tooltips;
- appear in the selected device details panel.

The compact TeleCafe column must:

- use the configured `column_name` as the heading;
- sort by its rendered text;
- participate in text search;
- appear in row tooltips.

## Compact TeleCafe Rendering

Render `summary_fields` as a compact generic summary of `field=value` pairs joined by ` |`, using the last segment of each field ID as the display name. Example:

`combined_state=idle | remote_active_count=0`

This keeps version one configurable without adding a field-format mini-language. It also ensures every configured field can appear in the column instead of being collapsed into a single operational label.

## Error Handling

Configuration errors must not prevent startup. The app should log a warning for unusable `telecafe` configuration values and continue with defaults.

Missing fields in device payloads are normal. They should render as `sem grupo` or `sem status`, not as errors.

## Testing And Verification

Add focused unit-test coverage if the current desktop app test harness supports it. If no harness exists, isolate pure helpers enough to validate them with a small Python test or direct module-level assertions.

Minimum verification:

- default config is applied when `telecafe` is absent;
- `group_field` resolves `telecafe.group` from flat and nested payloads;
- group appears in search and sort behavior;
- compact rendering shows configured `summary_fields` as stable `field=value` text;
- generic `summary_fields` render stable `field=value` text;
- invalid config values fall back to defaults.

Manual verification:

- run the MQTT desktop app;
- receive retained or live payloads containing `telecafe.group`;
- confirm the list shows `grupo`, sortable by header;
- confirm the compact column heading follows `column_name`;
- confirm devices without TeleCafe data remain readable as `sem grupo` / `sem status`.

## Out Of Scope

- Tree or grouped-section device list.
- Dedicated TeleCafe dashboard.
- In-app editor for `telecafe` display settings.
- Object-based custom formatting for `summary_fields`.
- Firmware changes.
