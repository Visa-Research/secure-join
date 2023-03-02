

#############################################
#            Install                        #
#############################################


# configure_file("${CMAKE_CURRENT_LIST_DIR}/findDependancies.cmake" "findDependancies.cmake" COPYONLY)
# configure_file("${CMAKE_CURRENT_LIST_DIR}/preamble.cmake" "preamble.cmake" COPYONLY)

# # make cache variables for install destinations
# include(GNUInstallDirs)
# include(CMakePackageConfigHelpers)


# # generate the config file that is includes the exports
# configure_package_config_file(
#   "${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in"
#   "${CMAKE_CURRENT_BINARY_DIR}/SECUREJOINConfig.cmake"
#   INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SECUREJOIN
#   NO_SET_AND_CHECK_MACRO
#   NO_CHECK_REQUIRED_COMPONENTS_MACRO
# )

# if(NOT DEFINED SECUREJOIN_VERSION_MAJOR)
#     message("\n\n\n\n warning, SECUREJOIN_VERSION_MAJOR not defined ${SECUREJOIN_VERSION_MAJOR}")
# endif()

# set_property(TARGET SECUREJOIN PROPERTY VERSION ${SECUREJOIN_VERSION})

# # generate the version file for the config file
# write_basic_package_version_file(
#   "${CMAKE_CURRENT_BINARY_DIR}/SECUREJOINConfigVersion.cmake"
#   VERSION "${SECUREJOIN_VERSION_MAJOR}.${SECUREJOIN_VERSION_MINOR}.${SECUREJOIN_VERSION_PATCH}"
#   COMPATIBILITY AnyNewerVersion
# )

# # install the configuration file
# install(FILES
#           "${CMAKE_CURRENT_BINARY_DIR}/SECUREJOINConfig.cmake"
#           "${CMAKE_CURRENT_BINARY_DIR}/SECUREJOINConfigVersion.cmake"
#           "${CMAKE_CURRENT_BINARY_DIR}/findDependancies.cmake"
#           "${CMAKE_CURRENT_BINARY_DIR}/preamble.cmake"
#         DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SECUREJOIN
# )

# # install library
# install(
#     TARGETS SECUREJOIN
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     EXPORT SECUREJOINTargets)

# # install headers
# install(
#     DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../SECUREJOIN"
#     DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/"
#     FILES_MATCHING PATTERN "*.h")


# # install config
# install(EXPORT SECUREJOINTargets
#   FILE SECUREJOINTargets.cmake
#   DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SECUREJOIN
#        NAMESPACE visa::
# )
#  export(EXPORT SECUREJOINTargets
#        FILE "${CMAKE_CURRENT_BINARY_DIR}/SECUREJOINTargets.cmake"
#        NAMESPACE visa::
# )