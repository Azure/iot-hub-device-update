cmake_minimum_required (VERSION 3.5)

install (
    FILES adu-agent.service
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system
    PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        GROUP_READ
        WORLD_READ)

if (ADUC_REGISTER_DAEMON)
    install (
        CODE "execute_process (COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/install.sh ${ADUC_CONF_FOLDER} ${ADUC_CONF_FILE} ${ADUC_HOME_FOLDER} ${ADUC_LOG_FOLDER} ${ADUC_DATA_FOLDER})"
    )
endif ()
