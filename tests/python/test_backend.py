from __future__ import annotations

import json
import os
import sys
import threading
import unittest
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[2]
BACKEND_DIR = ROOT / "src" / "python" / "backend"
sys.path.insert(0, str(BACKEND_DIR))

import server  # noqa: E402

CLI = Path(os.environ.get("MS2_CLI", ROOT / "build" / "ms2_cli"))
EXAMPLES = ROOT / "examples"


class BackendTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if not CLI.exists():
            raise unittest.SkipTest(f"CLI not found: {CLI}")
        cls.httpd = server.create_server("127.0.0.1", 0, CLI, EXAMPLES)
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()
        host, port = cls.httpd.server_address[:2]
        cls.base_url = f"http://{host}:{port}"

    @classmethod
    def tearDownClass(cls) -> None:
        cls.httpd.shutdown()
        cls.httpd.server_close()
        cls.thread.join(timeout=5)

    def get_json(self, path: str) -> tuple[int, dict]:
        with urlopen(self.base_url + path) as response:
            return response.status, json.loads(response.read())

    def post_json(self, path: str, payload: dict) -> tuple[int, dict]:
        request = Request(
            self.base_url + path,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        try:
            with urlopen(request) as response:
                return response.status, json.loads(response.read())
        except HTTPError as error:
            return error.code, json.loads(error.read())

    def test_health(self) -> None:
        status, payload = self.get_json("/api/health")
        self.assertEqual(status, 200)
        self.assertEqual(payload["status"], "ok")

    def test_examples(self) -> None:
        status, payload = self.get_json("/api/examples")
        self.assertEqual(status, 200)
        names = {example["name"] for example in payload["examples"]}
        self.assertIn("01-cantilever", names)
        self.assertIn("04-continuous-beam", names)

    def test_example_input(self) -> None:
        status, payload = self.get_json("/api/examples/01-cantilever")
        self.assertEqual(status, 200)
        self.assertEqual(len(payload["nodes"]), 2)
        self.assertEqual(len(payload["elements"]), 1)

    def test_solve_simply_supported_udl(self) -> None:
        model = {
            "nodes": [
                {"id": 1, "x": 0.0, "y": 0.0},
                {"id": 2, "x": 4.0, "y": 0.0},
            ],
            "elements": [
                {
                    "id": 1,
                    "node_i": 1,
                    "node_j": 2,
                    "E": 210e9,
                    "I": 8.333333333333333e-6,
                }
            ],
            "supports": [
                {"node": 1, "fix_v": True},
                {"node": 2, "fix_v": True},
            ],
            "distributed_loads": [
                {"element": 1, "q_i": -1000.0, "q_j": -1000.0}
            ],
        }

        status, payload = self.post_json("/api/solve?samples=201", model)

        self.assertEqual(status, 200)
        self.assertAlmostEqual(payload["reactions"][0]["fy"], 2000.0)
        self.assertAlmostEqual(payload["reactions"][1]["fy"], 2000.0)
        self.assertAlmostEqual(payload["maxima"]["moment"], 2000.0)
        self.assertIn("elements", payload)

    def test_invalid_model_returns_error(self) -> None:
        status, payload = self.post_json("/api/solve", {"nodes": []})
        self.assertEqual(status, 400)
        self.assertIn("error", payload)

    def test_unknown_example_returns_404(self) -> None:
        try:
            self.get_json("/api/examples/does-not-exist")
        except HTTPError as error:
            self.assertEqual(error.code, 404)
        else:  # pragma: no cover
            self.fail("expected HTTPError")


if __name__ == "__main__":
    unittest.main()
