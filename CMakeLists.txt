cmake_minimum_required(VERSION 2.8.12)
PROJECT(indi-astrolink4micro CXX C)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")


find_package(INDI REQUIRED)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

include(CMakeCommon)

################ AstroLink4 micro ################

set(indi_astrolink4micro_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/indi_astrolink4micro.cpp
   )

add_executable(indi_astrolink4micro ${indi_astrolink4micro_SRCS})
target_link_libraries(indi_astrolink4micro indidriver)
install(TARGETS indi_astrolink4micro RUNTIME DESTINATION bin )
install(FILES indi_astrolink4micro.xml DESTINATION ${INDI_DATA_DIR})

