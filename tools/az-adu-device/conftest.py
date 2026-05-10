"""Conftest to mock azure-cli dependencies for standalone testing."""

import sys
import types


def _ensure_mock_module(name, attrs=None):
    """Create a stub module if it doesn't already exist."""
    if name not in sys.modules:
        mod = types.ModuleType(name)
        sys.modules[name] = mod
    if attrs:
        for k, v in attrs.items():
            setattr(sys.modules[name], k, v)
    return sys.modules[name]


# Stub out azure.cli.core chain so __init__.py can import without azure-cli
_ensure_mock_module("azure")
_ensure_mock_module("azure.cli")
_ensure_mock_module("azure.cli.core")
_ensure_mock_module("azure.cli.core.commands")

# AzCommandsLoader stub
class _StubLoader:
    def __init__(self, *a, **kw):
        pass
setattr(sys.modules["azure.cli.core"], "AzCommandsLoader", _StubLoader)

# CliCommandType stub
class _StubCommandType:
    def __init__(self, *a, **kw):
        pass
setattr(sys.modules["azure.cli.core.commands"], "CliCommandType", _StubCommandType)

# knack stubs
_ensure_mock_module("knack")
_ensure_mock_module("knack.help_files", {"helps": {}})
_ensure_mock_module("knack.log")
_ensure_mock_module("knack.util")

def _get_logger(name):
    import logging
    return logging.getLogger(name)

setattr(sys.modules["knack.log"], "get_logger", _get_logger)

class _CLIError(Exception):
    pass
setattr(sys.modules["knack.util"], "CLIError", _CLIError)
