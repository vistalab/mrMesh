#include "stdafx.h"
#include "polydatabuffer.h"
#include "mesh.h"
#include "scene.h"
#include "camera.h"

CPolyDataBuffer::CPolyDataBuffer(CScene *_pScene)
{
	this->pScene = _pScene;

	pVertices = NULL;
	nVertices = 0;

	pTriangles = NULL;
	pTriangleOrder = NULL;
	nTriangles = 0;
}

CPolyDataBuffer::~CPolyDataBuffer()
{
	wxASSERT(arrayObjects.size() == 0);	// all meshes must be already unregistered

	wxASSERT(!pVertices);
	wxASSERT(!pTriangles);
	wxASSERT(!pTriangleOrder);

	//if (pVertices)
	//	delete[] pVertices;
	//if (pTriangles)
	//	delete[] pTriangles;
	//if (pTriangleOrder)
	//	delete[] pTriangleOrder;
}

bool CPolyDataBuffer::RegisterObject(CMesh *pMesh)
{
	wxASSERT(pMesh);
	wxASSERT(pMesh->nVertices);
	wxASSERT(pMesh->nTriangles);

	// check if we registering the same mesh for the second time
	std::vector<PolyObject*>::iterator it;
	for (it = arrayObjects.begin(); it != arrayObjects.end(); it++)
	{
		if ((*it)->pMesh == pMesh)
		{
			wxASSERT(0);
			return false;
		}
	}

	// add to list

	PolyObject *pNewItem = new PolyObject(pMesh, nVertices, nTriangles);

	int	nPrevTrianglesCount = nTriangles;

	if (!GrowVertexBuffer(pMesh->nVertices))
		return false;
	if (!GrowTriangleBuffer(pMesh->nTriangles))
	{
		DeleteFromVertexBuffer(pNewItem->iFirstVertexIndex, pMesh->nVertices);
		return false;
	}

	arrayObjects.push_back(pNewItem);

	int iT;
	for (iT = 0; iT < pMesh->nTriangles; iT++)
	{
		pTriangles[nPrevTrianglesCount + iT].fSumZ = 0;
		pTriangles[nPrevTrianglesCount + iT].iTriangleIndex = iT;
		pTriangles[nPrevTrianglesCount + iT].pOwner = pNewItem;
	}

	for (iT = 0; iT < nTriangles; iT++)
		pTriangleOrder[iT] = iT;

	SetupGLArrays();

	UpdateTransform();
	SortTriangles();

	return true;
}

bool CPolyDataBuffer::UnregisterObject(CMesh *pMesh)
{
	std::vector<PolyObject*>::iterator it = arrayObjects.begin();
	
	while (it != arrayObjects.end())
	{
		if ((*it)->pMesh == pMesh)
		{
			DeleteFromVertexBuffer((*it)->iFirstVertexIndex, pMesh->nVertices);
			
			DeleteFromTriangleBuffer((*it)->iFirstTriangleIndex, pMesh->nTriangles);

			// update meshes with moved vertices
			std::vector<PolyObject*>::iterator itUpdate;
			for (itUpdate = arrayObjects.begin(); itUpdate != arrayObjects.end(); itUpdate++)
			{
				if ((*itUpdate)->iFirstVertexIndex > (*it)->iFirstVertexIndex)
					(*itUpdate)->iFirstVertexIndex -= pMesh->nVertices;

				if ((*itUpdate)->iFirstTriangleIndex > (*it)->iFirstTriangleIndex)
					(*itUpdate)->iFirstTriangleIndex -= pMesh->nTriangles;
			}

			// reset triangle order to keep off invalid indices
			for (int iT = 0; iT < nTriangles; iT++)
				pTriangleOrder[iT] = iT;

			delete (*it);
			arrayObjects.erase(it);

			SetupGLArrays();

			UpdateTransform();
			SortTriangles();

			return true;
		}
		it++;
	}
	return false;
}

void CPolyDataBuffer::Render(bool bWithGLNames)
{
	if (!nTriangles)
		return;

	this->Timer.Start();

	int	iPrevActorID = -1;

	glBegin(GL_TRIANGLES);

	for (int iT=0; iT<nTriangles; iT++)
	{
		int	iCurTriangle = pTriangleOrder[iT];

		CMesh	*pMesh = pTriangles[iCurTriangle].pOwner->pMesh;
		int		iVertexBase = pTriangles[iCurTriangle].pOwner->iFirstVertexIndex;
		int		iIndex = pTriangles[iCurTriangle].iTriangleIndex;

		if (bWithGLNames)
		{
			int iActorID = pMesh->GetID();
			if (iActorID != iPrevActorID)
			{
				glEnd();

				glLoadName(iActorID);
				iPrevActorID = iActorID;

				glBegin(GL_TRIANGLES);
			}
		}
		
		for (int iV=0; iV<3; iV++)
		{
			glColor4ubv(pMesh->pVertices[pMesh->pTriangles[iIndex].v[iV]].cColor);
			glArrayElement(iVertexBase + pMesh->pTriangles[iIndex].v[iV]);
		}
	}
	glEnd();

	wxLogDebug(wxString::FromAscii("> PolyDataBuffer rendered in %d ms"), this->Timer.GetInterval());
	this->Timer.Stop();
}

void CPolyDataBuffer::UpdateTransform()
{
	this->Timer.Start();

	CCamera *pCamera = (CCamera *)pScene->GetActor(CScene::PA_CAMERA);

	std::vector<PolyObject*>::iterator	it;

	for (it  = arrayObjects.begin(); it != arrayObjects.end(); it++)
	{
		UpdateObjectTransform(*it, pCamera);
	}

	for (int iT=0; iT<nTriangles; iT++)
	{
		pTriangles[iT].fSumZ = 0;

		int		iTriangleIndex = pTriangles[iT].iTriangleIndex;
		CMesh	*pMesh = pTriangles[iT].pOwner->pMesh;
		int		iVertexBase = pTriangles[iT].pOwner->iFirstVertexIndex;

		for (int iV=0; iV<3; iV++)
		{
			pTriangles[iT].fSumZ +=
//				pVertices[pTriangles[iT].v[iV]].vCoord.z;
				pVertices[iVertexBase + pMesh->pTriangles[iTriangleIndex].v[iV]].vCoord.z;
		}
	}

	wxLogDebug(wxString::FromAscii("> PolyDataBuffer transformed in %d ms"), this->Timer.GetInterval());
	this->Timer.Stop();
}

void CPolyDataBuffer::UpdateMeshTransform(CMesh *pMesh)
{
	std::vector<PolyObject*>::iterator it = arrayObjects.begin();
	
	while (it != arrayObjects.end())
	{
		if ((*it)->pMesh == pMesh)
		{
			UpdateObjectTransform(*it, (CCamera*)pScene->GetActor(CScene::PA_CAMERA));
			return;
		}
		it++;
	}
}

void CPolyDataBuffer::UpdateObjectTransform(PolyObject *pObject, CCamera *pCamera)
{
	int		iVertexBase	= pObject->iFirstVertexIndex;
	CMesh	*pMesh		= pObject->pMesh;
	for (int i=0; i<pMesh->nVertices; i++)
	{
		pVertices[iVertexBase + i].vCoord = 
			(
				pMesh->pVertices[i].vCoord
				* pMesh->mRotation
				+ pMesh->vOrigin
			)
			* pCamera->mRotation;

		pVertices[iVertexBase + i].vNormal = 
			pMesh->pVertices[i].vNormal
			* pMesh->mRotation
			* pCamera->mRotation;
	}
}

bool CPolyDataBuffer::CompareTrianglesZ(int i1, int i2, void *pParam)
{
	TriangleInfo *pTriangleData = (TriangleInfo *)pParam;

	return pTriangleData[i1].fSumZ > pTriangleData[i2].fSumZ;
}

void CPolyDataBuffer::SortTriangles()
{
	this->Timer.Start();

	ShellSortInt(pTriangleOrder, nTriangles, CompareTrianglesZ, (void *)pTriangles);

	wxLogDebug(wxString::FromAscii("> PolyDataBuffer triangles sorted in %d ms"), this->Timer.GetInterval());
	this->Timer.Stop();
}

bool CPolyDataBuffer::GrowVertexBuffer(int nItemsToAdd)
{
	wxASSERT(nItemsToAdd > 0);

	wxLogDebug(wxString::FromAscii("> Adding %d vertices"), nItemsToAdd);

	TransformedVertex *pNewBuffer = new TransformedVertex[nVertices + nItemsToAdd];
	if (!pNewBuffer)
		return false;

	if (pVertices)	// if smaller array exists
	{
		memmove(pNewBuffer, pVertices, nVertices*sizeof(TransformedVertex));
		delete[] pVertices;
	}
	pVertices = pNewBuffer;
	nVertices += nItemsToAdd;

	return true;
}

bool CPolyDataBuffer::DeleteFromVertexBuffer(int iStart, int nCount)
{
	wxASSERT((nCount > 0) && (iStart >=0) && (iStart+nCount <= nVertices));

	if (!nVertices)
	{
		wxASSERT(0);
		return false;
	}

	wxLogDebug(wxString::FromAscii("> Deleting %d vertices from offset %d"), nCount, iStart);

	if (nCount == nVertices)
	{
		delete[] pVertices;
		pVertices = NULL;
		nVertices = 0;
	}
	else
	{
		TransformedVertex *pNewBuffer = new TransformedVertex[nVertices-nCount];
		if (!pNewBuffer)
			return false;

		// check these sizes
		memmove(pNewBuffer, pVertices, iStart*sizeof(TransformedVertex));
		memmove(pNewBuffer+iStart, pVertices+iStart+nCount, (nVertices-iStart-nCount)*sizeof(TransformedVertex));

		delete[] pVertices;
		pVertices = pNewBuffer;
		nVertices -= nCount;
	}
	return true;
}

bool CPolyDataBuffer::GrowTriangleBuffer(int nItemsToAdd)
{
	wxASSERT(nItemsToAdd > 0);

	wxLogDebug(wxString::FromAscii("> Adding %d triangles"), nItemsToAdd);

	TriangleInfo *pNewBuffer = new TriangleInfo[nTriangles + nItemsToAdd];
	if (!pNewBuffer)
		return false;

	int	*pNewOrder /*tell me how do i feel :)*/ = new int[nTriangles + nItemsToAdd];
	if (!pNewOrder)
	{
		delete[] pNewBuffer;
		return false;
	}

	if (pTriangles)
	{
		wxASSERT(pTriangleOrder);
		memmove(pNewBuffer, pTriangles, nTriangles*sizeof(TriangleInfo));
		delete[] pTriangles;
	}
	if (pTriangleOrder)
	{
		delete[] pTriangleOrder;
	}

	pTriangles = pNewBuffer;
	nTriangles += nItemsToAdd;

	pTriangleOrder = pNewOrder;

	return true;
}

bool CPolyDataBuffer::DeleteFromTriangleBuffer(int iStart, int nCount)
{
	wxASSERT((nCount > 0) && (iStart >=0) && (iStart+nCount <= nTriangles));

	if (!nTriangles)
	{
		wxASSERT(0);
		return false;
	}

	wxLogDebug(wxString::FromAscii("> Deleting %d triangles from offset %d"), nCount, iStart);

	if (nCount == nTriangles)
	{
		delete[] pTriangles;
		pTriangles = NULL;
		delete[] pTriangleOrder;
		pTriangleOrder = NULL;
	}
	else
	{
		TriangleInfo *pNewBuffer = new TriangleInfo[nTriangles - nCount];
		if (!pNewBuffer)
			return false;
		int *pNewOrder = new int[nTriangles - nCount];
		if (!pNewOrder)
		{
			delete[] pNewBuffer;
			return false;
		}
		
		memmove(pNewBuffer, pTriangles, iStart * sizeof(TriangleInfo));
		memmove(pNewBuffer + iStart, pTriangles+iStart+nCount, (nTriangles-iStart-nCount)*sizeof(TriangleInfo));

		delete[] pTriangles;
		pTriangles = pNewBuffer;

		delete[] pTriangleOrder;
		pTriangleOrder = pNewOrder;
	}
	
	nTriangles -= nCount;
	return true;
}

void CPolyDataBuffer::SetupGLArrays()
{
	if (pVertices)
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(TransformedVertex), &(pVertices[0].vCoord));
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(TransformedVertex), &(pVertices[0].vNormal));
	}
	else
	{
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
	}
}
