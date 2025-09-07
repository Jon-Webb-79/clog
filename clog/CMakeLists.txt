# ================================================================================
# ================================================================================
# - File:    CMakeLists.txt
# - Purpose: Primary CMake file for C build system
#
# Source Metadata
# - Author:  Name
# - Date:    Month Day, Year
# - Version: 1.0
# - Copyright: Copyright Year, Name Inc.
# ================================================================================
# ================================================================================
cmake_minimum_required(VERSION 4.1.1)
cmake_minimum_required(VERSION 3.20)

project(clog VERSION 0.1.0 LANGUAGES C)

# ---- Options ---------------------------------------------------------------

option(LOGGER_BUILD_STATIC "Build static logger library" ON)
option(LOGGER_BUILD_SHARED "Build shared logger library" OFF)
option(LOGGER_BUILD_TESTS  "Build unit tests (CMocka)" OFF)
option(LOGGER_INSTALL      "Install headers and libraries" ON)

# ---- Globals ---------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Useful default if not provided
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---- Sources / includes ----------------------------------------------------

set(LOGGER_PUBLIC_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/logger.h
)
set(LOGGER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/logger.c
)

# Threads (pthreads on POSIX; harmless on Windows)
find_package(Threads REQUIRED)

# ---- Libraries -------------------------------------------------------------

# Helper to set common props
function(_logger_common target)
  target_include_directories(${target}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
  target_link_libraries(${target}
    PUBLIC Threads::Threads
  )
  target_compile_features(${target} PRIVATE c_std_11)
  target_compile_options(${target} PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<C_COMPILER_ID:MSVC>:/W4>
  )
endfunction()

if(LOGGER_BUILD_STATIC)
  add_library(logger_static STATIC ${LOGGER_SOURCES})
  set_target_properties(logger_static PROPERTIES OUTPUT_NAME logger)
  _logger_common(logger_static)
endif()

if(LOGGER_BUILD_SHARED)
  add_library(logger_shared SHARED ${LOGGER_SOURCES})
  set_target_properties(logger_shared PROPERTIES OUTPUT_NAME logger)
  _logger_common(logger_shared)
endif()

if(NOT LOGGER_BUILD_STATIC AND NOT LOGGER_BUILD_SHARED)
  message(FATAL_ERROR "At least one of LOGGER_BUILD_STATIC or LOGGER_BUILD_SHARED must be ON")
endif()

# ---- Install ---------------------------------------------------------------

if(LOGGER_INSTALL)
  include(GNUInstallDirs)

  if(TARGET logger_static)
    install(TARGETS logger_static
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endif()

  if(TARGET logger_shared)
    install(TARGETS logger_shared
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endif()

  install(FILES ${LOGGER_PUBLIC_HEADERS}
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# ---- Tests (CMocka) --------------------------------------------------------

if(LOGGER_BUILD_TESTS)
  enable_testing()

  # Try pkg-config first (common on Linux/macOS)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(CMOCKA QUIET cmocka)
  endif()

  # Fallback manual find (works on many setups)
  if(NOT CMOCKA_FOUND)
    find_library(CMOCKA_LIBRARY NAMES cmocka)
    find_path(CMOCKA_INCLUDE_DIR NAMES cmocka.h)
    if(CMOCKA_LIBRARY AND CMOCKA_INCLUDE_DIR)
      set(CMOCKA_LIBRARIES ${CMOCKA_LIBRARY})
      set(CMOCKA_INCLUDE_DIRS ${CMOCKA_INCLUDE_DIR})
      set(CMOCKA_FOUND TRUE)
    endif()
  endif()

  if(NOT CMOCKA_FOUND)
    message(FATAL_ERROR "CMocka not found. Install cmocka or provide CMOCKA_INCLUDE_DIRS/CMOCKA_LIBRARIES.")
  endif()

  add_executable(unit_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_log.c
  )
  target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    ${CMOCKA_INCLUDE_DIRS}
  )
  if(TARGET logger_static)
    target_link_libraries(unit_tests PRIVATE logger_static)
  elseif(TARGET logger_shared)
    target_link_libraries(unit_tests PRIVATE logger_shared)
  endif()
  target_link_libraries(unit_tests PRIVATE ${CMOCKA_LIBRARIES} Threads::Threads)

  add_test(NAME logger_unit_tests COMMAND unit_tests)
endif()

# ================================================================================
# ================================================================================
# eof
