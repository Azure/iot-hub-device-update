#!/usr/bin/env python3
"""Mock ADU (Azure Device Update) server for testing the Gen2 agent.

Simulates the ADU REST API that the ADU Direct communication provider polls.
Uses only Python standard library modules (no external dependencies).
"""

from __future__ import annotations

import argparse
import json
import logging
import os
import re
import sys
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from typing import Any
from urllib.parse import urlparse, parse_qs

logger = logging.getLogger(__name__)

# Default deployment manifest (Gen1-compatible format)
DEFAULT_MANIFEST: dict[str, Any] = {
    "workflowId": "wf-001",
    "updateId": {
        "provider": "Contoso",
        "name": "VacuumFirmware",
        "version": "2.0.0",
    },
    "instructions": {
        "steps": [
            {
                "type": "inline",
                "handler": "microsoft/script:2",
                "handlerProperties": {
                    "id": "install",
                    "scriptFileName": "install.sh",
                    "arguments": "--force",
                    "timeout": 60,
                },
                "files": ["f1"],
                "installedCriteria": "2.0.0",
            }
        ]
    },
    "files": {
        "f1": {
            "fileName": "install.sh",
            "sizeInBytes": 1024,
            "hashes": {"sha256": "abc123fake"},
        }
    },
}

# Dummy file content served when no --content-dir is specified
DUMMY_FILE_CONTENT = b"#!/bin/bash\necho 'Mock install script'\nexit 0\n"


class ServerState:
    """Shared mutable state for the mock server."""

    def __init__(
        self,
        manifest: dict[str, Any],
        no_deployment: bool,
        content_dir: Path | None,
        port: int,
    ) -> None:
        self.manifest = manifest
        self.deployment_available = not no_deployment
        self.content_dir = content_dir
        self.port = port
        # Track completed deployments per device
        self.completed_deployments: set[tuple[str, str]] = set()
        # Logged device states and results
        self.device_states: list[dict[str, Any]] = []
        self.deployment_results: list[dict[str, Any]] = []


class ADURequestHandler(BaseHTTPRequestHandler):
    """HTTP request handler implementing mock ADU REST API endpoints."""

    server_state: ServerState  # Set at class level before server starts

    # Route patterns
    _HEALTH_RE = re.compile(r"^/health$")
    _ADMIN_STATUS_RE = re.compile(r"^/admin/status$")
    _DEPLOYMENTS_RE = re.compile(r"^/devices/(?P<deviceId>[^/]+)/deployments$")
    _STATE_RE = re.compile(r"^/devices/(?P<deviceId>[^/]+)/state$")
    _RESULT_RE = re.compile(
        r"^/devices/(?P<deviceId>[^/]+)/deployments/(?P<workflowId>[^/]+)/result$"
    )
    _FILE_URL_RE = re.compile(
        r"^/devices/(?P<deviceId>[^/]+)/files/(?P<fileId>[^/]+)/url$"
    )
    _FILE_CONTENT_RE = re.compile(r"^/files/(?P<fileId>[^/]+)/content$")

    def do_GET(self) -> None:
        """Handle GET requests."""
        path = urlparse(self.path).path

        if self._HEALTH_RE.match(path):
            self._handle_health()
        elif self._ADMIN_STATUS_RE.match(path):
            self._handle_admin_status()
        elif m := self._DEPLOYMENTS_RE.match(path):
            self._handle_get_deployments(m.group("deviceId"))
        elif m := self._FILE_URL_RE.match(path):
            self._handle_get_file_url(m.group("deviceId"), m.group("fileId"))
        elif m := self._FILE_CONTENT_RE.match(path):
            self._handle_get_file_content(m.group("fileId"))
        else:
            self._send_json(404, {"error": "Not found", "path": path})

    def do_PUT(self) -> None:
        """Handle PUT requests."""
        path = urlparse(self.path).path

        if m := self._STATE_RE.match(path):
            self._handle_put_state(m.group("deviceId"))
        else:
            self._send_json(404, {"error": "Not found", "path": path})

    def do_POST(self) -> None:
        """Handle POST requests."""
        path = urlparse(self.path).path

        if m := self._RESULT_RE.match(path):
            self._handle_post_result(m.group("deviceId"), m.group("workflowId"))
        else:
            self._send_json(404, {"error": "Not found", "path": path})

    # ── Endpoint handlers ───────────────────────────────────────────────

    def _handle_health(self) -> None:
        """GET /health → 200 {"status": "ok"}"""
        self._send_json(200, {"status": "ok"})

    def _handle_admin_status(self) -> None:
        """GET /admin/status → 200 with all device states and results."""
        state = self.server_state
        self._send_json(200, {
            "deployment_available": state.deployment_available,
            "completed_deployments": [
                {"deviceId": d, "workflowId": w}
                for d, w in state.completed_deployments
            ],
            "device_states": state.device_states,
            "deployment_results": state.deployment_results,
        })

    def _handle_get_deployments(self, device_id: str) -> None:
        """GET /devices/{deviceId}/deployments?api-version=...

        Returns deployment manifest or 204 if no deployment is pending.
        """
        state = self.server_state
        workflow_id = state.manifest.get("workflowId", "")

        # No deployment available or already completed for this device
        if not state.deployment_available or (
            device_id,
            workflow_id,
        ) in state.completed_deployments:
            self._send_no_content()
            return

        logger.info("Serving deployment manifest to device '%s'", device_id)
        self._send_json(200, state.manifest)

    def _handle_put_state(self, device_id: str) -> None:
        """PUT /devices/{deviceId}/state → 200, logs reported state."""
        body = self._read_body()
        entry = {"deviceId": device_id, "state": body}
        self.server_state.device_states.append(entry)
        logger.info("Device '%s' reported state: %s", device_id, json.dumps(body))
        self._send_json(200, {"acknowledged": True})

    def _handle_post_result(self, device_id: str, workflow_id: str) -> None:
        """POST /devices/{deviceId}/deployments/{workflowId}/result

        Logs the result and marks the deployment as complete for this device.
        """
        body = self._read_body()
        entry = {
            "deviceId": device_id,
            "workflowId": workflow_id,
            "result": body,
        }
        self.server_state.deployment_results.append(entry)
        self.server_state.completed_deployments.add((device_id, workflow_id))
        logger.info(
            "Device '%s' completed deployment '%s': %s",
            device_id,
            workflow_id,
            json.dumps(body),
        )
        self._send_json(200, {"acknowledged": True})

    def _handle_get_file_url(self, device_id: str, file_id: str) -> None:
        """GET /devices/{deviceId}/files/{fileId}/url

        Returns a URL pointing to the local file content endpoint.
        """
        port = self.server_state.port
        url = f"http://localhost:{port}/files/{file_id}/content"
        logger.info(
            "Device '%s' requested URL for file '%s' → %s", device_id, file_id, url
        )
        self._send_json(200, {"url": url})

    def _handle_get_file_content(self, file_id: str) -> None:
        """GET /files/{fileId}/content

        Serves file from --content-dir if available, otherwise dummy content.
        """
        content_dir = self.server_state.content_dir

        if content_dir is not None:
            # Try to find the file by ID in the manifest, then serve from content_dir
            file_meta = self.server_state.manifest.get("files", {}).get(file_id)
            if file_meta:
                file_name = file_meta.get("fileName", file_id)
            else:
                file_name = file_id

            file_path = content_dir / file_name
            if file_path.is_file():
                data = file_path.read_bytes()
                self._send_binary(200, data, "application/octet-stream")
                return
            else:
                logger.warning("File not found in content dir: %s", file_path)
                self._send_json(404, {"error": f"File not found: {file_name}"})
                return

        # Serve dummy content
        logger.info("Serving dummy content for file '%s'", file_id)
        self._send_binary(200, DUMMY_FILE_CONTENT, "application/octet-stream")

    # ── Helper methods ──────────────────────────────────────────────────

    def _read_body(self) -> Any:
        """Read and parse JSON request body."""
        content_length = int(self.headers.get("Content-Length", 0))
        if content_length == 0:
            return None
        raw = self.rfile.read(content_length)
        try:
            return json.loads(raw)
        except json.JSONDecodeError:
            return raw.decode("utf-8", errors="replace")

    def _send_json(self, status: int, data: Any) -> None:
        """Send a JSON response."""
        body = json.dumps(data, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_binary(self, status: int, data: bytes, content_type: str) -> None:
        """Send a binary response."""
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _send_no_content(self) -> None:
        """Send 204 No Content."""
        self.send_response(204)
        self.end_headers()

    def log_message(self, format: str, *args: Any) -> None:
        """Override to use Python logging."""
        logger.debug("%s - %s", self.address_string(), format % args)


def load_manifest(manifest_path: str) -> dict[str, Any]:
    """Load a deployment manifest from a JSON file.

    Args:
        manifest_path: Path to the JSON manifest file.

    Returns:
        Parsed manifest dictionary.

    Raises:
        SystemExit: If the file cannot be read or parsed.
    """
    path = Path(manifest_path)
    if not path.is_file():
        print(f"Error: manifest file not found: {path}", file=sys.stderr)
        sys.exit(1)
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        print(f"Error: invalid JSON in manifest file: {e}", file=sys.stderr)
        sys.exit(1)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments.

    Args:
        argv: Argument list (defaults to sys.argv[1:]).

    Returns:
        Parsed arguments namespace.
    """
    parser = argparse.ArgumentParser(
        description="Mock ADU server for testing the Gen2 agent.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
Examples:
  # Start with default manifest on port 8080
  python server.py

  # Start with no pending deployment
  python server.py --no-deployment

  # Use a custom manifest and serve files from a directory
  python server.py --manifest-file my_manifest.json --content-dir ./payloads

  # Verbose logging on a custom port
  python server.py --port 9090 --verbose
""",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8080,
        help="Port to listen on (default: 8080)",
    )
    parser.add_argument(
        "--manifest-file",
        type=str,
        default=None,
        help="Path to a JSON file containing the deployment manifest",
    )
    parser.add_argument(
        "--content-dir",
        type=str,
        default=None,
        help="Directory to serve file content from",
    )
    parser.add_argument(
        "--no-deployment",
        action="store_true",
        help="Start with no pending deployment (returns 204)",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose request logging",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> None:
    """Entry point for the mock ADU server.

    Args:
        argv: Optional argument list for testing.
    """
    args = parse_args(argv)

    # Configure logging
    log_level = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(
        level=log_level,
        format="%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%H:%M:%S",
    )

    # Load manifest
    if args.manifest_file:
        manifest = load_manifest(args.manifest_file)
        logger.info("Loaded manifest from: %s", args.manifest_file)
    else:
        manifest = DEFAULT_MANIFEST
        logger.info("Using default deployment manifest")

    # Validate content directory
    content_dir: Path | None = None
    if args.content_dir:
        content_dir = Path(args.content_dir)
        if not content_dir.is_dir():
            print(f"Error: content directory not found: {content_dir}", file=sys.stderr)
            sys.exit(1)
        logger.info("Serving file content from: %s", content_dir)

    # Set up shared state
    state = ServerState(
        manifest=manifest,
        no_deployment=args.no_deployment,
        content_dir=content_dir,
        port=args.port,
    )
    ADURequestHandler.server_state = state

    # Start server
    server = HTTPServer(("0.0.0.0", args.port), ADURequestHandler)
    logger.info("Mock ADU server listening on http://0.0.0.0:%d", args.port)
    if args.no_deployment:
        logger.info("Started with --no-deployment: all deployment queries return 204")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        logger.info("Shutting down...")
        server.shutdown()


if __name__ == "__main__":
    main()
