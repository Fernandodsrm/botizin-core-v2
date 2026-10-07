import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('observer', Path(__file__).resolve().parents[1] / 'ci/external_observer.py')
observer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(observer)


class HistoryTests(unittest.TestCase):
    def test_missing_is_unknown(self):
        s = observer.summarize({}, 10000)
        self.assertIsNone(s['last_heartbeat_age_seconds'])
        self.assertIsNone(s['minimum_reported_free_heap_bytes'])

    def test_old_field_is_not_retimestamped(self):
        s = observer.summarize({'uptime_seconds': [{'ts': 1000, 'value': '10'}, {'ts': 61000, 'value': '70'}],
                               'boot_id': [{'ts': 1000, 'value': 'a'}, {'ts': 61000, 'value': 'b'}],
                               'ram_internal_free_bytes': [{'ts': 1000, 'value': '200000'}]}, 91000)
        self.assertEqual(s['last_heartbeat_age_seconds'], 30)
        self.assertEqual(s['largest_observed_report_gap_seconds'], 60)
        self.assertEqual(s['observed_boot_transitions'], 1)
        self.assertEqual(s['latest_fields']['ram_internal_free_bytes']['ts'], 1000)


if __name__ == '__main__':
    unittest.main()
