#
# Build system for mrMesh - makes building mrMesh across platforms much easier.
#
# Uses cmake to figure out code dependencies, i.e. what libraries and header files
# should be linkded against / included
#
# Author: Mike Bennett (mikemb@stanford.edu), 2011
#

cmake_minimum_required(VERSION 2.8)
project(MRMESH)


# Handy for seeing compile flags, included libraries, etc. Very useful when
# editing these build scripts and you want to understand what params are getting
# passed to compilers and linkers
#set(CMAKE_VERBOSE_MAKEFILE ON)


# Find an installed version of VTK
include(FindVTK REQUIRED)
find_package(VTK REQUIRED vtkRendering vtkGraphics vtkImaging vtkFiltering vtkCommon)

IF(VTK_FOUND)
	INCLUDE(${VTK_USE_FILE})
ELSE(VTK_FOUND)
	MESSAGE("ERROR: You need to install a version of VTK")
ENDIF(VTK_FOUND)


# Find location of wxWidgets
include(FindwxWidgets)
find_package(wxWidgets REQUIRED core base gl richtext aui xrc qa html adv xml net)

IF(wxWidgets_FOUND)
	INCLUDE(${wxWidgets_USE_FILE})
ELSE(wxWidgets_FOUND)
	MESSAGE("ERROR: You need to intall a version of wxWidgets")
ENDIF(wxWidgets_FOUND)


# General build flags, shared across all operating systems
SET(GEN_BUILD_FLAGS "-O3 -Wno-deprecated")


# What OS is this build happening on? Set some build wide OS specific build flags (if needed)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

	# On 64 bit Macs we've to build this as a 32 bit app
	MESSAGE(STATUS "Platform: MAC OS X")
	SET(CMAKE_SYSTEM_PROCESSOR i386)
	SET(CMAKE_OSX_ARCHITECTURES i386)
	SET(ARCH i386)
	SET(CMAKE_CXX_FLAGS "${GEN_BUILD_FLAGS}")

	# What params to use for compiling pnet mex (used to talk to mesh server)
	SET(MEX_FLAGS "")

	# Should build and not use addr.LocalHost()
	SET(HAVE_NO_SOCKET_LOCALHOST 1)

	# What Matlab build flags to set?
	SET(MATLAB_FLAGS "-maci")		# Compile in 32 bit mode (leads to 32 bit mex files)

ELSEIF(CYGWIN)

	# Compile on CYGWIN in Windows. I don't like hard code include
	#  but at the moment FindOpenGL.cmake doesn't set it correctly
	MESSAGE(STATUS "Platform: Windows with CYGWIN")
	SET(CMAKE_CXX_FLAGS "${GEN_BUILD_FLAGS} -D__USE_W32_SOCKETS -D_WIN32 -I/usr/include/opengl")

	# Forcing WIN32 means FindOpenGL.cmake will look for WIN32 CYGWIN
	# versions of opengl libraries (for linking against)
	SET(WIN32 1)
	SET(CMAKE_LEGACY_CYGWIN_WIN32 1)

	# What Matlab build flags to set?
	SET(MATLAB_FLAGS "")

	# What params to use for compiling pnet mex (used to talk to mesh server)
	SET(MEX_FLAGS wsock32.lib -DWIN32)

ELSEIF(UNIX)

	# Compile on Unix (Linux tested)
	MESSAGE(STATUS "Platform: Standard Unix flavour (prob Linux)")
	SET(CMAKE_CXX_FLAGS "${GEN_BUILD_FLAGS}")

	# What Matlab build flags to set?
	SET(MATLAB_FLAGS "")

	# What params to use for compiling pnet mex (used to talk to mesh server)
	SET(MEX_FLAGS "")

ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")



# Find OpenGL / Mesa
#
# Note: We put OpenGL find here as FindOpenGL.cmake's behaviour under CYGWIN
# doesn't behave properly without WIN32 being set (done in operating system check above)
find_package(OpenGL REQUIRED)


# Is this building with OpenGL or MESA?
INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES("OpenGL/gl.h;OpenGL/glu.h" HAVE_OPENGL_GLU_H)


# Process build instructions in main code dir
add_subdirectory(server)
add_subdirectory(tcpToolbox)
add_subdirectory(mex)
