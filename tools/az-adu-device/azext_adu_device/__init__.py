"""Azure CLI extension for ADU device-side agent management."""

from azure.cli.core import AzCommandsLoader
from azext_adu_device._help import helps  # noqa: F401


class ADUDeviceCommandsLoader(AzCommandsLoader):
    """Command loader for the adu device extension."""

    def __init__(self, cli_ctx=None):
        from azure.cli.core.commands import CliCommandType

        adu_device_custom = CliCommandType(
            operations_tmpl="azext_adu_device.custom#{}"
        )
        super().__init__(cli_ctx=cli_ctx, custom_command_type=adu_device_custom)

    def load_command_table(self, args):
        from azext_adu_device.commands import load_command_table

        load_command_table(self, args)
        return self.command_table

    def load_arguments(self, command):
        from azext_adu_device._params import load_arguments

        load_arguments(self, command)


COMMAND_LOADER_CLS = ADUDeviceCommandsLoader
