cmake_minimum_required (VERSION 3.5)

set (CPACK_PACKAGE_NAME "adu-agent")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Azure Device Update Client")
set (CPACK_PACKAGE_VENDOR "ADU Client Devs")
# TODO: consider creating an email alias or link to github team
set (CPACK_PACKAGE_CONTACT "ADU Client Devs")

set (CPACK_PACKAGE_VERSION_MAJOR "${ADUC_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${ADUC_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${ADUC_VERSION_PATCH}")
set (CPACK_PACKAGE_VERSION "${ADUC_VERSION}")

# Debian
# See https://cmake.org/cmake/help/v3.3/module/CPackDeb.html for debian specific CPack variables.

if (NOT CPACK_GENERATOR)
    set (CPACK_GENERATOR "DEB")
else ()
    string (APPEND CPACK_GENERATOR ";DEB")
endif ()

set (CPACK_DEB_COMPONENT_INSTALL ON)

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

set (CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Azure/adu-private-preview")
set (CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set (CPACK_DEBIAN_PACKAGE_SECTION "admin")

# Note: deliveryoptimization-agent creates DO user and group, that expected by preinst maintainer script.
# If remove deliveryoptimization-agent from dependencies, preinst script must be updated accordingly.

# See https://www.debian.org/doc/debian-policy/ch-relationships.html#s-binarydeps
set (CPACK_DEBIAN_PACKAGE_DEPENDS "deliveryoptimization-agent, libdeliveryoptimization, libcpprest")
set (CPACK_DEBIAN_PACKAGE_SUGGESTS "deliveryoptimization-plugin-apt")

# Use dpkg-shlibdeps to generate better package dependency list.
set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set (
    CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/debian/preinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/prerm;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm"
)

include (CPack)
