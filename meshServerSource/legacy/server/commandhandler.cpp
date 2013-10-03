#include "stdafx.h"
#include "commandhandler.h"

BEGIN_HANDLER_TABLE(CCommandHandler)
	COMMAND_HANDLER((char* const)"test", CCommandHandler::TestHandler)
END_HANDLER_TABLE()

CCommandHandler::CCommandHandler()
{
}

CCommandHandler::~CCommandHandler()
{
}

bool CCommandHandler::ProcessCommand(char *pCommand, bool *pRes, CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	const CommandTableEntry *pTable = GetHandlersTable();
	if (!pTable)
	{
		wxASSERT(0);
		return false;
	}

	wxASSERT(pRes);

	int i = 0;
	while (pTable[i].pCommand)
	{
		if (wxStricmp(wxString::FromAscii(pTable[i].pCommand), wxString::FromAscii(pCommand)) == 0)
		{
			wxASSERT(pTable[i].procHandler);

			if (!pTable[i].procHandler)
				return false;
			
			*pRes = (this->*(pTable[i].procHandler))(paramsIn, paramsOut);
			return true;
		}
		i++;
	}
	return false;
}
