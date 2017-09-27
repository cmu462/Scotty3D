cmake_minimum_required(VERSION 2.8)
project(CMU462)

#-------------------------------------------------------------------------------
# Build options
#-------------------------------------------------------------------------------
option(CMU462_BUILD_DEBUG    "Build for debug"          OFF)
option(CMU462_BUILD_SHARED   "Build shared libraries"   OFF)
option(CMU462_BUILD_DOCS     "Build documentation"      OFF)
option(CMU462_BUILD_TESTS    "Build tests programs"     OFF)
option(CMU462_BUILD_EXAMPLES "Build examples"           OFF)

#-------------------------------------------------------------------------------
# CMake modules
#-------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMU462_SOURCE_DIR}/cmake/modules/")

#-------------------------------------------------------------------------------
# CMU462 paths
#-------------------------------------------------------------------------------
set(CMU462_INCLUDE_DIRS "${CMU462_SOURCE_DIR}/include")

#-------------------------------------------------------------------------------
# Platform-specific settings
#-------------------------------------------------------------------------------

###################
# Building on OSX #
###################
if(APPLE)

  # Clang only
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "CLANG")

    # OSX Framework dependencies
    if(NOT CMU462_BUILD_SHARED)
      include_directories( "/System/Library/Frameworks" )
      find_library (COCOA_LIBRARIES Cocoa)
      find_library (IOKIT_LIBRARIES IOkit)
      find_library (COREVIDEO_LIBRARIES CoreVideo)
    endif()

    # Clang configuration
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")

      set(CLANG_CXX_FLAGS "-std=c++11 -m64")

      if(CMU462_BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
      else(CMU462_BUILD_DEBUG)
        set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -O3")
        set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -funroll-loops")
        set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -Wno-narrowing")
        set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -Wno-deprecated-register")
      endif(CMU462_BUILD_DEBUG)

      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_CXX_FLAGS}")

    endif()

    # GCC configuration
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

      set(GCC_CXX_FLAGS "-std=gnu++11 -m64")

      if(CMU462_BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
      else(CMU462_BUILD_DEBUG)
        set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -O3")
        set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -fopenmp")
      endif(CMU462_BUILD_DEBUG)

      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_CXX_FLAGS}")

    endif()

  endif()

endif(APPLE)

##################
# Build on Linux #
##################
set(LINUX UNIX AND NOT APPLE)
if(LINUX)

  # GCC only
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

    set(GCC_CXX_FLAGS "-std=gnu++11 -m64")

    # X11 Dependencies
    if(NOT CMU462_BUILD_SHARED)
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXi")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXxf86vm")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXinerama")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXcursor")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXfixes")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXrandr")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXext")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXrender")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lX11")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lpthread")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lxcb")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -lXau")
    endif()

    # Debug configuration
    if(CMU462_BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
    else(CMU462_BUILD_DEBUG)
        set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -O3")
        set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -fopenmp")
    endif(CMU462_BUILD_DEBUG)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_CXX_FLAGS}")

  endif()

endif(LINUX)

####################
# Build on Windows #
####################
if(WIN32)

  if(MSVC)

    set(MSVC_CXX_FLAGS "-std=gnu++11")

    if(CMU462_BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
    else(CMU462_BUILD_DEBUG)
    endif(CMU462_BUILD_DEBUG)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_CXX_FLAGS}")

    add_subdirectory("${CMU462_SOURCE_DIR}/deps/freetype")

  endif(MSVC)

  if(MINGW)

    set(MSVC_CXX_FLAGS "-std=gnu++11")

    if(CMU462_BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
    else(CMU462_BUILD_DEBUG)
    endif(CMU462_BUILD_DEBUG)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_CXX_FLAGS}")

  endif(MINGW)

endif(WIN32)

#-------------------------------------------------------------------------------
# Find dependencies
#-------------------------------------------------------------------------------

# Required packages
find_package(OpenGL REQUIRED)
if(NOT WIN32)
  find_package(Freetype REQUIRED)
endif()

# Use pkg-config for GLEW & GLFW if available
find_package(PkgConfig QUIET)

# GLEW
if(PKGCONFIG_FOUND)
  pkg_search_module(GLEW QUIET GLEW)
else(PKGCONFIG_FOUND)
  find_package(GLEW QUIET)
endif()
if(NOT GLEW_FOUND)
  add_subdirectory("${CMU462_SOURCE_DIR}/deps/glew")
  set_property( TARGET glew APPEND_STRING PROPERTY COMPILE_FLAGS -w )
endif()

# GLFW
if(PKGCONFIG_FOUND)
  pkg_search_module(GLFW QUIET glfw3)
else(PKGCONFIG_FOUND)
  find_package(GLFW QUIET)
endif()
if(NOT GLFW_FOUND)
  add_subdirectory("${CMU462_SOURCE_DIR}/deps/glfw")
  set_property( TARGET glfw APPEND_STRING PROPERTY COMPILE_FLAGS -w )
endif()

#-------------------------------------------------------------------------------
# Set include directories
#-------------------------------------------------------------------------------
include_directories(
  ${CMU462_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
)

#-------------------------------------------------------------------------------
# Set link directories
#-------------------------------------------------------------------------------
link_directories(
  ${GLEW_LIBRARY_DIRS}
  ${GLFW_LIBRARY_DIRS}
  ${FREETYPE_LIBRARY_DIRS}
)

#-------------------------------------------------------------------------------
# Add subdirectories
#-------------------------------------------------------------------------------

# CMU462 library source directory
add_subdirectory(src)

# CMU462 tests source directory
if(CMU462_BUILD_TESTS)
  add_subdirectory(tests)
endif()

# CMU462 exmaples source directory
if(CMU462_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# CMU462 documentation directory
if(CMU462_BUILD_DOCS)
  find_package(DOXYGEN)
  if(DOXYGEN_FOUND AND CMU462_BUILD_DOCS)
    add_subdirectory(docs)
  endif()
endif()

#-------------------------------------------------------------------------------
# Packing
#-------------------------------------------------------------------------------

# Install settings
set(CMAKE_INSTALL_PREFIX "${CMU462_SOURCE_DIR}")
