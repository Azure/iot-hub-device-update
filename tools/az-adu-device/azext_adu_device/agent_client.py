"""Local API client for the ADU agent via Unix domain socket wire protocol."""

import json
import socket
import struct

# AF_UNIX may not be available on all platforms (e.g. older Windows)
_AF_UNIX = getattr(socket, "AF_UNIX", 1)

# Wire protocol header: version (u16) + type/status (u16) + payload length (u16)
HEADER_FORMAT = "<HHH"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)


class ADUAgentError(Exception):
    """Error communicating with the ADU agent."""


class ADUAgentClient:
    """Client for the ADU agent Local API over Unix domain socket.

    Wire protocol:
        Request:  [ver:u16][type:u16][len:u16][payload:var]
        Response: [ver:u16][status:u16][len:u16][payload:var]
    """

    PROTOCOL_VERSION = 1

    # Message types
    MSG_GET_STATUS = 1
    MSG_GET_HEALTH = 2
    MSG_TRIGGER_SCAN = 3
    MSG_GET_DIAGNOSTICS = 4
    MSG_CANCEL_UPDATE = 5

    # Response status codes
    STATUS_OK = 0
    STATUS_ERROR = 1
    STATUS_NOT_FOUND = 2
    STATUS_BUSY = 3
    STATUS_PERMISSION_DENIED = 4

    STATUS_MESSAGES = {
        STATUS_OK: "OK",
        STATUS_ERROR: "Internal error",
        STATUS_NOT_FOUND: "Not found",
        STATUS_BUSY: "Agent is busy",
        STATUS_PERMISSION_DENIED: "Permission denied",
    }

    def __init__(self, socket_path="/run/adu/adu.sock", timeout=30):
        self.socket_path = socket_path
        self.timeout = timeout

    def _send_request(self, msg_type, payload=b""):
        """Send a request and return (status, payload_bytes)."""
        header = struct.pack(
            HEADER_FORMAT, self.PROTOCOL_VERSION, msg_type, len(payload)
        )

        try:
            sock = socket.socket(_AF_UNIX, socket.SOCK_STREAM)
            sock.settimeout(self.timeout)
            sock.connect(self.socket_path)
        except FileNotFoundError:
            raise ADUAgentError(
                f"Agent socket not found at {self.socket_path}. "
                "Is the ADU agent running?"
            )
        except PermissionError:
            raise ADUAgentError(
                f"Permission denied connecting to {self.socket_path}. "
                "Ensure you are in the 'adu' group or running as root."
            )
        except ConnectionRefusedError:
            raise ADUAgentError(
                f"Connection refused at {self.socket_path}. "
                "The ADU agent may be restarting."
            )
        except OSError as e:
            raise ADUAgentError(f"Failed to connect to agent socket: {e}")

        try:
            sock.sendall(header + payload)
            resp_header = self._recv_exact(sock, HEADER_SIZE)
            version, status, resp_len = struct.unpack(HEADER_FORMAT, resp_header)

            if version != self.PROTOCOL_VERSION:
                raise ADUAgentError(
                    f"Protocol version mismatch: expected {self.PROTOCOL_VERSION}, "
                    f"got {version}"
                )

            resp_payload = self._recv_exact(sock, resp_len) if resp_len > 0 else b""
            return status, resp_payload
        except socket.timeout:
            raise ADUAgentError(
                f"Timed out waiting for agent response (timeout={self.timeout}s)"
            )
        finally:
            sock.close()

    @staticmethod
    def _recv_exact(sock, num_bytes):
        """Receive exactly num_bytes from socket."""
        buf = bytearray()
        while len(buf) < num_bytes:
            chunk = sock.recv(num_bytes - len(buf))
            if not chunk:
                raise ADUAgentError(
                    "Connection closed by agent before full response was received"
                )
            buf.extend(chunk)
        return bytes(buf)

    def _request_json(self, msg_type, payload=b""):
        """Send request and parse JSON response, raising on error status."""
        status, data = self._send_request(msg_type, payload)
        if status != self.STATUS_OK:
            msg = self.STATUS_MESSAGES.get(status, f"Unknown error (status={status})")
            detail = ""
            if data:
                try:
                    detail = f": {json.loads(data).get('error', data.decode())}"
                except (json.JSONDecodeError, UnicodeDecodeError):
                    detail = f": {data.decode(errors='replace')}"
            raise ADUAgentError(f"Agent returned error: {msg}{detail}")

        if not data:
            return {}
        try:
            return json.loads(data)
        except json.JSONDecodeError as e:
            raise ADUAgentError(f"Invalid JSON in agent response: {e}")

    def get_status(self):
        """Get agent status: state, installed update, last check time, connection."""
        return self._request_json(self.MSG_GET_STATUS)

    def get_health(self):
        """Get health information for agent and extensions."""
        return self._request_json(self.MSG_GET_HEALTH)

    def trigger_scan(self):
        """Trigger an update check."""
        return self._request_json(self.MSG_TRIGGER_SCAN)

    def cancel_update(self, force=False):
        """Cancel an in-progress update."""
        payload = b""
        if force:
            payload = json.dumps({"force": True}).encode("utf-8")
        return self._request_json(self.MSG_CANCEL_UPDATE, payload)

    def get_diagnostics(self):
        """Collect diagnostic information from the agent."""
        return self._request_json(self.MSG_GET_DIAGNOSTICS)


def encode_request(version, msg_type, payload=b""):
    """Encode a wire protocol request. Utility for testing."""
    header = struct.pack(HEADER_FORMAT, version, msg_type, len(payload))
    return header + payload


def decode_response(data):
    """Decode a wire protocol response. Returns (version, status, payload_bytes)."""
    if len(data) < HEADER_SIZE:
        raise ADUAgentError(
            f"Response too short: expected at least {HEADER_SIZE} bytes, "
            f"got {len(data)}"
        )
    version, status, payload_len = struct.unpack(HEADER_FORMAT, data[:HEADER_SIZE])
    payload = data[HEADER_SIZE : HEADER_SIZE + payload_len]
    if len(payload) < payload_len:
        raise ADUAgentError(
            f"Incomplete payload: expected {payload_len} bytes, got {len(payload)}"
        )
    return version, status, payload
