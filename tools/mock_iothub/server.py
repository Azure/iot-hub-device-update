#!/usr/bin/env python3
"""Mock IoT Hub server for testing the ADU Gen2 agent communication adapter.

Simulates IoT Hub device twin + direct methods for local development and testing.
Uses only Python standard library modules (no external dependencies).

Endpoints:
  GET  /twins/{deviceId}                         - Get device twin
  PATCH /twins/{deviceId}/properties/reported    - Update reported properties
  POST /twins/{deviceId}/methods/{methodName}    - Invoke direct method
  GET  /devices/{deviceId}/deployments           - Get ADU deployment
  POST /devices/{deviceId}/deployments/{wfId}/result - Report deployment result
  GET  /files/{fileId}/content                   - Serve payload files
  POST /admin/trigger-deployment                 - Push new deployment
  GET  /admin/status                             - Show all device states
  GET  /health                                   - Health check
"""

from __future__ import annotations

import argparse
import copy
import json
import logging
import os
import re
import sys
import threading
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from typing import Any
from urllib.parse import urlparse, parse_qs

logger = logging.getLogger(__name__)


# Default manifest used when no --manifest-file is provided
DEFAULT_MANIFEST: dict[str, Any] = {
    "workflowId": "wf-default-001",
    "updateId": {
        "provider": "Contoso",
        "name": "DefaultUpdate",
        "version": "1.0.0",
    },
    "instructions": {
        "steps": [
            {
                "type": "inline",
                "handler": "microsoft/script:2",
                "handlerProperties": {
                    "scriptFileName": "install.sh",
                    "timeout": 60,
                },
                "files": ["f1"],
                "installedCriteria": "1.0.0",
            }
        ]
    },
    "files": {
        "f1": {
            "fileName": "install.sh",
            "sizeInBytes": 128,
            "hashes": {"sha256": "default-hash"},
        }
    },
}

DUMMY_FILE_CONTENT = b"#!/bin/bash\necho 'Mock install script'\nexit 0\n"


class DeviceTwin:
    """Represents an IoT Hub device twin for a single device."""

    def __init__(self, device_id: str) -> None:
        self.device_id = device_id
        self.desired_version = 1
        self.reported_version = 1
        self.desired: dict[str, Any] = {}
        self.reported: dict[str, Any] = {
            "ADUAgent": {
                "state": "Idle",
                "lastDeploymentId": "",
                "installedUpdateId": "",
            }
        }

    def get_twin(self) -> dict[str, Any]:
        return {
            "deviceId": self.device_id,
            "properties": {
                "desired": {**self.desired, "$version": self.desired_version},
                "reported": {**self.reported, "$version": self.reported_version},
            },
        }

    def update_reported(self, patch: dict[str, Any]) -> None:
        self._deep_merge(self.reported, patch)
        self.reported_version += 1

    def set_desired(self, patch: dict[str, Any]) -> None:
        self._deep_merge(self.desired, patch)
        self.desired_version += 1

    @staticmethod
    def _deep_merge(target: dict, source: dict) -> None:
        for key, value in source.items():
            if key.startswith("$"):
                continue
            if isinstance(value, dict) and isinstance(target.get(key), dict):
                DeviceTwin._deep_merge(target[key], value)
            else:
                target[key] = value


class ServerState:
    """Shared mutable state for the mock IoT Hub server."""

    def __init__(
        self,
        manifest: dict[str, Any],
        no_deployment: bool,
        content_dir: Path | None,
        port: int,
        default_device_id: str,
    ) -> None:
        self.manifest = manifest
        self.deployment_available = not no_deployment
        self.content_dir = content_dir
        self.port = port
        self.default_device_id = default_device_id
        self.lock = threading.Lock()

        # Device twins keyed by device ID
        self.twins: dict[str, DeviceTwin] = {}

        # Interaction log
        self.interactions: list[dict[str, Any]] = []
        self.deployment_results: list[dict[str, Any]] = []
        self.method_calls: list[dict[str, Any]] = []

        # Initialize default device
        self._get_or_create_twin(default_device_id)

    def _get_or_create_twin(self, device_id: str) -> DeviceTwin:
        if device_id not in self.twins:
            twin = DeviceTwin(device_id)
            if self.deployment_available:
                twin.set_desired(
                    {
                        "ADUGroup": "test-group",
                        "deployment": {
                            "workflowId": self.manifest.get("workflowId", ""),
                            "manifest": self.manifest,
                        },
                    }
                )
            self.twins[device_id] = twin
        return self.twins[device_id]

    def get_twin(self, device_id: str) -> DeviceTwin:
        with self.lock:
            return self._get_or_create_twin(device_id)

    def log_interaction(self, device_id: str, action: str, data: Any = None) -> None:
        entry = {
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "deviceId": device_id,
            "action": action,
            "data": data,
        }
        with self.lock:
            self.interactions.append(entry)
        logger.info("[%s] %s: %s", device_id, action, json.dumps(data) if data else "")

    def trigger_deployment(self, manifest: dict[str, Any], device_id: str | None = None) -> None:
        """Push a new deployment to one or all devices."""
        with self.lock:
            self.manifest = manifest
            self.deployment_available = True
            targets = [device_id] if device_id else list(self.twins.keys())
            for did in targets:
                twin = self._get_or_create_twin(did)
                twin.set_desired(
                    {
                        "ADUGroup": "test-group",
                        "deployment": {
                            "workflowId": manifest.get("workflowId", ""),
                            "manifest": manifest,
                        },
                    }
                )

    def get_status(self) -> dict[str, Any]:
        with self.lock:
            devices = {}
            for did, twin in self.twins.items():
                devices[did] = twin.get_twin()
            return {
                "devices": devices,
                "deployment_available": self.deployment_available,
                "total_interactions": len(self.interactions),
                "deployment_results": self.deployment_results[-10:],
                "recent_interactions": self.interactions[-20:],
            }


class IoTHubRequestHandler(BaseHTTPRequestHandler):
    """HTTP request handler implementing mock IoT Hub endpoints."""

    server_state: ServerState

    # Route patterns
    _HEALTH_RE = re.compile(r"^/health$")
    _TWIN_GET_RE = re.compile(r"^/twins/(?P<deviceId>[^/]+)$")
    _TWIN_REPORTED_RE = re.compile(r"^/twins/(?P<deviceId>[^/]+)/properties/reported$")
    _METHOD_RE = re.compile(r"^/twins/(?P<deviceId>[^/]+)/methods/(?P<methodName>[^/]+)$")
    _DEPLOYMENTS_RE = re.compile(r"^/devices/(?P<deviceId>[^/]+)/deployments$")
    _RESULT_RE = re.compile(
        r"^/devices/(?P<deviceId>[^/]+)/deployments/(?P<workflowId>[^/]+)/result$"
    )
    _FILE_URL_RE = re.compile(r"^/devices/(?P<deviceId>[^/]+)/files/(?P<fileId>[^/]+)/url$")
    _FILE_CONTENT_RE = re.compile(r"^/files/(?P<fileId>[^/]+)/content$")
    _ADMIN_TRIGGER_RE = re.compile(r"^/admin/trigger-deployment$")
    _ADMIN_STATUS_RE = re.compile(r"^/admin/status$")

    def do_GET(self) -> None:
        path = urlparse(self.path).path

        if self._HEALTH_RE.match(path):
            self._send_json(200, {"status": "ok", "server": "mock-iothub"})
        elif m := self._TWIN_GET_RE.match(path):
            self._handle_get_twin(m.group("deviceId"))
        elif m := self._DEPLOYMENTS_RE.match(path):
            self._handle_get_deployments(m.group("deviceId"))
        elif m := self._FILE_URL_RE.match(path):
            self._handle_get_file_url(m.group("deviceId"), m.group("fileId"))
        elif m := self._FILE_CONTENT_RE.match(path):
            self._handle_get_file_content(m.group("fileId"))
        elif self._ADMIN_STATUS_RE.match(path):
            self._handle_admin_status()
        else:
            self._send_json(404, {"error": "Not found", "path": path})

    def do_PATCH(self) -> None:
        path = urlparse(self.path).path

        if m := self._TWIN_REPORTED_RE.match(path):
            self._handle_patch_reported(m.group("deviceId"))
        else:
            self._send_json(404, {"error": "Not found", "path": path})

    def do_POST(self) -> None:
        path = urlparse(self.path).path

        if m := self._METHOD_RE.match(path):
            self._handle_method_invoke(m.group("deviceId"), m.group("methodName"))
        elif m := self._RESULT_RE.match(path):
            self._handle_post_result(m.group("deviceId"), m.group("workflowId"))
        elif self._ADMIN_TRIGGER_RE.match(path):
            self._handle_admin_trigger()
        else:
            self._send_json(404, {"error": "Not found", "path": path})

    def do_PUT(self) -> None:
        """Handle PUT for backward compatibility with ADU Direct provider."""
        path = urlparse(self.path).path
        # Support PUT /devices/{deviceId}/state for legacy compat
        state_re = re.compile(r"^/devices/(?P<deviceId>[^/]+)/state$")
        if m := state_re.match(path):
            body = self._read_body()
            device_id = m.group("deviceId")
            state = self.server_state
            state.log_interaction(device_id, "state_report", body)
            if isinstance(body, dict):
                twin = state.get_twin(device_id)
                twin.update_reported({"ADUAgent": body})
            self._send_json(200, {"acknowledged": True})
        else:
            self._send_json(404, {"error": "Not found", "path": path})

    # ── Twin endpoints ──────────────────────────────────────────────────

    def _handle_get_twin(self, device_id: str) -> None:
        """GET /twins/{deviceId} — Return full device twin."""
        state = self.server_state
        twin = state.get_twin(device_id)
        state.log_interaction(device_id, "twin_get")
        self._send_json(200, twin.get_twin())

    def _handle_patch_reported(self, device_id: str) -> None:
        """PATCH /twins/{deviceId}/properties/reported — Update reported properties."""
        body = self._read_body()
        state = self.server_state
        twin = state.get_twin(device_id)
        if isinstance(body, dict):
            twin.update_reported(body)
        state.log_interaction(device_id, "reported_update", body)
        self._send_json(200, {
            "acknowledged": True,
            "version": twin.reported_version,
        })

    # ── Direct method endpoint ──────────────────────────────────────────

    def _handle_method_invoke(self, device_id: str, method_name: str) -> None:
        """POST /twins/{deviceId}/methods/{methodName} — Direct method invocation."""
        body = self._read_body()
        state = self.server_state
        entry = {
            "deviceId": device_id,
            "methodName": method_name,
            "payload": body,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        }
        state.method_calls.append(entry)
        state.log_interaction(device_id, f"method_invoke:{method_name}", body)

        # Simulate method response based on method name
        if method_name == "adu_apply":
            response = {"status": 200, "payload": {"result": "accepted"}}
        elif method_name == "adu_cancel":
            response = {"status": 200, "payload": {"result": "cancelled"}}
        elif method_name == "reboot":
            response = {"status": 200, "payload": {"result": "rebooting"}}
        else:
            response = {"status": 200, "payload": {"result": "ok"}}

        self._send_json(200, response)

    # ── Deployment endpoints ────────────────────────────────────────────

    def _handle_get_deployments(self, device_id: str) -> None:
        """GET /devices/{deviceId}/deployments — Return pending deployment or 204."""
        state = self.server_state

        if not state.deployment_available:
            self._send_no_content()
            return

        twin = state.get_twin(device_id)
        deployment = twin.get_twin()["properties"]["desired"].get("deployment")
        if not deployment:
            self._send_no_content()
            return

        state.log_interaction(device_id, "deployment_get")
        # Return the manifest from the deployment
        manifest = deployment.get("manifest", state.manifest)
        self._send_json(200, manifest)

    def _handle_post_result(self, device_id: str, workflow_id: str) -> None:
        """POST /devices/{deviceId}/deployments/{workflowId}/result — Report result."""
        body = self._read_body()
        state = self.server_state
        entry = {
            "deviceId": device_id,
            "workflowId": workflow_id,
            "result": body,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        }
        state.deployment_results.append(entry)
        state.log_interaction(device_id, "deployment_result", {
            "workflowId": workflow_id,
            "result": body,
        })

        # Update twin reported state
        twin = state.get_twin(device_id)
        twin.update_reported({
            "ADUAgent": {
                "state": "Idle",
                "lastDeploymentId": workflow_id,
            }
        })

        self._send_json(200, {"acknowledged": True})

    # ── File serving ────────────────────────────────────────────────────

    def _handle_get_file_url(self, device_id: str, file_id: str) -> None:
        """GET /devices/{deviceId}/files/{fileId}/url — Return download URL."""
        port = self.server_state.port
        url = f"http://localhost:{port}/files/{file_id}/content"
        self.server_state.log_interaction(device_id, "file_url_request", {"fileId": file_id})
        self._send_json(200, {"url": url})

    def _handle_get_file_content(self, file_id: str) -> None:
        """GET /files/{fileId}/content — Serve file content."""
        content_dir = self.server_state.content_dir
        manifest = self.server_state.manifest

        if content_dir is not None:
            file_meta = manifest.get("files", {}).get(file_id)
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

        logger.info("Serving dummy content for file '%s'", file_id)
        self._send_binary(200, DUMMY_FILE_CONTENT, "application/octet-stream")

    # ── Admin endpoints ─────────────────────────────────────────────────

    def _handle_admin_trigger(self) -> None:
        """POST /admin/trigger-deployment — Push a new deployment."""
        body = self._read_body()
        state = self.server_state

        if isinstance(body, dict):
            manifest = body.get("manifest", body)
            device_id = body.get("deviceId")
            state.trigger_deployment(manifest, device_id)
            logger.info("Admin triggered deployment: %s", manifest.get("workflowId", "unknown"))
            self._send_json(200, {
                "triggered": True,
                "workflowId": manifest.get("workflowId", ""),
            })
        else:
            self._send_json(400, {"error": "Request body must be JSON with 'manifest' field"})

    def _handle_admin_status(self) -> None:
        """GET /admin/status — Show all device states."""
        self._send_json(200, self.server_state.get_status())

    # ── Helpers ─────────────────────────────────────────────────────────

    def _read_body(self) -> Any:
        content_length = int(self.headers.get("Content-Length", 0))
        if content_length == 0:
            return None
        raw = self.rfile.read(content_length)
        try:
            return json.loads(raw)
        except json.JSONDecodeError:
            return raw.decode("utf-8", errors="replace")

    def _send_json(self, status: int, data: Any) -> None:
        body = json.dumps(data, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_binary(self, status: int, data: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _send_no_content(self) -> None:
        self.send_response(204)
        self.end_headers()

    def log_message(self, format: str, *args: Any) -> None:
        logger.debug("%s - %s", self.address_string(), format % args)


def load_manifest(manifest_path: str) -> dict[str, Any]:
    """Load a deployment manifest from a JSON file."""
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
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Mock IoT Hub server for testing the ADU Gen2 agent.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
Examples:
  # Start with default manifest on port 8081
  python server.py --port 8081

  # Use a custom manifest and serve payload files
  python server.py --manifest-file manifest.json --content-dir ./payloads --verbose

  # Start with no pending deployment
  python server.py --no-deployment

  # Specify a default device ID
  python server.py --device-id my-device-001
""",
    )
    parser.add_argument(
        "--port", type=int, default=8081,
        help="Port to listen on (default: 8081)",
    )
    parser.add_argument(
        "--manifest-file", type=str, default=None,
        help="Path to a JSON file containing the deployment manifest",
    )
    parser.add_argument(
        "--content-dir", type=str, default=None,
        help="Directory to serve file content from",
    )
    parser.add_argument(
        "--device-id", type=str, default="test-device-001",
        help="Default device ID to initialize (default: test-device-001)",
    )
    parser.add_argument(
        "--no-deployment", action="store_true",
        help="Start with no pending deployment (returns 204)",
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Enable verbose/debug logging",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> None:
    """Entry point for the mock IoT Hub server."""
    args = parse_args(argv)

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
        default_device_id=args.device_id,
    )
    IoTHubRequestHandler.server_state = state

    # Start server
    server = HTTPServer(("0.0.0.0", args.port), IoTHubRequestHandler)
    logger.info("Mock IoT Hub server listening on http://0.0.0.0:%d", args.port)
    logger.info("Default device: %s", args.device_id)
    if args.no_deployment:
        logger.info("Started with --no-deployment: twin has no deployment in desired")

    print(f"\nMock IoT Hub ready at http://localhost:{args.port}")
    print(f"  Health:    GET  /health")
    print(f"  Twin:      GET  /twins/{{deviceId}}")
    print(f"  Reported:  PATCH /twins/{{deviceId}}/properties/reported")
    print(f"  Methods:   POST /twins/{{deviceId}}/methods/{{methodName}}")
    print(f"  Deploy:    GET  /devices/{{deviceId}}/deployments")
    print(f"  Admin:     GET  /admin/status")
    print(f"  Trigger:   POST /admin/trigger-deployment")
    print()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        logger.info("Shutting down...")
        server.shutdown()


if __name__ == "__main__":
    main()
