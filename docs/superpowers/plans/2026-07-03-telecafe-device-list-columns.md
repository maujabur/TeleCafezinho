# TeleCafe Device List Columns Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add configurable TeleCafezinho group and compact status columns to the MQTT desktop device list.

**Architecture:** Keep the existing flat `ttk.Treeview`. Add pure helper functions for TeleCafe config normalization, payload field lookup, and compact summary rendering, then wire them into `DeviceInfo`, config loading, list values, sorting, search, tooltips, and details. Tests target the pure helpers and lightweight `App` methods without creating a Tk window.

**Tech Stack:** Python 3, `unittest`, `customtkinter`, `ttk.Treeview`, existing JSON config files.

---

## File Map

- Modify `tools/mqtt_desktop/mqtt_control_center.py`: add TeleCafe display defaults/helpers, cache device group, load config, update tree columns, search/sort/tooltips/details.
- Modify `tools/mqtt_desktop/config.example.json`: document default `telecafe` display configuration.
- Modify `tools/mqtt_desktop/config.json`: add the same local default block for this workspace.
- Create `tools/mqtt_desktop/test_telecafe_device_list.py`: unit tests for helper behavior and method-level list/search/sort output.
- Modify `.gitignore`: keep `.superpowers/` ignored; `.codex/` is already ignored.

---

### Task 1: Pure TeleCafe Display Helpers

**Files:**
- Modify: `tools/mqtt_desktop/mqtt_control_center.py`
- Create: `tools/mqtt_desktop/test_telecafe_device_list.py`

- [ ] **Step 1: Write failing helper tests**

Create `tools/mqtt_desktop/test_telecafe_device_list.py` with:

```python
import unittest
from datetime import datetime

from mqtt_control_center import (
    DEFAULT_TELECAFE_GROUP_FIELD,
    DEFAULT_TELECAFE_SUMMARY_COLUMN_NAME,
    DEFAULT_TELECAFE_SUMMARY_FIELDS,
    DeviceInfo,
    MessageSnapshot,
    normalize_telecafe_display_config,
    resolve_payload_field,
    telecafe_display_sources_for_device,
    telecafe_group_text,
    telecafe_summary_text,
)


class TeleCafeDisplayHelperTest(unittest.TestCase):
    def test_normalize_telecafe_display_config_uses_defaults_for_missing_config(self):
        cfg = normalize_telecafe_display_config({})

        self.assertEqual(cfg["group_field"], DEFAULT_TELECAFE_GROUP_FIELD)
        self.assertEqual(cfg["summary_column_name"], DEFAULT_TELECAFE_SUMMARY_COLUMN_NAME)
        self.assertEqual(cfg["summary_fields"], DEFAULT_TELECAFE_SUMMARY_FIELDS)

    def test_normalize_telecafe_display_config_ignores_invalid_values(self):
        cfg = normalize_telecafe_display_config(
            {
                "group_field": "",
                "summary_column_name": 12,
                "summary_fields": ["telecafe.combined_state", 9, ""],
            }
        )

        self.assertEqual(cfg["group_field"], DEFAULT_TELECAFE_GROUP_FIELD)
        self.assertEqual(cfg["summary_column_name"], DEFAULT_TELECAFE_SUMMARY_COLUMN_NAME)
        self.assertEqual(cfg["summary_fields"], ["telecafe.combined_state"])

    def test_resolve_payload_field_supports_flat_and_nested_keys(self):
        flat = {"telecafe.group": "mesa-01"}
        nested = {"telecafe": {"group": "mesa-02"}}

        self.assertEqual(resolve_payload_field(flat, "telecafe.group"), "mesa-01")
        self.assertEqual(resolve_payload_field(nested, "telecafe.group"), "mesa-02")
        self.assertIsNone(resolve_payload_field({}, "telecafe.group"))

    def test_display_sources_prioritize_get_state_before_retained_payloads(self):
        device = DeviceInfo(device_id="dev-1")
        device.last_get_state_result = {"telecafe.group": "mesa-live"}
        device.last_technical_status_result = {"telecafe.group": "mesa-tech"}
        device.last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={"telecafe.group": "mesa-retained"},
            payload_raw="{}",
        )

        sources = telecafe_display_sources_for_device(device)

        self.assertEqual(resolve_payload_field(sources[0], "telecafe.group"), "mesa-live")
        self.assertEqual(telecafe_group_text(device, "telecafe.group"), "mesa-live")

    def test_group_text_returns_sem_grupo_when_missing(self):
        self.assertEqual(telecafe_group_text(DeviceInfo(device_id="dev-1"), "telecafe.group"), "sem grupo")

    def test_default_summary_renders_operational_states(self):
        fields = DEFAULT_TELECAFE_SUMMARY_FIELDS

        self.assertEqual(telecafe_summary_text([{"telecafe.combined_state": "idle"}], fields), "idle")
        self.assertEqual(telecafe_summary_text([{"telecafe.combined_state": "local_active"}], fields), "local ativo")
        self.assertEqual(
            telecafe_summary_text(
                [{"telecafe.combined_state": "remote_active", "telecafe.remote_active_count": 2}],
                fields,
            ),
            "remoto 2",
        )
        self.assertEqual(
            telecafe_summary_text(
                [{"telecafe.combined_state": "mutual_active", "telecafe.remote_active_count": 3}],
                fields,
            ),
            "mutuo 3",
        )
        self.assertEqual(telecafe_summary_text([{}], fields), "sem status")

    def test_custom_summary_fields_render_field_value_pairs(self):
        text = telecafe_summary_text(
            [{"telecafe.combined_state": "idle", "telecafe.signal_seq": 42}],
            ["telecafe.combined_state", "telecafe.signal_seq"],
        )

        self.assertEqual(text, "combined_state=idle | signal_seq=42")


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run helper tests and verify failure**

Run:

```bash
python3 -m unittest tools.mqtt_desktop.test_telecafe_device_list -v
```

Expected: FAIL or import error because the TeleCafe helper constants/functions do not exist yet.

- [ ] **Step 3: Add minimal helper implementation**

In `tools/mqtt_desktop/mqtt_control_center.py`, near existing constants and dataclasses, add:

```python
DEFAULT_TELECAFE_GROUP_FIELD = "telecafe.group"
DEFAULT_TELECAFE_SUMMARY_COLUMN_NAME = "telecafe"
DEFAULT_TELECAFE_SUMMARY_FIELDS = [
    "telecafe.combined_state",
    "telecafe.local_active",
    "telecafe.remote_active_count",
]
DEFAULT_TELECAFE_DISPLAY_CONFIG = {
    "group_field": DEFAULT_TELECAFE_GROUP_FIELD,
    "summary_column_name": DEFAULT_TELECAFE_SUMMARY_COLUMN_NAME,
    "summary_fields": DEFAULT_TELECAFE_SUMMARY_FIELDS,
}
```

Add `group: str = ""` to `DeviceInfo`.

Add pure helpers:

```python
def normalize_telecafe_display_config(raw_config: Any) -> Dict[str, Any]:
    config = raw_config if isinstance(raw_config, dict) else {}
    group_field = config.get("group_field")
    if not isinstance(group_field, str) or not group_field.strip():
        group_field = DEFAULT_TELECAFE_GROUP_FIELD
    else:
        group_field = group_field.strip()

    summary_column_name = config.get("summary_column_name")
    if not isinstance(summary_column_name, str) or not summary_column_name.strip():
        summary_column_name = DEFAULT_TELECAFE_SUMMARY_COLUMN_NAME
    else:
        summary_column_name = summary_column_name.strip()

    raw_fields = config.get("summary_fields")
    summary_fields = []
    if isinstance(raw_fields, list):
        summary_fields = [item.strip() for item in raw_fields if isinstance(item, str) and item.strip()]
    if not summary_fields:
        summary_fields = list(DEFAULT_TELECAFE_SUMMARY_FIELDS)

    return {
        "group_field": group_field,
        "summary_column_name": summary_column_name,
        "summary_fields": summary_fields,
    }


def resolve_payload_field(payload: Dict[str, Any], field_id: str) -> Any:
    if not isinstance(payload, dict) or not field_id:
        return None
    if field_id in payload:
        return payload[field_id]
    current: Any = payload
    for part in field_id.split("."):
        if not isinstance(current, dict) or part not in current:
            return None
        current = current[part]
    return current


def telecafe_display_sources_for_device(device: DeviceInfo) -> list[Dict[str, Any]]:
    sources: list[Dict[str, Any]] = []
    for payload in (
        device.last_get_state_result or {},
        _snapshot_payload(device, "state"),
        _snapshot_payload(device, "heartbeat"),
        _cmd_out_result_payload(device),
        device.last_technical_status_result or {},
    ):
        if isinstance(payload, dict):
            sources.append(payload)
    return sources


def _snapshot_payload(device: DeviceInfo, msg_type: str) -> Dict[str, Any]:
    snap = device.last_messages.get(msg_type)
    if snap and isinstance(snap.payload_obj, dict):
        return snap.payload_obj
    return {}


def _cmd_out_result_payload(device: DeviceInfo) -> Dict[str, Any]:
    cmd_out = _snapshot_payload(device, "cmd/out")
    result = cmd_out.get("result")
    return result if isinstance(result, dict) else {}


def telecafe_group_text(device: DeviceInfo, group_field: str) -> str:
    for payload in telecafe_display_sources_for_device(device):
        value = resolve_payload_field(payload, group_field)
        if value not in (None, ""):
            return str(value)
    return "sem grupo"


def telecafe_summary_text(sources: list[Dict[str, Any]], summary_fields: list[str]) -> str:
    values = {field_id: _first_payload_value(sources, field_id) for field_id in summary_fields}
    if summary_fields == DEFAULT_TELECAFE_SUMMARY_FIELDS:
        return _default_telecafe_summary(values)
    parts = []
    for field_id in summary_fields:
        value = values.get(field_id)
        if value not in (None, ""):
            parts.append(f"{field_id.split('.')[-1]}={value}")
    return " | ".join(parts) if parts else "sem status"


def _first_payload_value(sources: list[Dict[str, Any]], field_id: str) -> Any:
    for payload in sources:
        value = resolve_payload_field(payload, field_id)
        if value not in (None, ""):
            return value
    return None


def _default_telecafe_summary(values: Dict[str, Any]) -> str:
    combined_state = values.get("telecafe.combined_state")
    local_active = _truthy(values.get("telecafe.local_active"))
    remote_count = _int_or_none(values.get("telecafe.remote_active_count"))

    if combined_state == "idle":
        return "idle"
    if combined_state == "local_active" or (local_active and not remote_count):
        return "local ativo"
    if combined_state == "remote_active":
        return f"remoto {remote_count}" if remote_count is not None else "remoto"
    if combined_state == "mutual_active":
        return f"mutuo {remote_count}" if remote_count is not None else "mutuo"
    return "sem status"


def _truthy(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        return value.strip().lower() in {"true", "1", "sim", "yes", "on"}
    return bool(value)


def _int_or_none(value: Any) -> Optional[int]:
    try:
        return int(value)
    except (TypeError, ValueError):
        return None
```

- [ ] **Step 4: Run helper tests and verify pass**

Run:

```bash
python3 -m unittest tools.mqtt_desktop.test_telecafe_device_list -v
```

Expected: PASS.

- [ ] **Step 5: Commit helper slice**

```bash
git add tools/mqtt_desktop/mqtt_control_center.py tools/mqtt_desktop/test_telecafe_device_list.py
git commit -m "feat: add telecafe display helpers"
```

---

### Task 2: Load Config And Wire Device List Values

**Files:**
- Modify: `tools/mqtt_desktop/mqtt_control_center.py`
- Modify: `tools/mqtt_desktop/test_telecafe_device_list.py`

- [ ] **Step 1: Add failing method-level tests**

Append to `tools/mqtt_desktop/test_telecafe_device_list.py`:

```python
class TeleCafeDeviceListMethodTest(unittest.TestCase):
    def make_app_shell(self):
        from mqtt_control_center import App

        app = object.__new__(App)
        app.telecafe_display_config = normalize_telecafe_display_config(
            {
                "group_field": "telecafe.group",
                "summary_column_name": "indicacao",
                "summary_fields": DEFAULT_TELECAFE_SUMMARY_FIELDS,
            }
        )
        app.pending_cmd_by_id = {}
        app.device_search_var = type("Var", (), {"get": lambda self: ""})()
        app.device_filter_var = type("Var", (), {"get": lambda self: "Todos"})()
        return app

    def test_tree_values_include_group_and_telecafe_summary(self):
        app = self.make_app_shell()
        device = DeviceInfo(device_id="dev-1", fw="1.0")
        device.last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "remote_active",
                "telecafe.remote_active_count": 2,
            },
            payload_raw="{}",
        )

        _presence_key, values = app._tree_values_for_device(device)

        self.assertEqual(values[1], "mesa-01")
        self.assertEqual(values[5], "remoto 2")

    def test_sort_key_orders_grouped_devices_before_ungrouped(self):
        app = self.make_app_shell()
        app.tree_sort_column = "group"
        grouped = DeviceInfo(device_id="a")
        grouped.group = "mesa-01"
        ungrouped = DeviceInfo(device_id="b")

        self.assertLess(app._device_sort_key(grouped), app._device_sort_key(ungrouped))

    def test_search_matches_group_and_telecafe_summary(self):
        app = self.make_app_shell()
        app.device_search_var = type("Var", (), {"get": lambda self: "remoto 2"})()
        device = DeviceInfo(device_id="dev-1", online=True)
        device.last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "remote_active",
                "telecafe.remote_active_count": 2,
            },
            payload_raw="{}",
        )

        self.assertTrue(app._device_matches_current_filter(device))
```

- [ ] **Step 2: Run tests and verify failure**

Run:

```bash
python3 -m unittest tools.mqtt_desktop.test_telecafe_device_list -v
```

Expected: FAIL because tree values, sort, and search do not include TeleCafe columns yet.

- [ ] **Step 3: Wire config, values, search, and sort**

In `App.__init__`, before `_build_ui()`:

```python
self.telecafe_display_config = normalize_telecafe_display_config({})
```

In `_load_example_or_local_config`, after technical settings are read:

```python
self.telecafe_display_config = normalize_telecafe_display_config(config_data.get("telecafe"))
```

Add methods to `App`:

```python
def _telecafe_group_for_device(self, device: DeviceInfo) -> str:
    group = telecafe_group_text(device, str(self.telecafe_display_config["group_field"]))
    device.group = "" if group == "sem grupo" else group
    return group

def _telecafe_summary_for_device(self, device: DeviceInfo) -> str:
    return telecafe_summary_text(
        telecafe_display_sources_for_device(device),
        list(self.telecafe_display_config["summary_fields"]),
    )
```

Update tree columns in `_build_ui`:

```python
cols = ("presence", "group", "device_id", "age", "fw", "telecafe", "summary")
widths = {"presence": 78, "group": 92, "device_id": 142, "age": 82, "fw": 72, "telecafe": 104, "summary": 260}
heading_labels = {
    "device_id": "device_id",
    "presence": "estado",
    "group": "grupo",
    "age": "idade",
    "fw": "fw",
    "telecafe": str(self.telecafe_display_config["summary_column_name"]),
    "summary": "resumo",
}
```

Update `_device_matches_current_filter` haystack to include:

```python
self._telecafe_group_for_device(device),
self._telecafe_summary_for_device(device),
```

Update `_device_sort_key`:

```python
if col == "group":
    group = self._telecafe_group_for_device(device)
    return (group == "sem grupo", group.casefold(), device.device_id.casefold())
if col == "telecafe":
    return self._telecafe_summary_for_device(device).casefold()
```

Update `_tree_values_for_device`:

```python
values = (
    presence_text,
    self._telecafe_group_for_device(device),
    device.device_id,
    self._format_age(device.last_seen),
    device.fw,
    self._telecafe_summary_for_device(device),
    self._device_summary(device),
)
```

- [ ] **Step 4: Run tests and verify pass**

Run:

```bash
python3 -m unittest tools.mqtt_desktop.test_telecafe_device_list -v
```

Expected: PASS.

- [ ] **Step 5: Commit device-list slice**

```bash
git add tools/mqtt_desktop/mqtt_control_center.py tools/mqtt_desktop/test_telecafe_device_list.py
git commit -m "feat: show telecafe fields in device list"
```

---

### Task 3: Tooltips, Details, And Example Config

**Files:**
- Modify: `tools/mqtt_desktop/mqtt_control_center.py`
- Modify: `tools/mqtt_desktop/config.example.json`
- Modify: `tools/mqtt_desktop/config.json`
- Modify: `.gitignore`

- [ ] **Step 1: Add config blocks**

Add this block to both config files:

```json
  "telecafe": {
    "group_field": "telecafe.group",
    "summary_column_name": "telecafe",
    "summary_fields": [
      "telecafe.combined_state",
      "telecafe.local_active",
      "telecafe.remote_active_count"
    ]
  }
```

- [ ] **Step 2: Add details row**

In `_build_details_panel`, add rows after `Session`:

```python
("Grupo", "group"),
("TeleCafe", "telecafe_summary"),
```

In `_refresh_device_details`, after setting `session_id`:

```python
self._set_detail("group", self._telecafe_group_for_device(device))
self._set_detail("telecafe_summary", self._telecafe_summary_for_device(device))
```

- [ ] **Step 3: Add tooltip text for new columns**

In `_device_tree_cell_tooltip_text`, add:

```python
if column_name == "group":
    return f"grupo: {self._telecafe_group_for_device(device)}"
if column_name == "telecafe":
    return f"{self.telecafe_display_config['summary_column_name']}: {self._telecafe_summary_for_device(device)}"
```

- [ ] **Step 4: Ensure local tooling ignores remain**

Keep `.gitignore` containing:

```gitignore
.codex/
.superpowers/
```

- [ ] **Step 5: Run tests**

Run:

```bash
python3 -m unittest tools.mqtt_desktop.test_telecafe_device_list -v
```

Expected: PASS.

- [ ] **Step 6: Commit integration slice**

```bash
git add .gitignore tools/mqtt_desktop/mqtt_control_center.py tools/mqtt_desktop/config.example.json tools/mqtt_desktop/config.json
git commit -m "feat: configure telecafe device columns"
```

---

### Task 4: Final Verification

**Files:**
- Verify: `tools/mqtt_desktop/mqtt_control_center.py`
- Verify: `tools/mqtt_desktop/test_telecafe_device_list.py`

- [ ] **Step 1: Run unit tests**

Run:

```bash
python3 -m unittest tools.mqtt_desktop.test_telecafe_device_list -v
```

Expected: all tests PASS.

- [ ] **Step 2: Run syntax compile**

Run:

```bash
python3 -m py_compile tools/mqtt_desktop/mqtt_control_center.py tools/mqtt_desktop/test_telecafe_device_list.py
```

Expected: exits 0 with no output.

- [ ] **Step 3: Inspect diff**

Run:

```bash
git diff --stat HEAD
git status --short
```

Expected: no uncommitted implementation files unless intentionally left for review.
