"""Parameter definitions for adu device commands."""


def load_arguments(self, _):
    with self.argument_context("adu device") as c:
        c.argument(
            "socket_path",
            options_list=["--socket-path"],
            help="Path to the ADU agent Unix domain socket.",
            default="/run/adu/adu.sock",
        )

    with self.argument_context("adu device scan") as c:
        c.argument(
            "wait",
            options_list=["--wait"],
            action="store_true",
            default=False,
            help="Wait for the scan to complete before returning.",
        )
        c.argument(
            "timeout",
            options_list=["--timeout"],
            type=int,
            default=60,
            help="Timeout in seconds when using --wait.",
        )

    with self.argument_context("adu device cancel") as c:
        c.argument(
            "force",
            options_list=["--force"],
            action="store_true",
            default=False,
            help="Force cancel even if the update is in a critical phase.",
        )

    with self.argument_context("adu device logs collect") as c:
        c.argument(
            "output_path",
            options_list=["--output-path", "-o"],
            help="File path for the output .tar.gz diagnostic bundle.",
            default=None,
        )
        c.argument(
            "since",
            options_list=["--since"],
            help="Collect logs since this time (e.g. '1h', '2d', '2024-01-01').",
            default=None,
        )
        c.argument(
            "include_core_dumps",
            options_list=["--include-core-dumps"],
            action="store_true",
            default=False,
            help="Include core dump files in the bundle.",
        )
        c.argument(
            "ssh_host",
            options_list=["--ssh-host"],
            help="SSH hostname or IP for remote log collection.",
            default=None,
        )
        c.argument(
            "ssh_user",
            options_list=["--ssh-user"],
            help="SSH username for remote log collection.",
            default=None,
        )
        c.argument(
            "ssh_key",
            options_list=["--ssh-key"],
            help="Path to SSH private key for remote log collection.",
            default=None,
        )

    with self.argument_context("adu device logs upload") as c:
        c.argument(
            "bundle_path",
            options_list=["--bundle-path"],
            help="Path to the diagnostic bundle .tar.gz file.",
            required=True,
        )
        c.argument(
            "storage_account",
            options_list=["--storage-account"],
            help="Azure Storage account name.",
            required=True,
        )
        c.argument(
            "container",
            options_list=["--container"],
            help="Blob container name.",
            required=True,
        )
        c.argument(
            "sas_token",
            options_list=["--sas-token"],
            help="SAS token for authentication to Azure Blob Storage.",
            required=True,
        )

    with self.argument_context("adu device config show") as c:
        c.argument(
            "section",
            options_list=["--section"],
            help="Filter output to a specific configuration section.",
            default=None,
        )
        c.argument(
            "config_path",
            options_list=["--config-path"],
            help="Path to the agent configuration file.",
            default="/etc/adu/agent.toml",
        )
