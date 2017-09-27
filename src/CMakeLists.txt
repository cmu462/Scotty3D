cmake_minimum_required(VERSION 2.8)

# Application source
set(APPLICATION_SOURCE

    # Collada Parser
    collada/collada.cpp
    collada/camera_info.cpp
    collada/light_info.cpp
    collada/sphere_info.cpp
    collada/polymesh_info.cpp
    collada/material_info.cpp

    # Dynamic Scene
    dynamic_scene/mesh.cpp
    dynamic_scene/scene.cpp
    dynamic_scene/sphere.cpp
    dynamic_scene/widgets.cpp
    dynamic_scene/skeleton.cpp
    dynamic_scene/joint.cpp

    # Static scene
    static_scene/sphere.cpp
    static_scene/triangle.cpp
    static_scene/object.cpp
    static_scene/environment_light.cpp
    static_scene/light.cpp

    # MeshEdit
    halfEdgeMesh.cpp
    meshEdit.cpp

    # PathTracer
    bvh.cpp
    bbox.cpp
    bsdf.cpp
    camera.cpp
    sampler.cpp
    pathtracer.cpp

    # Animator
    timeline.cpp

    # misc
    misc/sphere_drawing.cpp
    getopt.c
    error_dialog.cpp

    # Application
    application.cpp
    main.cpp
)

#-------------------------------------------------------------------------------
# Set include directories
#-------------------------------------------------------------------------------
include_directories(
  ${GLEW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${CMU462_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
)

#-------------------------------------------------------------------------------
# Set link directories
#-------------------------------------------------------------------------------
link_directories(
  ${GLEW_LIBRARY_DIRS}
  ${GLFW_LIBRARY_DIRS}
  ${CMU462_LIBRARY_DIRS}
  ${FREETYPE_LIBRARY_DIRS}
)

#-------------------------------------------------------------------------------
# Add executable
#-------------------------------------------------------------------------------
add_executable(scotty3d ${APPLICATION_SOURCE})

target_link_libraries( scotty3d
    CMU462 ${CMU462_LIBRARIES}
    glew ${GLEW_LIBRARIES}
    glfw ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${CMAKE_THREADS_INIT}
)

#-------------------------------------------------------------------------------
# Platform-specific configurations for target
#-------------------------------------------------------------------------------
if(APPLE)
  set_property( TARGET scotty3d APPEND_STRING PROPERTY COMPILE_FLAGS
                "-Wno-deprecated-declarations -Wno-c++11-extensions")
endif(APPLE)

# Put executable in build directory root
set(EXECUTABLE_OUTPUT_PATH ..)

# Install to project root
install(TARGETS scotty3d DESTINATION ${Scotty3D_SOURCE_DIR})

# Copy Freetype DLLs to the build directory
if(WIN32)
    foreach(FREETYPE_DLL ${FREETYPE_RUNTIMELIBS})
        add_custom_command(TARGET scotty3d POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FREETYPE_DLL}"
                $<TARGET_FILE_DIR:scotty3d>)
    endforeach(FREETYPE_DLL)
endif(WIN32)
