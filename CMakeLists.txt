# Root CMakeList.txt for libbbbpwm

set(PROJECT_NAME "libbbbpwm")

cmake_minimum_required(VERSION 2.8.11)
project(${PROJECT_NAME})

enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(UDev REQUIRED)

set(LIBBBBPWM_LIBRARY "bbbpwm")
set(LIBBBBPWM_INCLUDEDIR "${PROJECT_SOURCE_DIR}/include")
set(LIBBBBPWM_SRC "${PROJECT_SOURCE_DIR}/src")
set(LIBBBBPWM_TEST "${PROJECT_SOURCE_DIR}/test")
set(LIBBBBPWM_UTIL "${PROJECT_SOURCE_DIR}/util")
set(LIBBBBPWM_VERSION_MAJOR "0")
set(LIBBBBPWM_VERSION_MINOR "2")
set(LIBBBBPWM_VERSION_PATCH "0")
set(LIBBBBPWM_VERSION 
	"${LIBBBBPWM_VERSION_MAJOR}.${LIBBBBPWM_VERSION_MINOR}.${LIBBBBPWM_VERSION_PATCH}")

# CMake Install Directories
set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_INCLUDEDIR include)

# pkg-config directory
if(WIN32 AND NOT CYGWIN)
	set(DEF_PKGCONFIG_INSTALL_DIR pkgconfig)
else()
	set(DEF_PKGCONFIG_INSTALL_DIR lib/pkgconfig)
endif()
set(PKGCONFIG_INSTALL_DIR ${DEF_PKGCONFIG_INSTALL_DIR} CACHE PATH
  "Installation directory for pkg-config files")

# C flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O3")

# Source Code
add_subdirectory(${LIBBBBPWM_SRC})
set(LIBBBBPWM_SOURCE_FILES ${SOURCE_FILES})

# Headers
add_subdirectory(${LIBBBBPWM_INCLUDEDIR})
include_directories(${LIBBBBPWM_INCLUDEDIR})
set(LIBBBBPWM_HEADER_FILES ${HEADER_FILES})

include_directories(${udev_INCLUDEDIR})

# Build and link the library
add_library(${LIBBBBPWM_LIBRARY} SHARED ${LIBBBBPWM_SOURCE_FILES})
target_link_libraries(${LIBBBBPWM_LIBRARY} ${udev_LIBRARY})

# Tests
add_subdirectory(${LIBBBBPWM_TEST})

# Utilities
add_subdirectory(${LIBBBBPWM_UTIL})

# Config File Settings
set(LIBBBBPWM_INSTALL_PREFIX      "${CMAKE_INSTALL_PREFIX}")
set(LIBBBBPWM_INSTALL_EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBBBBPWM_INSTALL_LIBDIR 
	"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(LIBBBBPWM_INSTALL_INCLUDEDIR 
	"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")

# Configure Files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libbbbpwm.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libbbbpwm.pc @ONLY)

# Set install targets
install(TARGETS ${LIBBBBPWM_LIBRARY} 
	DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${LIBBBBPWM_HEADER_FILES}
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libbbbpwm.pc
	DESTINATION "${PKGCONFIG_INSTALL_DIR}")

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
# CPack Options
set(CPACK_PACKAGE_CONTACT "" CACHE STRING 
	"The contact information for the package distributor")
set(CPACK_TARGET_ARCH "armhf" CACHE STRING "Target architecture.")
set(CPACK_TARGET_OS "Linux" CACHE STRING "Target operating system.")
# CPack Settings 
SET(CPACK_GENERATOR "DEB")
SET(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README  "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VERSION_MAJOR "${LIBBBBPWM_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${LIBBBBPWM_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${LIBBBBPWM_VERSION_PATCH}")
SET(CPACK_SYSTEM_NAME "${CPACK_TARGET_OS}-${CPACK_TARGET_ARCH}")
SET(CPACK_TOPLEVEL_TAG "${CPACK_TARGET_OS}-${CPACK_TARGET_ARCH}")
include(CPack)
