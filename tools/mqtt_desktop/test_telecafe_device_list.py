import sys
import types
import unittest
from datetime import datetime


class _FakeCTk:
    pass


class _FakeVar:
    def __init__(self, value=None, **_kwargs):
        self._value = value

    def get(self):
        return self._value

    def set(self, value):
        self._value = value


class _FakeTree:
    def __init__(self, columns):
        self.columns = columns

    def __getitem__(self, key):
        if key == "columns":
            return self.columns
        raise KeyError(key)


class _FakeLabel:
    def __init__(self):
        self.text = "-"

    def configure(self, **kwargs):
        if "text" in kwargs:
            self.text = kwargs["text"]

    def cget(self, key):
        if key == "text":
            return self.text
        return None


customtkinter = types.ModuleType("customtkinter")
customtkinter.CTk = _FakeCTk
customtkinter.CTkLabel = object
customtkinter.CTkToplevel = object
customtkinter.BooleanVar = _FakeVar
customtkinter.CTkFont = object
customtkinter.set_appearance_mode = lambda *_args, **_kwargs: None
customtkinter.set_default_color_theme = lambda *_args, **_kwargs: None
sys.modules.setdefault("customtkinter", customtkinter)

tkinter = types.ModuleType("tkinter")
tkinter.END = "end"
tkinter.StringVar = _FakeVar
tkinter.TclError = Exception
tkinter.PhotoImage = object
tkinter_ttk = types.ModuleType("tkinter.ttk")
tkinter_ttk.Treeview = object
tkinter.ttk = tkinter_ttk
sys.modules.setdefault("tkinter", tkinter)
sys.modules.setdefault("tkinter.ttk", tkinter_ttk)

paho = types.ModuleType("paho")
paho_mqtt = types.ModuleType("paho.mqtt")
paho_mqtt_client = types.ModuleType("paho.mqtt.client")
paho_mqtt_client.Client = object
paho_mqtt_client.CallbackAPIVersion = types.SimpleNamespace(VERSION2=2)
sys.modules.setdefault("paho", paho)
sys.modules.setdefault("paho.mqtt", paho_mqtt)
sys.modules.setdefault("paho.mqtt.client", paho_mqtt_client)

from tools.mqtt_desktop.mqtt_control_center import (  # noqa: E402
    App,
    DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS,
    DeviceInfo,
    MessageSnapshot,
    custom_column_text,
    default_split_sash_position,
    device_display_sources_for_device,
    normalize_device_list_display_config,
    resolve_payload_field,
)


class TeleCafeDisplayHelperTest(unittest.TestCase):
    def test_default_split_sash_position_uses_half_width_with_safe_minimum(self):
        self.assertEqual(default_split_sash_position(1400), 700)
        self.assertEqual(default_split_sash_position(600), 320)
        self.assertEqual(default_split_sash_position(2000), 1000)

    def test_normalize_device_list_display_config_uses_defaults_for_missing_config(self):
        cfg = normalize_device_list_display_config({})

        self.assertEqual(cfg["custom_columns"], DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS)

    def test_normalize_device_list_display_config_ignores_invalid_values(self):
        cfg = normalize_device_list_display_config(
            {
                "device_list": {
                    "custom_columns": [
                        {
                            "id": "",
                            "column_name": 12,
                            "fields": ["telecafe.combined_state", 9, ""],
                        }
                    ],
                }
            }
        )

        self.assertEqual(cfg["custom_columns"], DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS)

    def test_normalize_device_list_display_config_accepts_multiple_custom_columns(self):
        cfg = normalize_device_list_display_config(
            {
                "device_list": {
                    "custom_columns": [
                        {
                            "id": "group",
                            "column_name": "grupo",
                            "role": "group",
                            "empty": "sem grupo",
                            "fields": ["telecafe.group"],
                        },
                        {
                            "id": "telecafe",
                            "column_name": "indicacao",
                            "empty": "sem status",
                            "fields": [
                                {"id": "telecafe.combined_state", "label": ""},
                                {"id": "telecafe.local_active", "label": "local"},
                            ],
                        }
                    ],
                }
            }
        )

        self.assertEqual([column["id"] for column in cfg["custom_columns"]], ["group", "telecafe"])
        self.assertEqual(cfg["custom_columns"][0]["role"], "group")
        self.assertEqual(cfg["custom_columns"][0]["fields"], [{"id": "telecafe.group", "label": None}])
        self.assertEqual(cfg["custom_columns"][1]["column_name"], "indicacao")
        self.assertEqual(
            cfg["custom_columns"][1]["fields"],
            [
                {"id": "telecafe.combined_state", "label": ""},
                {"id": "telecafe.local_active", "label": "local"},
            ],
        )

    def test_normalize_device_list_display_config_ignores_legacy_telecafe_section(self):
        cfg = normalize_device_list_display_config(
            {
                "telecafe": {
                    "column_name": "legacy",
                    "summary_fields": ["telecafe.signal_seq"],
                }
            }
        )

        self.assertEqual(cfg["custom_columns"], DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS)

    def test_resolve_payload_field_supports_flat_and_nested_keys(self):
        flat = {"telecafe.group": "mesa-01"}
        nested = {"telecafe": {"group": "mesa-02"}}

        self.assertEqual(resolve_payload_field(flat, "telecafe.group"), "mesa-01")
        self.assertEqual(resolve_payload_field(nested, "telecafe.group"), "mesa-02")
        self.assertIsNone(resolve_payload_field({}, "telecafe.group"))

    def test_display_sources_prioritize_live_heartbeat_over_stale_state(self):
        device = DeviceInfo(device_id="dev-1")
        device.last_get_state_result = {
            "telecafe.group": "mesa-01",
            "telecafe.combined_state": "idle",
        }
        device.last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={"telecafe.group": "mesa-01", "telecafe.combined_state": "idle"},
            payload_raw="{}",
        )
        device.last_messages["heartbeat"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "local_active",
                "telecafe.local_active": True,
            },
            payload_raw="{}",
        )

        sources = device_display_sources_for_device(device)

        self.assertEqual(resolve_payload_field(sources[0].payload, "telecafe.combined_state"), "local_active")
        self.assertEqual(custom_column_text(sources, DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS[0]), "mesa-01")

    def test_group_text_returns_sem_grupo_when_missing(self):
        self.assertEqual(
            custom_column_text(device_display_sources_for_device(DeviceInfo(device_id="dev-1")), DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS[0]),
            "sem grupo",
        )

    def test_single_field_custom_column_renders_value_only(self):
        text = custom_column_text([{"telecafe.group": "mesa-01"}], DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS[0])

        self.assertEqual(text, "mesa-01")

    def test_multi_field_custom_column_uses_compact_labels(self):
        self.assertEqual(
            custom_column_text(
                [
                    {
                        "telecafe.combined_state": "idle",
                        "telecafe.local_active": False,
                        "telecafe.remote_active_count": 0,
                    }
                ],
                DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS[1],
            ),
            "idle | local=False | rem=0",
        )
        self.assertEqual(custom_column_text([{}], DEFAULT_DEVICE_LIST_CUSTOM_COLUMNS[1]), "sem status")

    def test_custom_column_without_labels_uses_field_tail(self):
        text = custom_column_text(
            [{"telecafe.combined_state": "idle", "telecafe.signal_seq": 42}],
            {
                "id": "telecafe",
                "column_name": "telecafe",
                "empty": "sem status",
                "role": "",
                "fields": [
                    {"id": "telecafe.combined_state", "label": None},
                    {"id": "telecafe.signal_seq", "label": None},
                ],
            },
        )

        self.assertEqual(text, "combined_state=idle | signal_seq=42")


class TeleCafeDeviceListMethodTest(unittest.TestCase):
    def make_app_shell(self):
        app = object.__new__(App)
        app.device_list_display_config = normalize_device_list_display_config(
            {
                "device_list": {
                    "custom_columns": [
                        {
                            "id": "group",
                            "column_name": "grupo",
                            "role": "group",
                            "empty": "sem grupo",
                            "fields": ["telecafe.group"],
                        },
                        {
                            "id": "telecafe",
                            "column_name": "indicacao",
                            "empty": "sem status",
                            "fields": [
                                {"id": "telecafe.combined_state", "label": ""},
                                {"id": "telecafe.local_active", "label": "local"},
                                {"id": "telecafe.remote_active_count", "label": "rem"},
                            ],
                        }
                    ],
                }
            }
        )
        app.tree_heading_labels = {"presence": "estado", "group": "grupo", "telecafe": "indicacao"}
        app.pending_cmd_by_id = {}
        app.device_search_var = _FakeVar("")
        app.device_filter_var = _FakeVar("Todos")
        return app

    def test_tree_values_include_configured_custom_columns(self):
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
        self.assertEqual(values[2], "remote_active | rem=2")

    def test_tree_values_prefer_live_heartbeat_over_stale_state(self):
        app = self.make_app_shell()
        device = DeviceInfo(device_id="dev-1", fw="1.0")
        device.last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "idle",
                "telecafe.local_active": False,
            },
            payload_raw="{}",
        )
        device.last_messages["heartbeat"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "local_active",
                "telecafe.local_active": True,
            },
            payload_raw="{}",
        )

        _presence_key, values = app._tree_values_for_device(device)

        self.assertEqual(values[2], "local_active | local=True")

    def test_tree_values_choose_latest_timestamp_per_field(self):
        app = self.make_app_shell()
        device = DeviceInfo(device_id="dev-1", fw="1.0")
        device.last_messages["heartbeat"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3, 10, 0, 0),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "local_active",
            },
            payload_raw="{}",
        )
        device.last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3, 10, 1, 0),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "idle",
            },
            payload_raw="{}",
        )

        _presence_key, values = app._tree_values_for_device(device)

        self.assertEqual(values[2], "idle")

    def test_manifest_values_prefer_live_heartbeat_over_stale_state(self):
        app = self.make_app_shell()

        values = app._manifest_value_sources(
            heartbeat={"telecafe.combined_state": "local_active", "telecafe.local_active": True},
            state_topic={"telecafe.combined_state": "idle", "telecafe.local_active": False},
            get_state={"telecafe.combined_state": "idle"},
            technical_status={},
        )

        self.assertEqual(values["telecafe.combined_state"], "local_active")
        self.assertIs(values["telecafe.local_active"], True)

    def test_manifest_values_choose_latest_timestamp_from_device(self):
        app = self.make_app_shell()
        device = DeviceInfo(device_id="dev-1")
        device.last_messages["heartbeat"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3, 10, 0, 0),
            topic="topic",
            payload_obj={"telecafe.combined_state": "local_active"},
            payload_raw="{}",
        )
        device.last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3, 10, 1, 0),
            topic="topic",
            payload_obj={"telecafe.combined_state": "idle"},
            payload_raw="{}",
        )

        values = app._manifest_value_sources_for_device(device)

        self.assertEqual(values["telecafe.combined_state"], "idle")

    def test_manifest_view_passes_device_to_manifest_sections(self):
        app = self.make_app_shell()
        app.status_manifest_frame = object()
        app.status_technical_manifest_frame = object()
        app.status_value_labels = {
            "card.manifest.value": _FakeLabel(),
            "card.manifest.detail": _FakeLabel(),
        }
        app.status_display_values = {}
        device = DeviceInfo(device_id="dev-1")
        calls = []

        def fake_section(*args):
            calls.append(args)
            return 1

        app._render_status_manifest_section = fake_section

        app._render_status_manifest_view(
            {
                "registry_revision": 1,
                "fields": [
                    {
                        "id": "telecafe.combined_state",
                        "group": "telecafe",
                        "flags": [{"flag": "state"}],
                    },
                    {
                        "id": "telecafe.signal_seq",
                        "group": "telecafe",
                        "flags": [{"flag": "technical"}],
                    },
                ],
            },
            device,
            {},
            {},
            {},
            {},
        )

        self.assertEqual(len(calls), 2)
        self.assertIs(calls[0][5], device)
        self.assertIs(calls[1][5], device)

    def test_sort_key_orders_grouped_devices_before_ungrouped(self):
        app = self.make_app_shell()
        app.tree_sort_column = "group"
        grouped = DeviceInfo(device_id="a")
        grouped.group = "mesa-01"
        ungrouped = DeviceInfo(device_id="b")

        self.assertLess(app._device_sort_key(grouped), app._device_sort_key(ungrouped))

    def test_search_matches_group_and_telecafe_summary(self):
        app = self.make_app_shell()
        app.device_search_var = _FakeVar("rem=2")
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

    def test_tooltip_text_describes_group_and_telecafe_columns(self):
        app = self.make_app_shell()
        app.tree = _FakeTree(("presence", "group", "telecafe", "device_id", "age", "fw", "summary"))
        app.devices = {"dev-1": DeviceInfo(device_id="dev-1")}
        app.devices["dev-1"].last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "local_active",
            },
            payload_raw="{}",
        )

        self.assertEqual(app._device_tree_cell_tooltip_text("dev-1", "#2"), "grupo: mesa-01")
        self.assertEqual(app._device_tree_cell_tooltip_text("dev-1", "#3"), "indicacao: local_active")

    def test_refresh_device_details_sets_group_and_telecafe_summary(self):
        app = self.make_app_shell()
        app.selected_device = "dev-1"
        app.devices = {"dev-1": DeviceInfo(device_id="dev-1")}
        app.detail_value_labels = {
            key: _FakeLabel()
            for key in (
                "device_id",
                "online",
                "last_seen",
                "wifi_ssid",
                "ip",
                "fw",
                "session_id",
                "group",
                "telecafe_summary",
                "hb_ts",
                "hb_uptime",
                "hb_rssi",
                "hb_heap",
                "hb_vbat",
                "state",
                "availability",
                "event",
                "event_message",
                "cmd_ts",
                "cmd_id",
                "cmd_ok",
                "cmd_error",
                "cmd_result",
                "state_payload",
                "availability_payload",
                "seen_payload",
                "heartbeat_payload",
                "config_manifest_payload",
                "status_manifest_payload",
                "commands_manifest_payload",
            )
        }
        app.devices["dev-1"].last_messages["state"] = MessageSnapshot(
            timestamp=datetime(2026, 7, 3),
            topic="topic",
            payload_obj={
                "telecafe.group": "mesa-01",
                "telecafe.combined_state": "mutual_active",
                "telecafe.remote_active_count": 1,
            },
            payload_raw="{}",
        )

        app._refresh_device_details()

        self.assertEqual(app.detail_value_labels["group"].text, "mesa-01")
        self.assertEqual(
            app.detail_value_labels["telecafe_summary"].text,
            "mutual_active | rem=1",
        )


if __name__ == "__main__":
    unittest.main()
