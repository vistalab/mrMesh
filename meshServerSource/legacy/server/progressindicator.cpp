#include "stdafx.h"
#include "progressindicator.h"

CProgressIndicator::CProgressIndicator()
{
	procSubscriberCallback = NULL;
	iTaskIDCounter = 0;
}

CProgressIndicator* CProgressIndicator::GetInstance()
{
	static CProgressIndicator	self;
	return	&self;
}

void* CProgressIndicator::StartNewTask(vtkProcessObject *pProcessor, const wxString &strAction)
{
	if (!procSubscriberCallback)
	{
//		wxASSERT(0);
		return NULL;
	}

	Task	*pTask = new Task;
	if (!pTask)
		return NULL;
	
	pTask->iID = iTaskIDCounter++;
	pTask->strAction = strAction;
	pTask->pProcessor = pProcessor;
	pTask->procCallback = procSubscriberCallback;
	
	pTask->lParam = lSubscriberParam;
	
	procSubscriberCallback(pTask->iID, PI_START, NULL, 0, lSubscriberParam);
	// FIXME 2009.02.25 RFD: Had to comment this out to compile with VTK 5.
	//if (pProcessor)
	//	pProcessor->SetProgressMethod(ProgressCallback, (void*)pTask);

	return	pTask;
}

void CProgressIndicator::EndTask(void* pTask)
{
	if (!pTask)
		return;

	if (procSubscriberCallback)
		procSubscriberCallback(((Task*)pTask)->iID, PI_END, NULL, 0, lSubscriberParam);
	delete (Task*)pTask;
}


void CProgressIndicator::SetDisplayProgressCallback(SubscriberCallback proc, long lParam)
{
	procSubscriberCallback = proc;
	lSubscriberParam = lParam;
}

void CProgressIndicator::ProgressCallback(void *pParam)
{
	if (!pParam)
		return;

	// FIXME 2009.02.25 RFD: Had to comment this out to compile with VTK 5.
	//Task *pTask = (Task *)pParam;
	//if (pTask->procCallback)
	//	pTask->procCallback(pTask->iID, PI_TICK, &pTask->strAction, int(pTask->pProcessor->GetProgress()*100), pTask->lParam);
}

void CProgressIndicator::SetProgressValue(void *pTaskID, int iProgress)
{
	if (!pTaskID)
		return;

	Task *pTask = (Task *)pTaskID;
	if (pTask->procCallback)
		pTask->procCallback(pTask->iID, PI_TICK, &pTask->strAction, iProgress, pTask->lParam);
}
