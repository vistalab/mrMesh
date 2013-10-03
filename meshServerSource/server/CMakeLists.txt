#
# Build system for mrMesh - makes building mrMesh across platforms much easier.
#       
# Uses cmake to figure out code dependencies, i.e. what libraries and header files
# should be linkded against / included
#       
# Author: Mike Bennett (mikemb@stanford.edu), 2011
#

# Generate config.h file with conditionals for compiling 
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${${PROJECT_NAME}_SOURCE_DIR}/server/config.h)


INCLUDE_DIRECTORIES(${${PROJECT_NAME}_SOURCE_DIR})


# The libraries to link againist
SET(CORELIBS "${OPENGL_LIBRARIES}" "${wxWidgets_LIBRARIES}" "vtkHybrid")


# Code files to compile
SET(CPPCODE 3dview.cpp actor.cpp camera.cpp clientframe.cpp commandhandler.cpp cursor.cpp doublearray.cpp grayVolume.cpp helpers.cpp imagequad.cpp light.cpp matrix3x3.cpp mesh.cpp mrmeshsrvapp.cpp parametersmap.cpp polydatabuffer.cpp polyline.cpp progressindicator.cpp roi.cpp scene.cpp serverconsole.cpp stdafx.cpp text.cpp transformarrow.cpp vtkfilter.cpp vtkPolyDataSingleSourceShortestPath.cpp)


# Binary to build
ADD_EXECUTABLE(mrMeshSrv ${CPPCODE})


# What to link to build the binary, i.e. what libraries
TARGET_LINK_LIBRARIES(mrMeshSrv ${CORELIBS})
