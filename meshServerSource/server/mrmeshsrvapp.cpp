#include "stdafx.h"
#include "mrmeshsrvapp.h"
#include "serverconsole.h"

IMPLEMENT_APP(CMrMeshSrvApp)

// I think this can be commented out.
/*
#ifdef _WINDOWS
DWORD WINAPI InterserverThread (LPVOID pParam);
#endif
*/


bool CMrMeshSrvApp::OnInit()
{
#ifdef _WINDOWS
		// leave some CPU time for other tasks
		SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#endif

        wxSocketBase::Initialize();     //RK 12/21/2009, added as advised by wxWidgets

	// I suppose we could change the console size here.  It looks to be
	// hard coded to 400,300 at this point.
	CServerConsole	*frame = new CServerConsole(wxString::FromAscii("mrMesh server"), wxDefaultPosition, wxSize(400, 300));
	frame->Show(TRUE);
	SetTopWindow(frame);

#ifdef _WINDOWS
		// We can't find this definition for an InterserverThread
		// anywhere.  Bob thinks it is from the old attempt at having three
		// programs, one of them an interserver.  We don't think it is used
		// any more.  And the program compiles and runs without this code -- BW
	    // DWORD k;
	    // CreateThread (NULL, 0, InterserverThread, NULL, 0, &k);
#endif

	return true;
}
