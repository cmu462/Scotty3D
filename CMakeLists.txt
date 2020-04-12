cmake_minimum_required(VERSION 2.8)
project(Scotty3D)

#-------------------------------------------------------------------------------
# Build options
#-------------------------------------------------------------------------------
option(BUILD_LIBCMU462 "Build with libCMU462"         ON)
option(BUILD_DEBUG     "Build with debug settings"    OFF)
option(BUILD_DOCS      "Build documentation"          OFF)

#-------------------------------------------------------------------------------
# Platform-specific settings
#-------------------------------------------------------------------------------

###################
# Building on OSX #
###################
if(APPLE)

  # OSX Framework dependencies
  if(NOT BUILD_LIBCMU462)
    include_directories( "/System/Library/Frameworks" )
    find_library (COCOA_LIBRARIES Cocoa)
    find_library (IOKIT_LIBRARIES IOkit)
    find_library (COREVIDEO_LIBRARIES CoreVideo)
  endif()

  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)

  # Clang configuration
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    set(CLANG_CXX_FLAGS "-std=c++11 -m64")

    if(BUILD_DEBUG)
      set(CMAKE_BUILD_TYPE Debug)
    else(BUILD_DEBUG)
      set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -O3")
      set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -funroll-loops")
      set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -Wno-narrowing")
      set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -Wno-deprecated-register")
    endif(BUILD_DEBUG)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_CXX_FLAGS}")

  endif()

  # GCC configuration
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    set(GCC_CXX_FLAGS "-std=gnu++11 -m64")

    if(BUILD_DEBUG)
      set(CMAKE_BUILD_TYPE Debug)
    else(BUILD_DEBUG)
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -O3")
      set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -fopenmp")
    endif(BUILD_DEBUG)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_CXX_FLAGS}")

  endif()

endif(APPLE)

##################
# Build on Linux #
##################
set(LINUX UNIX AND NOT APPLE)
if(LINUX)

  # GCC configuration
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

    set(GCC_CXX_FLAGS "-std=gnu++11 -m64")

    # X11 Dependencies
    if(NOT BUILD_LIBCMU462)
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
    if(BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
    else(BUILD_DEBUG)
        set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -O3")
        set(GCC_CXX_FLAGS "${GCC_CXX_FLAGS} -fopenmp")
    endif(BUILD_DEBUG)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_CXX_FLAGS}")

  endif()

  # Clang configuration (experimental)
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  
      set(CLANG_CXX_FLAGS "-std=c++11 -m64")
  
      if(BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
      else(BUILD_DEBUG)
        set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -O3")
        set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -funroll-loops")
        set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -Wno-narrowing")
        set(CLANG_CXX_FLAGS "${CLANG_CXX_FLAGS} -Wno-deprecated-register")
      endif(BUILD_DEBUG)
  
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_CXX_FLAGS}")
  
  endif()

endif(LINUX)

####################
# Build on Windows #
####################
if(WIN32)

  if(MSVC)

    set(MSVC_CXX_FLAGS "-std=gnu++11")

    if(BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
    else(BUILD_DEBUG)
    endif(BUILD_DEBUG)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_CXX_FLAGS}")
    add_definitions(-D_USE_MATH_DEFINES)

  endif(MSVC)

  if(MINGW)

    set(MSVC_CXX_FLAGS "-std=gnu++11")

    if(BUILD_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
    else(BUILD_DEBUG)
    endif(BUILD_DEBUG)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_CXX_FLAGS}")

  endif(MINGW)

endif(WIN32)

#-------------------------------------------------------------------------------
# Find dependencies
#-------------------------------------------------------------------------------

# Required packages
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
if (NOT WIN32)
  find_package(Freetype REQUIRED)
endif ()

# CMU462
if(BUILD_LIBCMU462)
  add_subdirectory(CMU462)
  include_directories(CMU462/include)
else(BUILD_LIBCMU462)
  find_package(CMU462 REQUIRED)
  find_package(GLEW REQUIRED)
  find_package(GLFW REQUIRED)
endif(BUILD_LIBCMU462)

#-------------------------------------------------------------------------------
# Add subdirectories
#-------------------------------------------------------------------------------
add_subdirectory(src)

# build documentation
if(BUILD_DOCS)
  find_package(DOXYGEN)
  if(DOXYGEN_FOUND AND BUILD_DOCS)
    add_subdirectory(docs)
  endif()
endif()

# Install settings
set(CMAKE_INSTALL_PREFIX "${Scotty3D_SOURCE_DIR}/")
