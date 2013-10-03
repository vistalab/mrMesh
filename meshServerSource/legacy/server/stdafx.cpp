// stdafx.cpp : source file that includes just the standard includes
// mrMeshSrv.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _WINDOWS

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "RpcRT4.Lib")

#pragma comment(lib,"vtkCommon.lib")
#pragma comment(lib,"vtkGraphics.lib")
//#pragma comment(lib,"vtkRendering.lib")
#pragma comment(lib,"vtkHybrid.lib")
#pragma comment(lib,"vtkPatented.lib")
#pragma comment(lib,"vtkFiltering.lib")

#ifdef _DEBUG
	#pragma comment(lib,"wxzlib.lib")
	#pragma comment(lib,"wxregex.lib")
	#pragma comment(lib,"wxpng.lib")
	#pragma comment(lib,"wxjpeg.lib")
	#pragma comment(lib,"wxtiff.lib")
	#pragma comment(lib,"wxbase26.lib")
	#pragma comment(lib,"wxmsw26_core.lib")
	#pragma comment(lib,"wxmsw26_adv.lib")
	#pragma comment(lib,"wxbase26_net.lib")
	#pragma comment(lib,"wxmsw26_gl.lib")

/*
	#pragma comment(linker, "/NODEFAULTLIB:libcd.lib /NODEFAULTLIB:msvcrtd.lib /NODEFAULTLIB:oleacc.lib")
	#pragma comment(linker, "/NODEFAULTLIB:libcid.lib")
*/
#else
	#pragma comment(lib,"wxzlib.lib")
	#pragma comment(lib,"wxregex.lib")
	#pragma comment(lib,"wxpng.lib")
	#pragma comment(lib,"wxjpeg.lib")
	#pragma comment(lib,"wxtiff.lib")
	#pragma comment(lib,"wxbase26.lib")
	#pragma comment(lib,"wxmsw26_core.lib")
	#pragma comment(lib,"wxmsw26_adv.lib")
	#pragma comment(lib,"wxbase26_net.lib")
	#pragma comment(lib,"wxmsw26_gl.lib")
#endif

#endif //_WINDOWS
