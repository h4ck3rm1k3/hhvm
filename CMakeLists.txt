CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
PROJECT(hphp C CXX)

SET( INCLUDE_DIR "/media/NewFoundSpace/usr/local/include/")
# boost checks


SET( BOOST_ROOT "/media/NewFoundSpace/usr/local/")
SET( BOOST_HOME "/media/NewFoundSpace/usr/local/")
SET( Boost_INCLUDE_DIR "/media/NewFoundSpace/usr/local/include/")
include_directories( "/media/NewFoundSpace/usr/local/include/")
include_directories( "/media/NewFoundSpace/usr/local/include/boost/")

find_package(Boost 1.37.0 COMPONENTS system;program_options;filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})


IF("$ENV{HPHP_HOME}" STREQUAL "")
	message(FATAL_ERROR "You should set the HPHP_HOME environmental")
ENDIF()

file(TO_CMAKE_PATH "$ENV{HPHP_HOME}" HPHP_HOME)

SET(CMAKE_MODULE_PATH "${HPHP_HOME}/CMake" ${CMAKE_MODULE_PATH})

include("${HPHP_HOME}/CMake/HPHPFunctions.cmake")
include(CheckFunctionExists)

IF(NOT EXISTS "src/third_party/libmbfl/mbfl/mbfilter.h")
	message(FATAL_ERROR "Please setup the git submodule to pull dependencies")
ENDIF()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

