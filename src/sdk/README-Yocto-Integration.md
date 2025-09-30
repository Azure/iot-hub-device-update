# Example Yocto Recipe Usage for ADU SDK

## Overview
The `aducsdk` library provides a C ABI for external apps to query the Azure Device Update agent service status.
This allows querying of high-level status of the adu agent service to know if it's in the middle of processing
update deployments with some additional details as to the sub-status if busy.

## Yocto Recipe Integration

### 1. In your application's Yocto recipe (`myapp_1.0.bb`):

```bitbake
DESCRIPTION = "IoT Power Management Application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=..."

# Add dependency on the ADU SDK
DEPENDS += "aducsdk"

# Use pkg-config to get compilation flags
inherit pkgconfig

do_compile() {
    # pkg-config automatically provides the right flags
    ${CC} ${CFLAGS} $(pkg-config --cflags aducsdk) -o myapp main.c $(pkg-config --libs aducsdk)
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 myapp ${D}${bindir}/
}
```

### 2. Example application code (`main.c`):

```c
#include <stdio.h>
#include <unistd.h>
#include <aduc/aducsdk.h>

int main() {
    printf("Checking ADU Agent status...\n");

    ADUC_ServiceStatus status = GetAduServiceStatus();
    const char* statusStr = ADUC_ServiceStatusToString(status);

    printf("Status: %s (%d)\n", statusStr, status);

    if (status == ADUC_ServiceStatus_Idle || status == ADUC_ServiceStatus_Paused) {
        printf("Agent is idle/paused - safe to power down to conserve battery\n");
        // TODO: Initiate power down sequence here
    } else if (status >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion) {
        printf("Error communicating with agent: %s\n", statusStr);
        return 1;
    } else {
        printf("Agent is busy working on update deployment processing\n");
    }

    return 0;
}
```

## ADU SDK Integration in Yocto

### Installing the SDK Package
In a Yocto build, the ADU SDK will be built and installed as part of the `iot-hub-device-update` recipe, providing:

- **Library**: `/usr/lib/libaducsdk.a`
- **Header**: `/usr/include/aduc/aducsdk.h`
- **pkg-config**: `/usr/lib/pkgconfig/aducsdk.pc`

### Runtime Dependencies
The app would communicate with:
- **Service**: `deviceupdate-agent.service` (systemd service)
- **Executable**: `/usr/bin/AducIotAgent`
- **Named Pipes**: Under `/var/lib/adu/api/` for IPC

### Configuration Options

#### Basic Recipe Usage
```bitbake
# myapp_1.0.bb
DESCRIPTION = "IoT Device Power Management Application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=..."

# Depend on ADU SDK
DEPENDS += "iot-hub-device-update"

# Use pkg-config for compilation
inherit pkgconfig

SRC_URI = "file://main.c \
           file://LICENSE"

S = "${WORKDIR}"

do_compile() {
    # Use pkg-config to get proper compilation flags
    ${CC} ${CFLAGS} ${LDFLAGS} \
        $(pkg-config --cflags aducsdk) \
        -o myapp main.c \
        $(pkg-config --libs aducsdk)
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 myapp ${D}${bindir}/
}

FILES_${PN} = "${bindir}/myapp"
```

#### Custom SDK Configuration in Yocto
```bitbake
# In local.conf or machine configuration
# Set custom timeout (30 seconds instead of default 10)
EXTRA_OECMAKE_pn-iot-hub-device-update_append = " -DADUC_SDK_REQUEST_FIFO_TIMEOUT_SECS=30"

# Set custom FIFO path
EXTRA_OECMAKE_pn-iot-hub-device-update_append = " -DADUC_API_DEFAULT_FIFO_PATH='/opt/adu/api/request.fifo'"

# Multiple options together
EXTRA_OECMAKE_pn-iot-hub-device-update_append = " \
    -DADUC_SDK_REQUEST_FIFO_TIMEOUT_SECS=15 \
    -DADUC_API_DEFAULT_FIFO_PATH='/mnt/data/adu_api/apireq.fifo' \
"
```
