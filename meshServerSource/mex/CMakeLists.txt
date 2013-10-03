#
# Build system for mex files for mrMesh - makes building mex files across platforms easier.
#       
# Uses cmake to figure out code dependencies, i.e. what libraries and header files
# should be linkded against / included
#       
# Author: Mike Bennett (mikemb@stanford.edu), 2011
#


INCLUDE_DIRECTORIES(${${PROJECT_NAME}_SOURCE_DIR})


# What OS is this build happening on? Set OS specific build flags for compiling mex's
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

	# What libs to link against for build_mesh.mex, curvature.mex & smooth_mesh.mex
	SET(MEX_MESHLIBS "-lvtkFiltering -lvtkCommon -lvtkGraphics -lvtksys")
	SET(MEX_VTKLIBDIRS "-L${VTK_LIBRARY_DIRS}")

ELSEIF(CYGWIN)

	# What libs to link against for build_mesh.mex, curvature.mex & smooth_mesh.mex
	#
	# NOTE: The libraries that are provided are built with VS2010, so your matlab
	#  mex -setup needs to be setup with that compiler, otherwise linking fails.
	SET(MEX_BITPATH "../../mex")

	# Should we get mex to link against 32 bit or 64 bit version of VTK libs?
	IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|i686")
		SET(MEX_MESHBIT "64")			# Link 64 bit libraries
	ELSE(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|i686")
		SET(MEX_MESHBIT "32")			# Link 32 bit libraries
	ENDIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|i686")

	MESSAGE(STATUS "Compiling ${MEX_MESHBIT} bit Matlab Mexs")

	SET(MEX_MESHLIBS "${MEX_BITPATH}/vtkFiltering${MEX_MESHBIT}.lib ${MEX_BITPATH}/vtkCommon${MEX_MESHBIT}.lib ${MEX_BITPATH}/vtkGraphics${MEX_MESHBIT}.lib ${MEX_BITPATH}/vtksys${MEX_MESHBIT}.lib")

	SET(MEX_VTKLIBDIRS "")
	SET(VTK_INCLUDE_DIRS "${MEX_BITPATH}/vtk_includes")

ELSEIF(UNIX)

	# What libs to link against for build_mesh.mex, curvature.mex & smooth_mesh.mex
	SET(MEX_MESHLIBS "-lvtkFiltering -lvtkCommon -lvtkGraphics -lvtksys")
	SET(MEX_VTKLIBDIRS "-L${VTK_LIBRARY_DIRS}")

ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


# Code files to compile
SET(CPPCODE_BUILDMESH ../../mex/build_mesh.cpp)
SET(CPPCODE_CURVATURE ../../mex/curvature.cpp)
SET(CPPCODE_SMOOTHMESH ../../mex/smooth_mesh.cpp)


# Binaries to build
ADD_CUSTOM_COMMAND(
	OUTPUT	build_mesh_mex
	COMMAND matlab -nodesktop -nosplash ${MATLAB_FLAGS} -r \"mex ${CPPCODE_BUILDMESH} \"${MEX_MESHLIBS}\" -I${VTK_INCLUDE_DIRS} ${MEX_VTKLIBDIRS}\;exit\"
)

ADD_CUSTOM_COMMAND(
	OUTPUT	curvature_mex
	COMMAND matlab -nodesktop -nosplash ${MATLAB_FLAGS} -r \"mex ${CPPCODE_CURVATURE} \"${MEX_MESHLIBS}\" -I${VTK_INCLUDE_DIRS} ${MEX_VTKLIBDIRS}\;exit\"
)

ADD_CUSTOM_COMMAND(
	OUTPUT	smooth_mesh_mex
	COMMAND matlab -nodesktop -nosplash ${MATLAB_FLAGS} -r \"mex ${CPPCODE_SMOOTHMESH} \"${MEX_MESHLIBS}\" -I${VTK_INCLUDE_DIRS} ${MEX_VTKLIBDIRS}\;exit\"
)


# Build binaries
ADD_CUSTOM_TARGET(build_mesh ALL echo DEPENDS build_mesh_mex)
ADD_CUSTOM_TARGET(curvature ALL echo DEPENDS curvature_mex)
ADD_CUSTOM_TARGET(smooth_mesh ALL echo DEPENDS smooth_mesh_mex)

