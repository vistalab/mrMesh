#include "stdafx.h"
#include "clientframe.h"
#include "serverconsole.h"

//IMPLEMENT_DYNAMIC_CLASS(CClientFrame, wxFrame)

BEGIN_EVENT_TABLE(CClientFrame, wxFrame)
	EVT_SIZE(CClientFrame::OnSize)
	EVT_CLOSE(CClientFrame::OnClose)
END_EVENT_TABLE()

CClientFrame::CClientFrame(wxFrame *pConsole, int iClientID, const wxString& title, const wxPoint& pos, const wxSize& size)
	:wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	m_pConsole = pConsole;
	m_iClientID = iClientID;

	m_p3DView = new C3DView(this, wxPoint(0, 0), GetClientSize(), m_pConsole);
}

CClientFrame::~CClientFrame()
{
	m_p3DView->Destroy();
}

void CClientFrame::OnSize(wxSizeEvent& event)
{
	int	w, h;

	GetClientSize(&w, &h);

	if (m_p3DView)
		m_p3DView->SetSize(w, h);
}

void CClientFrame::OnClose(wxCloseEvent& event)
{
	if (event.CanVeto())
	{
		if (wxMessageDialog(this, _T("Client didn't request to close the window."
			"Do you really want to close it now?"), _T("Warning!"),
			wxYES_NO | wxYES_DEFAULT | wxCENTRE).ShowModal() == wxID_NO)
		{
			event.Veto();
			return;
		}
	}

	Destroy();
	((CServerConsole*)m_pConsole)->OnClientFrameClosed(m_iClientID);
}
