#
# Build system for tcpToolbox files for mrMesh - makes building mex files across
# platforms easier.
#       
# Uses cmake to figure out code dependencies, i.e. what libraries and header files
# should be linkded against / included. Then run matlab's mex compiler.
#       
# Author: Mike Bennett (mikemb@stanford.edu), 2011
#


INCLUDE_DIRECTORIES(${${PROJECT_NAME}_SOURCE_DIR})


# Code files to compile
SET(CCODE_PNET ../../tcpToolbox/pnet.c)


# Binaries to build
ADD_CUSTOM_COMMAND(
	OUTPUT	pnet_mex
	COMMAND matlab -nodesktop -nosplash ${MATLAB_FLAGS} -r \"mex ${CCODE_PNET} ${MEX_FLAGS}\;exit\"
)


# Build binaries
ADD_CUSTOM_TARGET(pnet ALL echo DEPENDS pnet_mex)

