cmake_minimum_required (VERSION 3.5)

# Note: adu-swupdate.sh was removed - microsoft/swupdate:1 handler is deprecated
# Use microsoft/swupdate:2 with yocto-a-b-update.sh instead

# Install adu-reboot-wrapper.sh - required by adu-shell for synchronized reboot
# The script waits for the agent to finish cleanup before triggering reboot
install(
    PROGRAMS adu-reboot-wrapper.sh
    DESTINATION /usr/lib/adu
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
)
