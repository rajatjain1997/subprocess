if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_INSTALL_INCLUDEDIR include/subprocess CACHE PATH "")
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package subprocess)

install(
    DIRECTORY
    include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT subprocess_Development
)

install(
    TARGETS subprocess_subprocess
    EXPORT subprocessTargets
    RUNTIME #
    COMPONENT subprocess_Runtime
    LIBRARY #
    COMPONENT subprocess_Runtime
    NAMELINK_COMPONENT subprocess_Development
    ARCHIVE #
    COMPONENT subprocess_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    subprocess_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(subprocess_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${subprocess_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT subprocess_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${subprocess_INSTALL_CMAKEDIR}"
    COMPONENT subprocess_Development
)

install(
    EXPORT subprocessTargets
    NAMESPACE subprocess::
    DESTINATION "${subprocess_INSTALL_CMAKEDIR}"
    COMPONENT subprocess_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
