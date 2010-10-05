#
# This file sets some defaults from your *source* directory, such that
# for each build directory you create, the same defaults are used.
#
# DO NOT EDIT THIS FILE ! Instead: copy it to orocos-ocl.cmake (in the
# top source directory) and edit that file to set your defaults.
#
# ie: cp orocos-ocl.default.cmake orocos-ocl.cmake ; edit orocos-ocl.cmake
#
# It will be included by the build system, right after the compiler has been
# detected (see top level CMakeLists.txt file)

#
# Uncomment to set additional include and library paths for: 
# Boost, Xerces, TAO, Omniorb etc.
#
# set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "C:/orocos/Boost-1_36_0/include")
# set(CMAKE_LIBRARY_PATH ${CMAKE_INCLUDE_PATH} "C:/orocos/Boost-1_36_0/lib")

#
# An option to make it easy to turn on all tests (defaults to OFF)
#
# option( BUILD_TESTS "Turn me off to disable compilation of all tests" ON )

#
# Set the target operating system. One of [lxrt gnulinux xenomai macosx win32]
# You may leave this as-is or force a certain target by removing the if... logic.
#
if (MSVC)
  set( OROCOS_TARGET win32 CACHE STRING "The Operating System target. One of [lxrt gnulinux xenomai macosx win32]")
elseif( APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
  set( OROCOS_TARGET macosx CACHE STRING "The Operating System target. One of [lxrt gnulinux xenomai macosx win32]")
else()
  set( OROCOS_TARGET gnulinux CACHE STRING "The Operating System target. One of [lxrt gnulinux xenomai macosx win32]")
endif()

# Useful for Windows/MSVC builds, sets all libraries and executables in one place.
#
# Uncomment to let output go to bin/ and libs/
#
# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin CACHE PATH "Bin directory")
# set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs CACHE PATH "Lib directory")
#
# And additional link directories.
#
# LINK_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/libs )






