PROJECT(glosm)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake"
                EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_configure(REQUIRES cpr/1.9.0 boost/1.81.0 jsoncpp/1.9.5
                      GENERATORS cmake_find_package)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
					BUILD missing
					REMOTE conancenter
					SETTINGS ${settings})

find_package(cpr REQUIRED)
find_package(Boost REQUIRED)
find_package(jsoncpp REQUIRED)

# Python port
find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})


#SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Global options
IF(UNIX)
	SET(BUILD_TILER_DEFAULT ON)
ELSE(UNIX)
	SET(BUILD_TILER_DEFAULT OFF)
ENDIF(UNIX)

IF(WIN32)
	SET(WITH_GLEW_DEFAULT ON)
ELSE(WIN32)
	SET(WITH_GLEW_DEFAULT OFF)
ENDIF(WIN32)

OPTION(BUILD_VIEWER_SDL "Build first-person osm viewer (SDL version)" ON)
OPTION(BUILD_VIEWER_GLUT "Build first-person osm viewer (GLUT version)" OFF)
OPTION(BUILD_TILER "Build tile generator" ${BUILD_TILER_DEFAULT})
OPTION(BUILD_EXAMPLES "Build examples" OFF)
OPTION(BUILD_TESTS "Build tests" ON)
OPTION(WITH_GLEW "Use GLEW (needed when you system uses archaic OpenGL)" ${WITH_GLEW_DEFAULT})
OPTION(WITH_GLES "Use OpenGL ES 1.1" OFF)
# TODO: this requires rewriting some legacy OpenGL code
#OPTION(WITH_GLES2 "Use OpenGL ES 2.0" OFF)
OPTION(WITH_TOUCHPAD "Tune control for touchpad instead of mouse" OFF)
OPTION(DEBUG_TILING "Render tile bounds for debugging" OFF)
OPTION(DEBUG_FPS "Don't limit FPS for profiling" OFF)

# Global variables (still overridable via cmake -D ..)
SET(BINDIR "bin" CACHE STRING "Install subdirectory for binaries")
SET(LIBDIR "lib" CACHE STRING "Install subdirectory for libraries")

# Conditionals
IF(MINGW)
	# Leave console till we have better way to display "Loading..." message
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
ENDIF(MINGW)

# TODO: these should be disabled if only server software is built
IF(WITH_GLES)
	FIND_PACKAGE(GLES REQUIRED)
	ADD_DEFINITIONS(-DWITH_GLES)
	SET(GL_INCLUDE_DIRS ${GLES_INCLUDE_DIR})
	SET(GL_LIBRARIES ${GLES_LIBRARY})
ELSEIF(WITH_GLES2)
	FIND_PACKAGE(GLES2 REQUIRED)
	ADD_DEFINITIONS(-DWITH_GLES2)
	SET(GL_INCLUDE_DIRS ${GLES2_INCLUDE_DIR})
	SET(GL_LIBRARIES ${GLES2_LIBRARY})
ELSE(WITH_GLES)
	FIND_PACKAGE(OpenGL REQUIRED)
	SET(GL_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR})
	SET(GL_LIBRARIES ${OPENGL_gl_LIBRARY})
ENDIF(WITH_GLES)

IF(WITH_GLEW)
	FIND_PACKAGE(GLEW REQUIRED)
	ADD_DEFINITIONS(-DWITH_GLEW)
	SET(GL_INCLUDE_DIRS ${GL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR})
	SET(GL_LIBRARIES ${GL_LIBRARIES} ${GLEW_LIBRARY})
ENDIF(WITH_GLEW)

# Global definitions
ADD_DEFINITIONS(-Wall -Wextra -Wno-unused-parameter)
IF(DEBUG_TILING)
	ADD_DEFINITIONS(-DDEBUG_TILING)
ENDIF(DEBUG_TILING)
IF(DEBUG_FPS)
	ADD_DEFINITIONS(-DDEBUG_FPS)
ENDIF(DEBUG_FPS)

# Info
MESSAGE(STATUS "  Building SDL viewer: ${BUILD_VIEWER_SDL}")
MESSAGE(STATUS " Building GLUT viewer: ${BUILD_VIEWER_GLUT}")
MESSAGE(STATUS "       Building tiler: ${BUILD_TILER}")
MESSAGE(STATUS "    Building examples: ${BUILD_EXAMPLES}")
MESSAGE(STATUS "       Building tests: ${BUILD_TESTS}")
MESSAGE(STATUS "")
MESSAGE(STATUS "         GLEW support: ${WITH_GLEW}")
MESSAGE(STATUS "OpenGL ES 1.1 support: ${WITH_GLES}")
#MESSAGE(STATUS "OpenGL ES 2.0 support: ${WITH_GLES2}")
MESSAGE(STATUS "     Touchpad support: ${WITH_TOUCHPAD}")

# Framework subdirs
ADD_SUBDIRECTORY(libglosm-server)
ADD_SUBDIRECTORY(libglosm-client)
ADD_SUBDIRECTORY(libglosm-geomgen)

# Application subdirs
IF(BUILD_VIEWER_SDL OR BUILD_VIEWER_GLUT)
	ADD_SUBDIRECTORY(viewer)
ENDIF(BUILD_VIEWER_SDL OR BUILD_VIEWER_GLUT)
IF(BUILD_TILER)
	ADD_SUBDIRECTORY(tiler)
ENDIF(BUILD_TILER)
IF(BUILD_EXAMPLES)
	ADD_SUBDIRECTORY(examples)
ENDIF(BUILD_EXAMPLES)
IF(BUILD_TESTS)
	ENABLE_TESTING()
	ADD_SUBDIRECTORY(tests)
ENDIF(BUILD_TESTS)
