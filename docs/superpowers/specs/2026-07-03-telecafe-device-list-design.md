# TeleCafe Device List Design

## Goal

Improve `tools/mqtt_desktop/mqtt_control_center.py` so the generic MQTT Control Center can show TeleCafezinho group context directly in the **Dispositivos** area while keeping the list flat and configurable.

Operators should be able to:

- see each device group in the device list;
- sort and search by group;
- see compact TeleCafe sensor/indication state;
- configure device-list custom columns through `tools/mqtt_desktop/config.json`.

## Configuration

Display configuration lives under `device_list.custom_columns`. The older top-level `telecafe` section is ignored for device-list display.

Default configuration:

```json
{
  "device_list": {
    "custom_columns": [
      {
        "id": "group",
        "column_name": "grupo",
        "role": "group",
        "empty": "sem grupo",
        "width": 92,
        "fields": ["telecafe.group"]
      },
      {
        "id": "telecafe",
        "column_name": "telecafe",
        "empty": "sem status",
        "width": 120,
        "fields": [
          {"id": "telecafe.combined_state", "label": ""},
          {"id": "telecafe.local_active", "label": "local"},
          {"id": "telecafe.remote_active_count", "label": "rem"}
        ]
      }
    ]
  }
}
```

Each custom column supports:

- `id`: stable column ID, not colliding with built-in columns such as `presence`, `device_id`, `age`, `fw`, or `summary`;
- `column_name`: visible heading;
- `fields`: one or more payload field IDs, as strings or objects with `id` and optional `label`;
- `empty`: text to show when no configured field has a value;
- `role`: optional behavior marker. `role: "group"` gives group sorting and selected-device group detail behavior;
- `width`: optional Treeview column width.

If a column has one field, the device list shows only the value. If a column has multiple fields, each rendered part uses the field label when configured, the last segment of the field ID when no label exists, or just the value when `label` is the empty string.

Invalid config values should fall back safely:

- missing, non-list, or fully invalid `custom_columns` falls back to the default columns;
- invalid, duplicate, or reserved column IDs are ignored;
- columns without usable fields are ignored;
- invalid field entries are ignored;
- missing `column_name`, `empty`, `role`, or `width` values receive safe defaults.

## Data Extraction

Custom column fields resolve from known payload sources using the latest timestamp per field:

1. `heartbeat`
2. `state`
3. `device.last_get_state_result`
4. `cmd/out.result`, when it is a dictionary
5. `device.last_technical_status_result`

The field resolver supports both flattened keys like `"telecafe.group"` and nested dictionaries such as:

```json
{
  "telecafe": {
    "group": "mesa-01"
  }
}
```

## Device List UI

Keep the current flat list. Add custom columns after `estado`, so the default list becomes:

`estado | grupo | telecafe | device_id | idade | fw | resumo`

The default `grupo` custom column must:

- display the resolved group, or `sem grupo` when unavailable;
- sort lexicographically by group, with ungrouped devices last;
- participate in text search;
- appear in device row tooltips;
- appear in the selected device details panel.

Any custom column must:

- use the configured `column_name` as the heading;
- sort by its rendered text;
- participate in text search;
- appear in row tooltips.

## Compact Rendering

Render configured multi-field columns as compact parts joined by ` | `. Examples:

`idle | local=False | rem=0`

`combined_state=idle | signal_seq=42`

This keeps the display configurable without adding a field-format mini-language. A single-field column renders only the value, so the default group column shows `01` instead of `group=01`.

## Testing And Verification

Minimum verification:

- default config is applied when `device_list` is absent;
- legacy top-level `telecafe` display config is ignored;
- custom field IDs resolve from flat and nested payloads;
- group appears in search and sort behavior;
- single-field custom columns render only values;
- compact rendering uses configured labels for multi-field columns;
- newer timestamps win per field across heartbeat, state, and command/result snapshots;
- invalid config values fall back to defaults.

Manual verification:

- run the MQTT desktop app;
- receive retained or live payloads containing `telecafe.group`;
- confirm the list shows `grupo`, sortable by header;
- confirm custom column headings follow `column_name`;
- confirm devices without TeleCafe data remain readable as `sem grupo` / `sem status`.

## Out Of Scope

- Tree or grouped-section device list.
- Dedicated TeleCafe dashboard.
- In-app management of custom-column settings.
- Firmware changes.
