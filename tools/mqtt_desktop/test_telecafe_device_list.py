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
                "column_name": 12,
                "summary_fields": ["telecafe.combined_state", 9, ""],
            }
        )

        self.assertEqual(cfg["group_field"], DEFAULT_TELECAFE_GROUP_FIELD)
        self.assertEqual(cfg["summary_column_name"], DEFAULT_TELECAFE_SUMMARY_COLUMN_NAME)
        self.assertEqual(cfg["summary_fields"], ["telecafe.combined_state"])

    def test_normalize_telecafe_display_config_accepts_column_name(self):
        cfg = normalize_telecafe_display_config({"column_name": "indicacao"})

        self.assertEqual(cfg["summary_column_name"], "indicacao")

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

    def test_default_summary_renders_configured_field_values(self):
        fields = DEFAULT_TELECAFE_SUMMARY_FIELDS

        self.assertEqual(
            telecafe_summary_text(
                [
                    {
                        "telecafe.combined_state": "idle",
                        "telecafe.local_active": False,
                        "telecafe.remote_active_count": 0,
                    }
                ],
                fields,
            ),
            "combined_state=idle | local_active=False | remote_active_count=0",
        )
        self.assertEqual(telecafe_summary_text([{}], fields), "sem status")

    def test_custom_summary_fields_render_field_value_pairs(self):
        text = telecafe_summary_text(
            [{"telecafe.combined_state": "idle", "telecafe.signal_seq": 42}],
            ["telecafe.combined_state", "telecafe.signal_seq"],
        )

        self.assertEqual(text, "combined_state=idle | signal_seq=42")


class TeleCafeDeviceListMethodTest(unittest.TestCase):
    def make_app_shell(self):
        app = object.__new__(App)
        app.telecafe_display_config = normalize_telecafe_display_config(
            {
                "group_field": "telecafe.group",
                "column_name": "indicacao",
                "summary_fields": DEFAULT_TELECAFE_SUMMARY_FIELDS,
            }
        )
        app.pending_cmd_by_id = {}
        app.device_search_var = _FakeVar("")
        app.device_filter_var = _FakeVar("Todos")
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
        self.assertEqual(values[5], "combined_state=remote_active | remote_active_count=2")

    def test_sort_key_orders_grouped_devices_before_ungrouped(self):
        app = self.make_app_shell()
        app.tree_sort_column = "group"
        grouped = DeviceInfo(device_id="a")
        grouped.group = "mesa-01"
        ungrouped = DeviceInfo(device_id="b")

        self.assertLess(app._device_sort_key(grouped), app._device_sort_key(ungrouped))

    def test_search_matches_group_and_telecafe_summary(self):
        app = self.make_app_shell()
        app.device_search_var = _FakeVar("remote_active_count=2")
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
        app.tree = _FakeTree(("presence", "group", "device_id", "age", "fw", "telecafe", "summary"))
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
        self.assertEqual(app._device_tree_cell_tooltip_text("dev-1", "#6"), "indicacao: combined_state=local_active")

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
            "combined_state=mutual_active | remote_active_count=1",
        )


if __name__ == "__main__":
    unittest.main()
