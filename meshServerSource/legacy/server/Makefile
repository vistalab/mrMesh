#
# Makefile for mrMeshSrv
#
# Links against wxWidgets and VTK. These two libraries must be installed
# on your machine for a successful compilation and linking
#
# 2011.06.17 LMP modified Roozbeh Kiani's makefile which was
# based on an earlier version written by RFD
# 
# 	make		optimizes for speed
#	make DEBUG=1	adds debugging code 
# 	make tidy	removes *.o files
#	make clean	removed *.o and executable files 

# wxGadgets libs,			wx-config --libs

# WX_LIBS := $(shell wx-config --libs gl net)
WX_LIBS := $(shell ../wx/wx-config --libs gl net)

# wxGadgets headers and flags,		wx-config --cxxflags
# WX_CXXFLAGS = $(shell wx-config --cxxflags)
WX_CXXFLAGS = $(shell ../wx/wx-config --cxxflags)

# VTK headers (one of these should work)
#VTK_INCLUDES := -I/usr/include/vtk/ 
 VTK_INCLUDES := -I../local/include/vtk-5.8/

# VTK libs	stored at different places depending on 32 or 64 bit OS   
LBITS = $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)	# x86_64 machines
#   VTK_LIB_DIR = -L/usr/lib64
    VTK_LIB_DIR = -L$../local/liba/
else			# i386-i686 machines 
    VTK_LIB_DIR = -L/usr/lib/
endif

# Other vtk libs might be needed as dependencies on these, but they will get loaded
# automatically. However, if you try to statically link vtk, you need to explicitly
# include them all here.
VTK_LIBS := ${VTK_LIB_DIR} -lvtkRendering -lvtkGraphics -lvtkImaging -lvtkFiltering -lvtkCommon 

# Directories to search for header files
SEARCHDIRS := -iquote -I. ${VTK_INCLUDES} ${WX_CXXFLAGS} 

DEBUG := 0
ifeq ($(DEBUG),0)
# May want to add -O2 or -O3 and maybe -march=i686 for better performance
# -mfpmath=sse 	enables sse/sse2 for doubleing point operations
# -mmmx 	enables the mmx unit in Pentium II CPUs
# -msse 	enables the sse unit in Pentium III CPUs
# -msse2 	enables the sse2 unit in Pentium IV CPUs
# -msse-math 	enables math support via the sse unit in Pentium III and IV
# -m3dnow 	enables the 3dnow unit in Athlons
#DEPENDFLAGS := -Wall -Wno-deprecated -O3 -mcpu=i686 -march=i686 -msse -mfpmath=sse ${SEARCHDIRS}
#DEPENDFLAGS := -Wall -g ${SEARCHDIRS}
DEPENDFLAGS := -Wall -Wno-deprecated -O3 ${SEARCHDIRS}
DEBUGFLAGS := 
else
DEPENDFLAGS := -Wall -Wno-deprecated -O0 ${SEARCHDIRS}
# -g3 -v -Q -da
DEBUGFLAGS := -ggdb
endif

# C preprocessor (C, C++, FORTRAN)
CPPFLAGS :=  -DGTK_NO_CHECK_CASTS -D__WXGTK__ -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES

# C++ compiler
CXX      := g++ 
CXXFLAGS  := ${DEBUGFLAGS} ${DEPENDFLAGS}

%.o : %.cc
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@

%.o : %.C
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@

%.o : %.cpp
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@

%.o : %.cxx
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $< -o $@

# C/C++/Eiffel/FORTRAN linker
LINKER    := gcc
LDFLAGS    := -lstdc++
LIBS :=  ${WX_LIBS} ${VTK_LIBS} 

# mrMeshSrv
mrMeshSrv.glxa64 : ./3dview.o ./actor.o ./camera.o ./clientframe.o \
./commandhandler.o ./cursor.o ./doublearray.o ./grayVolume.o ./helpers.o \
./imagequad.o ./light.o ./matrix3x3.o ./mesh.o \
./mrmeshsrvapp.o ./parametersmap.o ./polydatabuffer.o ./polyline.o \
./progressindicator.o ./roi.o ./scene.o ./serverconsole.o ./stdafx.o \
./text.o ./transformarrow.o ./vtkfilter.o ./vtkPolyDataSingleSourceShortestPath.o
	${LINKER} ${LDFLAGS} -o $@ ${filter-out %.a %.so, $^} ${LIBS}

# target for making everything
all: mrMeshSrv.glxa64

# target for removing all object files
tidy: 
	@${RM} core ./*.o

# target for removing all object files and executable
clean: 
	@${RM} core ./*.o mrMeshSrv.glxa64
