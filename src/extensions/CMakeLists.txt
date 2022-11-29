
if (NOT ADUC_UBUNTU_CORE_SNAP_ONLY)
    add_subdirectory (component_enumerators/examples/contoso_component_enumerator)
endif()

add_subdirectory (content_downloaders)
add_subdirectory (download_handlers)
add_subdirectory (extension_manager)
add_subdirectory (step_handlers)
add_subdirectory (update_manifest_handlers)
add_subdirectory (shared_lib)
