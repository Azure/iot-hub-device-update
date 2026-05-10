"""Unit tests for ADU agent client and diagnostics."""

import io
import json
import os
import struct
import tarfile
import tempfile
import unittest
from unittest import mock

from azext_adu_device.agent_client import (
    ADUAgentClient,
    ADUAgentError,
    HEADER_FORMAT,
    HEADER_SIZE,
    decode_response,
    encode_request,
)
from azext_adu_device.diagnostics import DiagnosticCollector, _sanitize_config


class TestWireProtocolEncoding(unittest.TestCase):
    """Test wire protocol request encoding."""

    def test_encode_request_no_payload(self):
        data = encode_request(1, ADUAgentClient.MSG_GET_STATUS)
        self.assertEqual(len(data), HEADER_SIZE)
        ver, msg_type, length = struct.unpack(HEADER_FORMAT, data)
        self.assertEqual(ver, 1)
        self.assertEqual(msg_type, 1)
        self.assertEqual(length, 0)

    def test_encode_request_with_payload(self):
        payload = b'{"force": true}'
        data = encode_request(1, ADUAgentClient.MSG_CANCEL_UPDATE, payload)
        self.assertEqual(len(data), HEADER_SIZE + len(payload))
        ver, msg_type, length = struct.unpack(HEADER_FORMAT, data[:HEADER_SIZE])
        self.assertEqual(ver, 1)
        self.assertEqual(msg_type, ADUAgentClient.MSG_CANCEL_UPDATE)
        self.assertEqual(length, len(payload))
        self.assertEqual(data[HEADER_SIZE:], payload)

    def test_encode_all_message_types(self):
        for msg_type in (1, 2, 3, 4, 5):
            data = encode_request(1, msg_type)
            _, decoded_type, _ = struct.unpack(HEADER_FORMAT, data)
            self.assertEqual(decoded_type, msg_type)


class TestWireProtocolDecoding(unittest.TestCase):
    """Test wire protocol response decoding."""

    def test_decode_ok_response_with_payload(self):
        payload = json.dumps({"state": "Idle"}).encode()
        header = struct.pack(HEADER_FORMAT, 1, 0, len(payload))
        ver, status, data = decode_response(header + payload)
        self.assertEqual(ver, 1)
        self.assertEqual(status, 0)
        self.assertEqual(json.loads(data), {"state": "Idle"})

    def test_decode_ok_response_no_payload(self):
        header = struct.pack(HEADER_FORMAT, 1, 0, 0)
        ver, status, data = decode_response(header)
        self.assertEqual(ver, 1)
        self.assertEqual(status, 0)
        self.assertEqual(data, b"")

    def test_decode_error_response(self):
        payload = json.dumps({"error": "Not running"}).encode()
        header = struct.pack(HEADER_FORMAT, 1, 1, len(payload))
        ver, status, data = decode_response(header + payload)
        self.assertEqual(status, 1)
        self.assertEqual(json.loads(data)["error"], "Not running")

    def test_decode_truncated_header_raises(self):
        with self.assertRaises(ADUAgentError):
            decode_response(b"\x01\x00")

    def test_decode_truncated_payload_raises(self):
        header = struct.pack(HEADER_FORMAT, 1, 0, 100)
        with self.assertRaises(ADUAgentError):
            decode_response(header + b"short")


class TestADUAgentClient(unittest.TestCase):
    """Test ADUAgentClient with mocked socket."""

    def _make_mock_socket(self, response_status, response_payload=b""):
        """Create a mock socket that returns a canned response."""
        resp_header = struct.pack(
            HEADER_FORMAT, 1, response_status, len(response_payload)
        )
        resp_data = resp_header + response_payload

        mock_sock = mock.MagicMock()
        mock_sock.recv = mock.MagicMock(side_effect=[resp_data])

        # For _recv_exact: return header first, then payload
        call_count = [0]
        def recv_side_effect(n):
            nonlocal call_count
            if call_count[0] == 0:
                call_count[0] += 1
                return resp_header
            else:
                call_count[0] += 1
                return response_payload
        mock_sock.recv = mock.MagicMock(side_effect=recv_side_effect)

        return mock_sock

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_get_status_success(self, mock_socket_cls):
        status_data = {
            "state": "Idle",
            "installedUpdateId": "provider.name.1.0",
            "lastCheckTime": "2024-01-01T00:00:00Z",
            "connectionStatus": "Connected",
        }
        payload = json.dumps(status_data).encode()
        mock_sock = self._make_mock_socket(0, payload)
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient(socket_path="/test/adu.sock")
        result = client.get_status()

        self.assertEqual(result["state"], "Idle")
        self.assertEqual(result["connectionStatus"], "Connected")
        mock_sock.connect.assert_called_once_with("/test/adu.sock")
        mock_sock.sendall.assert_called_once()

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_get_health_success(self, mock_socket_cls):
        health_data = {
            "agentHealthy": True,
            "extensions": [
                {"name": "apt", "type": "updateHandler", "status": "loaded"}
            ],
        }
        payload = json.dumps(health_data).encode()
        mock_sock = self._make_mock_socket(0, payload)
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient()
        result = client.get_health()
        self.assertTrue(result["agentHealthy"])

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_trigger_scan_success(self, mock_socket_cls):
        payload = json.dumps({"scanTriggered": True}).encode()
        mock_sock = self._make_mock_socket(0, payload)
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient()
        result = client.trigger_scan()
        self.assertTrue(result["scanTriggered"])

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_cancel_update_with_force(self, mock_socket_cls):
        payload = json.dumps({"cancelled": True}).encode()
        mock_sock = self._make_mock_socket(0, payload)
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient()
        result = client.cancel_update(force=True)
        self.assertTrue(result["cancelled"])

        # Verify force flag was sent in payload
        sent_data = mock_sock.sendall.call_args[0][0]
        sent_header = sent_data[:HEADER_SIZE]
        _, msg_type, payload_len = struct.unpack(HEADER_FORMAT, sent_header)
        self.assertEqual(msg_type, ADUAgentClient.MSG_CANCEL_UPDATE)
        self.assertGreater(payload_len, 0)
        sent_payload = json.loads(sent_data[HEADER_SIZE:])
        self.assertTrue(sent_payload["force"])

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_error_status_raises(self, mock_socket_cls):
        payload = json.dumps({"error": "Something broke"}).encode()
        mock_sock = self._make_mock_socket(1, payload)
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient()
        with self.assertRaises(ADUAgentError) as ctx:
            client.get_status()
        self.assertIn("Internal error", str(ctx.exception))

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_permission_denied_status(self, mock_socket_cls):
        mock_sock = self._make_mock_socket(4, b"")
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient()
        with self.assertRaises(ADUAgentError) as ctx:
            client.get_status()
        self.assertIn("Permission denied", str(ctx.exception))

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_socket_not_found(self, mock_socket_cls):
        mock_sock = mock.MagicMock()
        mock_sock.connect.side_effect = FileNotFoundError()
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient(socket_path="/nonexistent/adu.sock")
        with self.assertRaises(ADUAgentError) as ctx:
            client.get_status()
        self.assertIn("not found", str(ctx.exception))

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_socket_permission_error(self, mock_socket_cls):
        mock_sock = mock.MagicMock()
        mock_sock.connect.side_effect = PermissionError()
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient()
        with self.assertRaises(ADUAgentError) as ctx:
            client.get_status()
        self.assertIn("Permission denied", str(ctx.exception))

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_socket_timeout(self, mock_socket_cls):
        import socket as socket_mod

        mock_sock = mock.MagicMock()
        mock_sock.recv.side_effect = socket_mod.timeout("timed out")
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient(timeout=5)
        with self.assertRaises(ADUAgentError) as ctx:
            client.get_status()
        self.assertIn("Timed out", str(ctx.exception))

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_connection_closed_early(self, mock_socket_cls):
        mock_sock = mock.MagicMock()
        mock_sock.recv.return_value = b""
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient()
        with self.assertRaises(ADUAgentError) as ctx:
            client.get_status()
        self.assertIn("Connection closed", str(ctx.exception))

    @mock.patch("azext_adu_device.agent_client.socket.socket")
    def test_protocol_version_mismatch(self, mock_socket_cls):
        # Response with version 99
        resp_header = struct.pack(HEADER_FORMAT, 99, 0, 0)
        mock_sock = mock.MagicMock()
        call_count = [0]
        def recv_effect(n):
            if call_count[0] == 0:
                call_count[0] += 1
                return resp_header
            return b""
        mock_sock.recv = mock.MagicMock(side_effect=recv_effect)
        mock_socket_cls.return_value = mock_sock

        client = ADUAgentClient()
        with self.assertRaises(ADUAgentError) as ctx:
            client.get_status()
        self.assertIn("version mismatch", str(ctx.exception))


class TestConfigSanitization(unittest.TestCase):
    """Test secret redaction in config files."""

    def test_connection_string_redacted(self):
        config = 'connection_string = "HostName=hub.azure.net;DeviceId=d1;SharedAccessKey=abc123="'
        result = _sanitize_config(config)
        self.assertIn("***REDACTED***", result)
        self.assertNotIn("abc123", result)

    def test_multiple_secrets_redacted(self):
        config = (
            'connection_string = "secret1"\n'
            'password = "mypassword"\n'
            'sas_token = "sv=2021&sig=xyz"\n'
            'normal_setting = "keep this"'
        )
        result = _sanitize_config(config)
        self.assertNotIn("secret1", result)
        self.assertNotIn("mypassword", result)
        self.assertNotIn("xyz", result)
        self.assertIn("keep this", result)
        self.assertEqual(result.count("***REDACTED***"), 3)

    def test_no_secrets_unchanged(self):
        config = '[agent]\nlog_level = "info"\nlog_dir = "/var/log/adu"'
        result = _sanitize_config(config)
        self.assertEqual(result, config)


class TestDiagnosticCollector(unittest.TestCase):
    """Test diagnostic bundle creation."""

    def test_parse_since_relative_hours(self):
        import datetime
        dt = DiagnosticCollector._parse_since("2h")
        expected = datetime.datetime.now() - datetime.timedelta(hours=2)
        self.assertAlmostEqual(dt.timestamp(), expected.timestamp(), delta=2)

    def test_parse_since_relative_days(self):
        import datetime
        dt = DiagnosticCollector._parse_since("3d")
        expected = datetime.datetime.now() - datetime.timedelta(days=3)
        self.assertAlmostEqual(dt.timestamp(), expected.timestamp(), delta=2)

    def test_parse_since_relative_minutes(self):
        import datetime
        dt = DiagnosticCollector._parse_since("30m")
        expected = datetime.datetime.now() - datetime.timedelta(minutes=30)
        self.assertAlmostEqual(dt.timestamp(), expected.timestamp(), delta=2)

    def test_parse_since_absolute_date(self):
        dt = DiagnosticCollector._parse_since("2024-06-15")
        self.assertEqual(dt.year, 2024)
        self.assertEqual(dt.month, 6)
        self.assertEqual(dt.day, 15)

    def test_parse_since_invalid_raises(self):
        with self.assertRaises(ValueError):
            DiagnosticCollector._parse_since("not-a-date")

    def test_parse_since_none_returns_none(self):
        self.assertIsNone(DiagnosticCollector._parse_since(None))

    def test_normalize_since_for_journalctl(self):
        self.assertEqual(
            DiagnosticCollector._normalize_since_for_journalctl("2h"),
            "2 hours ago",
        )
        self.assertEqual(
            DiagnosticCollector._normalize_since_for_journalctl("30m"),
            "30 minutes ago",
        )
        self.assertEqual(
            DiagnosticCollector._normalize_since_for_journalctl("1d"),
            "1 days ago",
        )
        # Absolute dates pass through unchanged
        self.assertEqual(
            DiagnosticCollector._normalize_since_for_journalctl("2024-01-01"),
            "2024-01-01",
        )

    def test_collect_system_info(self):
        collector = DiagnosticCollector()
        info = collector.collect_system_info()
        self.assertIn("timestamp", info)
        self.assertIn("uname", info)
        self.assertIn("system", info["uname"])
        self.assertIn("machine", info["uname"])

    def test_create_bundle(self):
        """Test bundle creation with mocked file system."""
        collector = DiagnosticCollector(
            log_dir="__nonexistent_log_dir__",
            config_path="__nonexistent_config__",
        )

        bundle_dir = os.path.join(os.path.dirname(__file__), "_test_output")
        os.makedirs(bundle_dir, exist_ok=True)
        bundle_path = os.path.join(bundle_dir, "test_bundle.tar.gz")

        try:
            path, manifest = collector.create_bundle(
                output_path=bundle_path,
                client=None,
                since=None,
                include_core_dumps=False,
            )

            self.assertTrue(os.path.isfile(path))
            self.assertIn("components", manifest)

            # Verify the tarball contains at least system info and manifest
            with tarfile.open(path, "r:gz") as tar:
                names = tar.getnames()
                self.assertIn("system/info.json", names)
                self.assertIn("manifest.json", names)

                # Verify system info is valid JSON
                sys_info = json.loads(
                    tar.extractfile("system/info.json").read()
                )
                self.assertIn("uname", sys_info)
        finally:
            if os.path.isfile(bundle_path):
                os.unlink(bundle_path)
            if os.path.isdir(bundle_dir):
                os.rmdir(bundle_dir)

    def test_add_string_to_tar(self):
        """Test adding string content to a tar archive."""
        buf = io.BytesIO()
        with tarfile.open(fileobj=buf, mode="w:gz") as tar:
            DiagnosticCollector._add_string_to_tar(
                tar, "test.txt", "hello world"
            )
        buf.seek(0)
        with tarfile.open(fileobj=buf, mode="r:gz") as tar:
            content = tar.extractfile("test.txt").read().decode()
            self.assertEqual(content, "hello world")


if __name__ == "__main__":
    unittest.main()
