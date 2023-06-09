# Common depends
IF(WITH_TOUCHPAD)
	ADD_DEFINITIONS(-DWITH_TOUCHPAD)
ENDIF(WITH_TOUCHPAD)

IF(BUILD_VIEWER_SDL)
	# Depends
	FIND_PACKAGE(SDL REQUIRED)
	find_package(cpr REQUIRED)
	find_package(Boost REQUIRED)

	IF(WITH_GLES OR WITH_GLES2)
		FIND_PACKAGE(SDL_gles REQUIRED)
	ENDIF(WITH_GLES OR WITH_GLES2)

	# Targets
	SET(SOURCES
		SDLMain.cc
		GlosmViewer.cc
	)

	INCLUDE_DIRECTORIES(
		${OPENGL_INCLUDE_DIR}
		${SDL_INCLUDE_DIR}
		${SDLGLES_INCLUDE_DIR}
		${GL_INCLUDE_DIRS}
		../libglosm-client
		../libglosm-server
		../libglosm-geomgen
	)

	ADD_EXECUTABLE(glosm-viewer-sdl ${SOURCES})
	TARGET_LINK_LIBRARIES(glosm-viewer-sdl
		glosm-client
		glosm-server
		glosm-geomgen
		${SDL_LIBRARY}
		${SDLGLES_LIBRARY}
		${GL_LIBRARIES}
		${CONAN_LIBS}
		cpr::cpr
		Boost::boost
		jsoncpp::jsoncpp
		${PYTHON_LIBRARIES}
	)

	# Installation
	INSTALL(TARGETS glosm-viewer-sdl RUNTIME DESTINATION ${BINDIR})
ENDIF(BUILD_VIEWER_SDL)

IF(BUILD_VIEWER_GLUT)
	# Depends
	
	FIND_PACKAGE(GLUT REQUIRED)
	find_package(cpr REQUIRED)
	find_package(Boost REQUIRED)
	
	# Targets
	SET(SOURCES
		GLUTMain.cc
		GlosmViewer.cc
	)

	INCLUDE_DIRECTORIES(
		${OPENGL_INCLUDE_DIR}
		${GLUT_INCLUDE_DIR}
		${GL_INCLUDE_DIRS}
		../libglosm-client
		../libglosm-server
		../libglosm-geomgen
	)

	ADD_EXECUTABLE(glosm-viewer-glut ${SOURCES})
	TARGET_LINK_LIBRARIES(glosm-viewer-glut
		glosm-client
		glosm-server
		glosm-geomgen
		${OPENGL_LIBRARIES}
		${GLUT_LIBRARIES}
		${GL_LIBRARIES}
		${CONAN_LIBS}
		cpr::cpr
		Boost::boost
		jsoncpp::jsoncpp
		${PYTHON_LIBRARIES}
	)

	# Installation
	INSTALL(TARGETS glosm-viewer-glut RUNTIME DESTINATION ${BINDIR})
ENDIF(BUILD_VIEWER_GLUT)
