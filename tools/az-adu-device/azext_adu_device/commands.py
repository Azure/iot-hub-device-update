"""Command group and command registration for adu device extension."""


def load_command_table(self, _):
    with self.command_group("adu device", is_experimental=True) as g:
        g.custom_command("status", "get_status")
        g.custom_command("health", "get_health")
        g.custom_command("scan", "trigger_scan")
        g.custom_command("cancel", "cancel_update")
        g.custom_command("config show", "config_show")
        g.custom_command("extensions list", "extensions_list")
        g.custom_command("update history", "get_update_history")

    with self.command_group("adu device logs", is_experimental=True) as g:
        g.custom_command("collect", "logs_collect")
        g.custom_command("upload", "logs_upload")
