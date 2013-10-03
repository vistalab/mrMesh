#include "stdafx.h"
#include "serverconsole.h"
#include "progressindicator.h"

BEGIN_EVENT_TABLE(CServerConsole, wxFrame)
	EVT_SOCKET(ID_SERVER_EVENT,  CServerConsole::OnServerEvent)
	EVT_SOCKET(ID_SOCKET_EVENT,  CServerConsole::OnSocketEvent)
	EVT_SIZE(CServerConsole::OnSize)
	EVT_CLOSE(CServerConsole::OnClose)
END_EVENT_TABLE()

CServerConsole::CommandHandlersArrayItem CServerConsole::m_CommandHandlersArray[]=
{
	//{"help",		CommandHelp},
	{(char* const)"message",		CommandMessage},
	{(char* const)"close",		CommandClose},
	{(char* const)"get_num_windows",		CommandGetNumWindows},
};

CServerConsole::CServerConsole(const wxString& title, const wxPoint& pos, const wxSize& size)
	:wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	m_pText = new wxTextCtrl(this, -1, wxString::FromAscii(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

	m_pListClients	= new wxListBox(this, -1);
	m_pListTasks	= new wxListCtrl(this, -1, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_NO_HEADER | wxLC_HRULES);
	if (m_pListTasks)
		m_pListTasks->InsertColumn(0, wxString::FromAscii("Tasks"));
			
#ifndef _NO_PROGRESS_INDICATOR
	CProgressIndicator::GetInstance()->SetDisplayProgressCallback(ProgressCallback, (long)this);
#endif

	SetupCommandHandlers();

	iClientIDCounter = 0;

	wxIPV4address	addr;
	addr.LocalHost();
	addr.Service(3000);

	m_pServer = new wxSocketServer(addr);

	if (!m_pServer->Ok())
		Log(wxString::FromAscii("Error starting server\n"));
	else
	{
		Log(wxString::FromAscii("Server started\n"));

		// Setup the event handler and subscribe to connection events
		m_pServer->SetEventHandler(*this, ID_SERVER_EVENT);
		m_pServer->SetNotify(wxSOCKET_CONNECTION_FLAG);
		m_pServer->Notify(TRUE);
	}
/*
	// testing CCommandHandler
	CCommandHandler	chTest;
	const CCommandHandler::CommandTableEntry *pTable = chTest.GetHandlersTable();
	CParametersMap	mapIn, mapOut;
	(chTest.*(pTable[0].procHandler))(mapIn, mapOut);	//WOW!!!
*/
}

CServerConsole::~CServerConsole()
{
	if (m_pText)
		delete m_pText;
	if (m_pListClients)
		delete m_pListClients;
	if (m_pListTasks)
		delete m_pListTasks;
	if (m_pServer)
		delete m_pServer;
}

void CServerConsole::SetupCommandHandlers()
{
	mapClientCommandHandlers.clear();
	for (unsigned i=0; i<sizeof(m_CommandHandlersArray)/sizeof(m_CommandHandlersArray[0]); i++)
	{
		mapClientCommandHandlers[wxString::FromAscii(m_CommandHandlersArray[i].pCommand)] = 
			m_CommandHandlersArray[i].proc;
	}
}

void CServerConsole::OnSize(wxSizeEvent& event)
{
	int	w, h;

	GetClientSize(&w, &h);

	if (m_pText && m_pListClients && m_pListTasks)
	{
		m_pListClients->SetSize(w/2, h/4);
		m_pListTasks->SetSize(w/2, 0, w/2, h/4);

		m_pListTasks->SetColumnWidth(0/*<-index*/, m_pListTasks->GetClientSize().GetWidth());
		m_pText->SetSize(0, h/4, w, 3*h/4);
	}
}

void CServerConsole::OnServerEvent(wxSocketEvent& event)
{
	wxSocketBase *sock;
	wxASSERT(event.GetSocketEvent() == wxSOCKET_CONNECTION);
	sock = m_pServer->Accept(TRUE);
	if (sock){
		Log(wxString::FromAscii("New client connection accepted"));
	}else{
		Log(wxString::FromAscii("Error: couldn't accept a connection"));
		return;
	}

	sock->SetEventHandler(*this, ID_SOCKET_EVENT);
	sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
	sock->Notify(TRUE);
	
	sock->SetFlags(wxSOCKET_WAITALL/* | wxSOCKET_BLOCK*/);
}

void CServerConsole::OnSocketEvent(wxSocketEvent& event){
	wxSocketBase *sock = event.GetSocket();
	switch(event.GetSocketEvent()){
		case wxSOCKET_LOST:{
			Log(wxString::FromAscii("Client disconnected"));
			sock->Destroy();
			break;
		}
		case wxSOCKET_INPUT:{
			// Disable input events, not to trigger wxSocketEvent again
			sock->SetNotify(wxSOCKET_LOST_FLAG);

			ClientHandshake	handshake;
			sock->ReadMsg(&handshake, sizeof(handshake));
			if (sock->Error()){
				Log(wxString::FromAscii("Error receiving client message"));
				sock->Close();
				return;
			}

                        Log(wxString::FromAscii("Got a handshake"));

			int iClientID = handshake.id;
			if ((iClientID == -1) || ((iClientID > 0) && !IsClientIDValid(iClientID))){
				iClientID = CreateNewClient(iClientID);
			}

			ServerReply	reply;
			reply.id = iClientID;
			reply.status = 1;

			if (!IsClientIDValid(iClientID))
				reply.status = -1;

			sock->WriteMsg(&reply, sizeof(reply));
			if (sock->Error()){
				Log(wxString::FromAscii("Error sending a reply"));
				return;
			}
			Log(wxString::FromAscii("Sent a reply"));

			if (!IsClientIDValid(iClientID))
				return;

			ReceiveClientCommand(iClientID, sock);

			// Enable input events again.
			sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);

			break;
		}
		default:
			wxASSERT(0);
	}
}

int	CServerConsole::CreateNewClient(int iRequestedID){
	wxASSERT( mapClients.find(iClientIDCounter) == mapClients.end() );

	int		iNewID = (iRequestedID < 0) ? iClientIDCounter : iRequestedID;
	bool	bUpdateCounterByID = (iRequestedID >= iClientIDCounter);

	CClientFrame	*frame = new CClientFrame(this, iNewID, wxString::Format(wxString::FromAscii("mrMesh %d"), iNewID));
	if (!frame)
		return -1;

	ClientIDData	client;
	client.pWindow = frame;

	m_pListClients->Append(wxString::Format(wxString::FromAscii("%d"), iNewID), (void*)iNewID);

	frame->Show(TRUE);

	mapClients[iNewID] = client;

	if (iRequestedID < 0)
		iClientIDCounter++;
	else if (bUpdateCounterByID)
		iClientIDCounter = iNewID + 1;

	return iNewID;
}


void CServerConsole::CloseClient(int iID){
	HashMapClients::iterator it;
	it = mapClients.find(iID);
	if (it == mapClients.end()){
		wxASSERT(0);
		return;
	}
	it->second.pWindow->Close(TRUE);
	// *** RFD: this was commented out:
	//mapClients.erase(it);
}

bool CServerConsole::IsClientIDValid(int iID){
	HashMapClients::iterator it;
	it = mapClients.find(iID);
	return (it != mapClients.end());
}

void CServerConsole::OnClientFrameClosed(int iClientID){
	HashMapClients::iterator it;
	it = mapClients.find(iClientID);
	if (it == mapClients.end()){
		wxASSERT(0);
		return;
	}

	for (unsigned long i=0; i<m_pListClients->GetCount(); i++){
		if((unsigned long)m_pListClients->GetClientData(i) == iClientID){
			m_pListClients->Delete(i);
			break;
		}
	}
	if(!mapClients.erase(iClientID)){ wxASSERT(0); }
	return;
}

bool CServerConsole::ReceiveClientCommand(int iID, wxSocketBase *sock){
	HashMapClients::iterator it;
	
	ClientHeader	client_header;
        ServerHeader	server_header;
        char		*pCommand = NULL;
        char		*pParams = NULL;
        CParametersMap	paramsIn;
        CParametersMap	paramsOut;

	// check if ID is valid
	it = mapClients.find(iID);
	if (it == mapClients.end()){
		wxASSERT(0);
		return false;
	}

	// receive message header

	sock->ReadMsg(&client_header, sizeof(client_header));
	if (sock->Error())
	{
		Log(wxString::FromAscii("Failed to receive client message"));
		return false;
	}

	Log(wxString::FromAscii("Received client message"));

	// receive command and parameters
	
	if (client_header.command_length){
                pCommand = new char[client_header.command_length];
		sock->ReadMsg(pCommand, client_header.command_length);

		//_strlwr(pCommand);
                for (char *s=pCommand; *s; s++){
                        if (*s >= 'A' && *s <= 'Z')
                                *s = *s - 'A' + 'a';
                }
                wxString wCommand(pCommand, wxConvUTF8);
                Log(wxString::Format(wxString::FromAscii("Command = %s"), wCommand.c_str()));   //RK 12/21/09: fixed, pCommand is showing up now
        }

	if (client_header.params_length){
		pParams = new char[client_header.params_length];
		//sock->ReadMsg(pParams, client_header.params_length);
		bool bRes = ReceiveInParts(sock, pParams, client_header.params_length);
		if (!bRes)
			Log(wxString::FromAscii("Error receiving client data"));
		else{
                        if (client_header.params_length < 50) {
                                wxString wParams(pParams, wxConvUTF8);
                                Log(wxString::Format(wxString::FromAscii("Params\n********\n%s********"), wParams.c_str()));   //RK 12/21/09: fixed
                        } else
                                Log(wxString::Format(wxString::FromAscii("Params : %d bytes\n"), client_header.params_length));
		}
        }

	// parse parameters

	if (pParams){
		if (!paramsIn.CreateFromString(pParams, client_header.params_length))
			paramsOut.SetString(wxString::FromAscii("warning"),
                                wxString::FromAscii("Invalid arguments string"));

		//<debug>
		//{
		//int		nLen = 0;
		//char	*pData = paramsIn.FormatString(&nLen);

		//Dump("params_in", pParams, client_header.params_length);
		//Dump("params_fmt", pData, nLen);

		//delete[] pData;
		//}
		//</debug>
	}

	// Search string => command arrays and maps

	bool bHandlerRes = false;
	
	if (!pCommand)
		paramsOut.SetString(wxString::FromAscii("error"),
                            wxString::FromAscii("No command"));
	else{
                if (!it->second.pWindow->Get3DView()->ProcessCommand(pCommand, &bHandlerRes, paramsIn, paramsOut)){
			// no handler in C3DView
                        ProcessClientCommand(&it->second, pCommand, paramsIn, paramsOut);
		}
	}
	if (bHandlerRes)
		server_header.status = 1;
	else
		server_header.status = -1;

	// cleanup
	
	if (pCommand)
		delete[] pCommand;
	if (pParams)
		delete[] pParams;

	// reply

	char	*pReplyData = paramsOut.FormatString(&server_header.data_length);

	if (server_header.data_length ==1)
		server_header.data_length = 0;

	sock->WriteMsg(&server_header, sizeof(server_header));
	if (sock->Error())
	{
		Log(wxString::FromAscii("Failed to respond"));
		if (pReplyData)
			delete[] pReplyData;
		return false;
	}
	
	if (server_header.data_length)
	{
		//Dump("outf", pReplyData, server_header.data_length);

		//sock->WriteMsg(pReplyData, server_header.data_length);
		//if (sock->Error())
		//{
		//	wxASSERT(0);			
		//	Log(wxString::Format(wxString::FromAscii("Failed to send data (%d bytes)"), server_header.data_length));
		//}

		if (!SendInParts(sock, pReplyData, server_header.data_length))
			Log(wxString::Format(wxString::FromAscii("Failed to send data (%d bytes)"), server_header.data_length));
	}

	if (pReplyData)
		delete[] pReplyData;

        Log(wxString::FromAscii("\n"));
	return true;
}

bool CServerConsole::ProcessClientCommand(ClientIDData *pClient, char *pCommand, CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	wxASSERT(pClient);

	HashMapCommandHandlers::iterator	it;

	it = mapClientCommandHandlers.find(wxString::FromAscii(pCommand));
	
	if (it == mapClientCommandHandlers.end())
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Unknown command"));

		Log(wxString::FromAscii("Unknown command\n"));
		return false;
	}
/*
	// uncomment to check parser
	int iLen;
	char	*pDump = paramsIn.FormatString(&iLen);
	Log(pDump);
	delete[] pDump;
*/
	return it->second(pClient, paramsIn, paramsOut, this);
}

bool CServerConsole::CommandMessage(ClientIDData *pClient, CParametersMap &paramsIn, CParametersMap &paramsOut, CServerConsole *pThis)
{
	wxString	strMsg;

	if (!paramsIn.GetString(wxString::FromAscii("message"), &strMsg))
	{
		paramsOut.SetString(wxString::FromAscii("error"),
                            wxString::FromAscii("No 'message' field"));
		return false;
	}

	pClient->pWindow->Raise();
	wxMessageDialog(pClient->pWindow, strMsg, wxString::FromAscii("Message"), wxOK | wxCENTRE).ShowModal();
	
	return true;
}

bool CServerConsole::CommandClose(ClientIDData *pClient, CParametersMap &paramsIn, CParametersMap &paramsOut, CServerConsole *pThis){
	HashMapClients::iterator	it;

	for (it = pThis->mapClients.begin(); it != pThis->mapClients.end(); it++){
		if (it->second.pWindow == pClient->pWindow){
			pThis->CloseClient(it->first);
			return true;
		}
	}
	wxASSERT(0);
	return false;
}

bool CServerConsole::CommandGetNumWindows(ClientIDData *pClient, CParametersMap &paramsIn, CParametersMap &paramsOut, CServerConsole *pThis){
	paramsOut.SetInt(wxString::FromAscii("n"), pThis->mapClients.size());
	return true;
}

bool CServerConsole::CommandHelp(ClientIDData *pClient, CParametersMap &paramsIn, CParametersMap &paramsOut, CServerConsole *pThis){
	HashMapCommandHandlers::iterator it;

	wxString	strHelp;
	strHelp.Alloc(pThis->mapClientCommandHandlers.size()*10);

	for (it = pThis->mapClientCommandHandlers.begin(); it != pThis->mapClientCommandHandlers.end(); it++)
            strHelp += it->first + wxString::FromAscii("\n");

//	wxString	strViewCommands;
//	C3DView::GetCommandsList(strViewCommands);

//	strHelp += strViewCommands;

	paramsOut.SetString(wxString::FromAscii("Available commands"), strHelp);

	return true;
}

void CServerConsole::OnClose(wxCloseEvent& event){
	if(!mapClients.empty()){
		if(event.CanVeto() && wxMessageDialog(this, wxString::FromAscii("Do you really want to quit?"),
                                wxString::FromAscii("Warning!"), 
								wxYES_NO | wxYES_DEFAULT | wxCENTRE).ShowModal() == wxID_NO){
			event.Veto();
			return;
		}else{
			HashMapClients::iterator	it;
			for (it = this->mapClients.begin(); it != this->mapClients.end(); it++)
				this->CloseClient(it->first);
		}
	}
	Destroy();
}

void CServerConsole::ProgressCallback(int iID, int iCommand, const wxString *strOperation, int iProgress, long lParam){
	CServerConsole *pThis = (CServerConsole *)lParam;

	switch (iCommand){
	case CProgressIndicator::PI_START:{
		long lIndex = pThis->m_pListTasks->InsertItem(0, wxString::FromAscii("---"));
		pThis->m_pListTasks->SetItemData(lIndex, iID);

		pThis->Raise();
		break;
	}
	case CProgressIndicator::PI_END:
		for (int i=0; i<pThis->m_pListTasks->GetItemCount(); i++){
			if (pThis->m_pListTasks->GetItemData(i) == iID){
				pThis->m_pListTasks->DeleteItem(i);
				break;
			}
		}
		break;
	case CProgressIndicator::PI_TICK:
		for (int i=0; i<pThis->m_pListTasks->GetItemCount(); i++){
			if (pThis->m_pListTasks->GetItemData(i) == iID){
				wxString strMsg = wxString::Format(wxString::FromAscii("%d%% - %s"), iProgress, strOperation->GetData());
				pThis->m_pListTasks->SetItemText(i, strMsg);
				break;
			}
		}
		break;
	}
	wxSafeYield();
	//wxYield();
}
