cmake_minimum_required (VERSION 3.5)

# Note: When using component-based packaging, we should not set CPACK_PACKAGE_NAME
# at the global level, as it creates an additional unwanted package.
# Instead, we set component-specific package names below.

set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Device update agent")
set (CPACK_PACKAGE_VENDOR "Microsoft")
set (CPACK_PACKAGE_CONTACT "aduct@microsoft.com")

set (CPACK_PACKAGE_VERSION_MAJOR "${ADUC_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${ADUC_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${ADUC_VERSION_PATCH}")
set (CPACK_PACKAGE_VERSION "${ADUC_VERSION}")

set (CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# Debian
# See https://cmake.org/cmake/help/v3.3/module/CPackDeb.html for debian specific CPack variables.

if (NOT CPACK_GENERATOR)
    set (CPACK_GENERATOR "DEB")
else ()
    string (APPEND CPACK_GENERATOR ";DEB")
endif ()

set (CPACK_DEB_COMPONENT_INSTALL ON)

# Prevent creation of a monolithic package - only generate component-based packages
set (CPACK_MONOLITHIC_INSTALL OFF)

# Configure main agent package (Unspecified component)
set (CPACK_DEBIAN_UNSPECIFIED_PACKAGE_NAME "deviceupdate-agent")
set (CPACK_DEBIAN_UNSPECIFIED_FILE_NAME DEB-DEFAULT)

# See https://www.debian.org/doc/manuals/maint-guide/dreq.en.html#control for more information on these values for debian packages.

# '-' are not allowed in debian package versions except to separate the
# upstream_version from the debian_version. We translate our semantic version to
# the supported debian version schema by replacing '-' with '~'.
# See https://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-version
string (
    REPLACE "-"
            "~"
            CPACK_DEBIAN_PACKAGE_VERSION
            ${ADUC_VERSION})

# Common metadata for all packages (these don't trigger monolithic package creation)
set (CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Azure/iot-hub-device-update")
set (CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set (CPACK_DEBIAN_PACKAGE_SECTION "admin")

# Component-specific settings for main agent package (Unspecified component)
# Note: deliveryoptimization-agent creates DO user and group, that expected by preinst maintainer script.
# If remove deliveryoptimization-agent from dependencies, preinst script must be updated accordingly.

# See https://www.debian.org/doc/debian-policy/ch-relationships.html#s-binarydeps
if (ADUC_BUILD_WITH_DELIVERY_OPTIMIZATION)
    set (CPACK_DEBIAN_UNSPECIFIED_PACKAGE_DEPENDS "deliveryoptimization-agent (>= 1.0.0), libdeliveryoptimization (>= 1.0.0), curl")
    set (CPACK_DEBIAN_UNSPECIFIED_PACKAGE_SUGGESTS "deliveryoptimization-plugin-apt")
else ()
    set (CPACK_DEBIAN_UNSPECIFIED_PACKAGE_DEPENDS "curl")
endif ()

# Use dpkg-shlibdeps to generate better package dependency list.
set (CPACK_DEBIAN_UNSPECIFIED_PACKAGE_SHLIBDEPS ON)

set (
    CPACK_DEBIAN_UNSPECIFIED_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/debian/preinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm"
)

#
# Delta Handler Package Configuration
#
if (ADUC_BUILD_DELTA_HANDLER)
    # Configure the delta handler package as a separate component
    set (CPACK_COMPONENT_DELTAHANDLER_DISPLAY_NAME "Azure Device Update Agent - Delta Download Handler")
    set (CPACK_COMPONENT_DELTAHANDLER_DESCRIPTION
         "Microsoft Delta Download Handler for Azure Device Update Agent with differential update support")
    set (CPACK_COMPONENT_DELTAHANDLER_DEPENDS "deviceupdate-agent")

    # Set package name for delta handler component
    set (CPACK_DEBIAN_DELTAHANDLER_PACKAGE_NAME "deviceupdate-agent-delta-handler")
    set (CPACK_DEBIAN_DELTAHANDLER_FILE_NAME DEB-DEFAULT)

    # Delta handler package dependencies
    # Note: This package depends on the main agent and any delta library runtime dependencies
    set (CPACK_DEBIAN_DELTAHANDLER_PACKAGE_DEPENDS
         "deviceupdate-agent (>= ${CPACK_PACKAGE_VERSION}), zlib1g, libzstd1, libbz2-1.0, libssl3 | libssl1.1, libjsoncpp25 | libjsoncpp1"
    )

    # Use custom control scripts for delta handler
    set (
        CPACK_DEBIAN_DELTAHANDLER_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_SOURCE_DIR}/debian-delta/preinst;${CMAKE_CURRENT_SOURCE_DIR}/debian-delta/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian-delta/prerm;${CMAKE_CURRENT_SOURCE_DIR}/debian-delta/postrm"
    )

    # Install the delta handler library to extensions directory
    install (
        FILES ${CMAKE_BINARY_DIR}/lib/libmicrosoft_delta_download_handler.so
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/../lib/adu/extensions/sources
        COMPONENT deltahandler
    )

    # Install delta library dependencies from system installation
    # The delta library should be installed via install-deps.sh --install-delta
    # which installs to /usr/lib or /usr/local/lib

    # Find all delta library files in both /usr/lib and /usr/local/lib
    file (
        GLOB DELTA_LIB_FILES
        "/usr/lib/libadudiffapi.so*"
        "/usr/local/lib/libadudiffapi.so*"
    )

    if (DELTA_LIB_FILES)
        message (STATUS "Found delta library files: ${DELTA_LIB_FILES}")

        # Install the delta library itself
        install (
            FILES ${DELTA_LIB_FILES}
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/adu-delta
            COMPONENT deltahandler
        )
    else ()
        message (WARNING "Delta library (libadudiffapi.so) not found in /usr/lib or /usr/local/lib")
        message (WARNING "The delta handler package will not include the delta library.")
        message (WARNING "To fix this, either:")
        message (WARNING "  1. Install the delta library package: sudo dpkg -i ms-adu_diffs_*.deb")
        message (WARNING "  2. Run: ./scripts/install-deps.sh --install-delta")
        message (WARNING "  3. Manually copy: sudo cp ~/.adu-tmp/iot-hub-device-update-delta/src/out/native/x64-linux/Debug/bin/libadudiffapi.so* /usr/lib/")
        message (WARNING "Then reconfigure and rebuild the package.")
    endif ()

    # Note: Other dependencies (zlib, zstd, bzip2, openssl, jsoncpp) are typically
    # system libraries and should be declared as package dependencies rather than bundled

    message (STATUS "Delta handler package configuration enabled")
endif ()

# Set components to package
# The main agent uses the default "Unspecified" component
# The delta handler uses the "deltahandler" component
if (ADUC_BUILD_DELTA_HANDLER)
    set (CPACK_COMPONENTS_ALL Unspecified deltahandler)
else ()
    set (CPACK_COMPONENTS_ALL Unspecified)
endif ()

include (CPack)
