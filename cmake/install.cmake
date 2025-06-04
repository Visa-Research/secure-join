

#############################################
#            Install                        #
#############################################

configure_file("${CMAKE_CURRENT_LIST_DIR}/findDependancies.cmake" "findDependancies.cmake" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/preamble.cmake" "preamble.cmake" COPYONLY)

# # make cache variables for install destinations
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)


# generate the config file that is includes the exports
configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/secureJoinConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/secureJoin
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

if(NOT DEFINED SECUREJOIN_VERSION_MAJOR)
    message("\n\n\n\n warning, SECUREJOIN_VERSION_MAJOR not defined ${SECUREJOIN_VERSION_MAJOR}")
endif()
set_property(TARGET secureJoin PROPERTY VERSION ${SECUREJOIN_VERSION})
# generate the version file for the config file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SECUREJOINConfigVersion.cmake"
  VERSION "${SECUREJOIN_VERSION_MAJOR}.${SECUREJOIN_VERSION_MINOR}.${SECUREJOIN_VERSION_PATCH}"
  COMPATIBILITY AnyNewerVersion
)

# install the configuration file
install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/secureJoinConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/findDependancies.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/preamble.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/secureJoin
)

# install library
install(
    TARGETS secureJoin
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    EXPORT secureJoinTargets)

# install headers
install(
    DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../secureJoin"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/"
    FILES_MATCHING PATTERN "*.h")


# install config
install(EXPORT secureJoinTargets
  FILE secureJoinTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/secureJoin
       NAMESPACE visa::
)
 export(EXPORT secureJoinTargets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/secureJoinTargets.cmake"
       NAMESPACE visa::
)

install(DIRECTORY secure-join DESTINATION include/secureJoin)

install(FILES 
    out/build/${SECUREJOIN_CONFIG}/secure-join/config.h   # Install the generated config.h
    DESTINATION include/secureJoin/secure-join
)