#!/usr/bin/env python3
"""Local HTTP API that exposes the C++ beam solver to the Electron GUI.

The backend uses only the Python standard library. Each solve request is written
to a temporary JSON file, passed to the compiled ``ms2_cli`` executable and the
resulting JSON is returned unchanged to the renderer.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

# Resolve project paths relative to this file, so the backend also works when
# Electron is launched from a different working directory.
ROOT = Path(__file__).resolve().parents[3]
DEFAULT_CLI = ROOT / "build" / "ms2_cli"
DEFAULT_EXAMPLES = ROOT / "examples"

# Protect the local server against oversized payloads and hung solver runs.
MAX_BODY_BYTES = 5_000_000
SOLVER_TIMEOUT_SECONDS = 30


class ApiError(Exception):
    """Error that carries the HTTP status code returned to the renderer."""

    def __init__(self, status: int, message: str) -> None:
        super().__init__(message)
        self.status = status
        self.message = message


def run_solver(model: dict, cli_path: Path, samples: int) -> dict:
    """Run the compiled solver in a temporary directory and return its JSON.

    ``ApiError`` is used for every failure mode so the HTTP handler can map the
    problem to a useful status code (missing binary, timeout or invalid model).
    """
    if not cli_path.exists():
        raise ApiError(500, f"Solver CLI not found at {cli_path}")

    # TemporaryDirectory removes the input/output files even when the solver fails.
    with tempfile.TemporaryDirectory(prefix="ms2-") as temporary:
        input_path = Path(temporary) / "input.json"
        output_path = Path(temporary) / "output.json"
        input_path.write_text(json.dumps(model), encoding="utf-8")

        try:
            completed = subprocess.run(
                [
                    str(cli_path),
                    "--input",
                    str(input_path),
                    "--output",
                    str(output_path),
                    "--samples",
                    str(samples),
                ],
                capture_output=True,
                text=True,
                timeout=SOLVER_TIMEOUT_SECONDS,
                check=False,
            )
        except subprocess.TimeoutExpired as exc:
            raise ApiError(504, "Solver timed out") from exc

        # The CLI writes validation errors to stderr; surface that text to the GUI.
        if completed.returncode != 0:
            message = completed.stderr.strip() or completed.stdout.strip() or "Solver failed"
            raise ApiError(400, message)

        try:
            return json.loads(output_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ApiError(500, f"Invalid solver output: {exc}") from exc


def list_examples(examples_dir: Path) -> list[dict]:
    """Return the example names and their optional descriptions."""
    examples = []
    for input_path in sorted(examples_dir.glob("*/input.json")):
        name = input_path.parent.name
        description = ""
        expected_path = input_path.parent / "expected.json"
        if expected_path.exists():
            try:
                description = json.loads(expected_path.read_text(encoding="utf-8")).get(
                    "description", ""
                )
            except (OSError, json.JSONDecodeError):
                description = ""
        examples.append({"name": name, "description": description})
    return examples


def load_example(examples_dir: Path, name: str) -> dict:
    """Load one example by name, rejecting paths outside the examples folder."""
    allowed = {path.parent.name for path in examples_dir.glob("*/input.json")}
    if name not in allowed:
        raise ApiError(404, f"Example not found: {name}")
    input_path = examples_dir / name / "input.json"
    try:
        return json.loads(input_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ApiError(500, f"Could not read example {name}: {exc}") from exc


class MS2Server(ThreadingHTTPServer):
    """HTTP server that keeps the solver and examples paths for each request."""

    def __init__(self, address: tuple[str, int], cli_path: Path, examples_dir: Path) -> None:
        super().__init__(address, RequestHandler)
        self.cli_path = cli_path
        self.examples_dir = examples_dir


class RequestHandler(BaseHTTPRequestHandler):
    """Route the small JSON API used by the Electron renderer."""

    server_version = "MS2Backend/0.1"

    def send_json(self, status: int, payload: object) -> None:
        """Send a JSON response with permissive CORS headers for local use."""
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()
        self.wfile.write(body)

    def read_json(self) -> dict:
        """Read and validate a JSON object from the request body."""
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0:
            raise ApiError(400, "Empty request body")
        if length > MAX_BODY_BYTES:
            raise ApiError(413, "Request body is too large")
        raw = self.rfile.read(length)
        try:
            document = json.loads(raw)
        except json.JSONDecodeError as exc:
            raise ApiError(400, f"Invalid JSON: {exc}") from exc
        if not isinstance(document, dict):
            raise ApiError(400, "The request body must be a JSON object")
        return document

    def do_OPTIONS(self) -> None:  # noqa: N802
        # CORS preflight: the renderer and backend use different origins.
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self) -> None:  # noqa: N802
        """Serve health checks and the bundled example models."""
        try:
            parsed = urlparse(self.path)
            if parsed.path == "/api/health":
                self.send_json(200, {"status": "ok"})
                return
            if parsed.path == "/api/examples":
                self.send_json(200, {"examples": list_examples(self.server.examples_dir)})
                return
            if parsed.path.startswith("/api/examples/"):
                name = parsed.path.rsplit("/", 1)[-1]
                self.send_json(200, load_example(self.server.examples_dir, name))
                return
            raise ApiError(404, f"Unknown route: {parsed.path}")
        except ApiError as error:
            self.send_json(error.status, {"error": error.message})
        except Exception as error:  # pragma: no cover - defensive boundary
            self.send_json(500, {"error": str(error)})

    def do_POST(self) -> None:  # noqa: N802
        """Validate the request, run the solver and return its JSON output."""
        try:
            parsed = urlparse(self.path)
            if parsed.path != "/api/solve":
                raise ApiError(404, f"Unknown route: {parsed.path}")

            # `samples` controls how many points each element contributes to the diagrams.
            samples_text = parse_qs(parsed.query).get("samples", ["101"])[0]
            try:
                samples = int(samples_text)
            except ValueError as exc:
                raise ApiError(400, "samples must be an integer") from exc
            if samples < 2 or samples > 100_000:
                raise ApiError(400, "samples must be between 2 and 100000")

            model = self.read_json()
            result = run_solver(model, self.server.cli_path, samples)
            self.send_json(200, result)
        except ApiError as error:
            self.send_json(error.status, {"error": error.message})
        except Exception as error:  # pragma: no cover - defensive boundary
            self.send_json(500, {"error": str(error)})

    def log_message(self, format: str, *args: object) -> None:
        # Keep the default HTTP log format but prefix it for the Electron console.
        print(f"[backend] {self.address_string()} - {format % args}", file=sys.stderr)


def create_server(host: str, port: int, cli_path: Path, examples_dir: Path) -> MS2Server:
    """Build a configured server; kept separate so tests can use an ephemeral port."""
    return MS2Server((host, port), cli_path, examples_dir)


def main() -> int:
    """Parse CLI/environment options and serve until interrupted."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument(
        "--cli",
        default=os.environ.get("MS2_CLI", str(DEFAULT_CLI)),
        help="Path to the ms2_cli executable",
    )
    parser.add_argument(
        "--examples",
        default=os.environ.get("MS2_EXAMPLES", str(DEFAULT_EXAMPLES)),
        help="Path to the examples directory",
    )
    args = parser.parse_args()

    server = create_server(args.host, args.port, Path(args.cli), Path(args.examples))
    host, port = server.server_address[:2]
    print(f"MS2 backend listening on http://{host}:{port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping MS2 backend", file=sys.stderr)
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
