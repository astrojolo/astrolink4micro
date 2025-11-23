cmake_minimum_required(VERSION 3.10)
project(indi-astrolink4micro VERSION 0.1 LANGUAGES CXX C)

# Preserve old-style version variables for consumers that might expect them
set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VERSION_MINOR ${PROJECT_VERSION_MINOR})

# Add project's cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

# Use the GNUInstallDirs module for standard install dir variables
include(GNUInstallDirs)
include(CMakeCommon)  # keep including the project's helper, if present

# Prefer a modern, explicit C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find INDI (must provide INDI_INCLUDE_DIR and INDI_DATA_DIR)
find_package(INDI REQUIRED)

# Sources
set(indi_astrolink4micro_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/indi_astrolink4micro.cpp
)

# Executable
add_executable(indi_astrolink4micro ${indi_astrolink4micro_SRCS})

# Modern target-based include/link usage
target_include_directories(indi_astrolink4micro PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${INDI_INCLUDE_DIR}
)

target_link_libraries(indi_astrolink4micro PRIVATE indidriver)

# Install rules using GNUInstallDirs
install(TARGETS indi_astrolink4micro
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the device xml file into the INDI data directory
install(FILES indi_astrolink4micro.xml DESTINATION ${INDI_DATA_DIR})
