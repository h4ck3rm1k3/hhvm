#http://github.com/facebook/hiphop-php/commit/f88c396124e5a5a789ed9b358b5e9b35eeacf586
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.4 FATAL_ERROR)
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

if (BOOST_VERSION EQUAL 104200)
  #http://github.com/facebook/hiphop-php/commit/dd209e5e429b6039e27d72a764807eac950d4dfe
  # Boost bug #3942 prevents us using 1.42
  message("Warning:Boost 1.42 is not compatible with HipHop")
endif()



IF("$ENV{HPHP_HOME}" STREQUAL "")
	message(FATAL_ERROR "You should set the HPHP_HOME environmental")
ENDIF()

file(TO_CMAKE_PATH "$ENV{HPHP_HOME}" HPHP_HOME)

SET(CMAKE_MODULE_PATH "${HPHP_HOME}/CMake" ${CMAKE_MODULE_PATH})

include("${HPHP_HOME}/CMake/HPHPFunctions.cmake")
include(CheckFunctionExists)

IF(NOT EXISTS "src/third_party/libmbfl/mbfl/mbfilter.h")
#	message(FATAL_ERROR "Please setup the git submodule to pull dependencies")
ENDIF()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

