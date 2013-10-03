#include "stdafx.h"
#include "scene.h"

#include "mesh.h"
#include "text.h"
#include "light.h"
#include "cursor.h"
#include "polyline.h"
#include "imagequad.h"
#include "transformarrow.h"

BEGIN_HANDLER_TABLE(CScene)
	COMMAND_HANDLER((char* const)"add_actor",	CScene::CommandAddActor)
	COMMAND_HANDLER((char* const)"remove_actor", CScene::CommandRemoveActor)
	COMMAND_HANDLER((char* const)"transparency", CScene::CommandTransparency)
	COMMAND_HANDLER((char* const)"background",	CScene::CommandBackground)
	COMMAND_HANDLER((char* const)"enable_origin_arrows", CScene::CommandEnableOriginArrows)
	COMMAND_HANDLER((char* const)"enable_3d_cursor",		CScene::CommandEnable3DCursor)
	COMMAND_HANDLER((char* const)"get_num_actors",		CScene::CommandGetNumActors)
END_HANDLER_TABLE()

const int CScene::nAuxActorsCount = CScene::PA_MANIP_X;

CScene::CScene()
{
	iActorCounter = PA_PREDEFINED_COUNT;

	CCamera *pCamera = new CCamera(&siSceneInfo, PA_CAMERA);

	mapActors[PA_CAMERA]	= pCamera;
	mapActors[PA_CURSOR]	= new CCursor(&siSceneInfo, PA_CURSOR);

	mapActors[PA_MANIP_X]	= new CTransformArrow(&siSceneInfo, PA_MANIP_X, CVector(1.0f, 0, 0));
	mapActors[PA_MANIP_Y]	= new CTransformArrow(&siSceneInfo, PA_MANIP_Y, CVector(0, 1.0f, 0));
	mapActors[PA_MANIP_Z]	= new CTransformArrow(&siSceneInfo, PA_MANIP_Z, CVector(0, 0, 1.0f));

	bEnableArrows	= true;
	bEnable3DCursor	= true;

	siSceneInfo.nLights		= 0;
	siSceneInfo.pPolyBuffer	= new CPolyDataBuffer(this);
	siSceneInfo.pCamera		= pCamera;
	siSceneInfo.bTransparentMeshEnabled = false;
}

CScene::~CScene()
{
	HashMapActors::iterator	it;

	for (it = mapActors.begin(); it != mapActors.end(); it++)
	{
		delete it->second;
	}
	mapActors.clear();

	for (it = map2DActors.begin(); it != map2DActors.end(); it++)
	{
		delete it->second;
	}
	map2DActors.clear();

	delete siSceneInfo.pPolyBuffer;
}

CActor*	CScene::CreateActorInstance(const wxString &strClassName, int iActorID)
{
	if (strClassName.CmpNoCase(wxString::FromAscii(CLight::pszClassName)) == 0)
	{
		if (siSceneInfo.nLights == 8)
			return NULL;

		CLight *pLight = new CLight(&siSceneInfo, iActorID);
		if (pLight)
			siSceneInfo.nLights++;
		return pLight;
	}
	else if (strClassName.CmpNoCase(wxString::FromAscii(CPolyLine::pszClassName)) == 0)
	{
		return new CPolyLine(&siSceneInfo, iActorID);
	}
	else if (strClassName.CmpNoCase(wxString::FromAscii(CImageQuad::pszClassName)) == 0)
	{
		return new CImageQuad(&siSceneInfo, iActorID);
	}
	else if (strClassName.CmpNoCase(wxString::FromAscii(CMesh::pszClassName)) == 0)
	{
		return new CMesh(&siSceneInfo, iActorID);
	}
	else if (strClassName.CmpNoCase(wxString::FromAscii(CText::pszClassName)) == 0)
	{
		return new CText(&siSceneInfo, iActorID);
	}
	return NULL;
}

void CScene::Render(bool bWithGLNames)
{
	HashMapActors::iterator	it;

	double		fActorTransform[16];
	CMatrix3x3	*pCameraTransform;

	if (bWithGLNames)
	{
		glInitNames();
		glPushName(0);
	}

	pCameraTransform = &(GetActor(PA_CAMERA)->mRotation);

	////////// Render 3d actors

	glPushMatrix();	// push identity matrix

	//int	nImageQuads = 0;

	for (it = mapActors.begin(); it != mapActors.end(); it++)
	{
		if (bWithGLNames)	// we don't want clicking in "system" actors
			if (it->first < /*PA_PREDEFINED_COUNT*/nAuxActorsCount)
				continue;
		
		if (!bEnableArrows && (it->second->GetClassName() == CTransformArrow::pszClassName))
			continue;
		if (!bEnable3DCursor && (it->second->GetID() == PA_CURSOR))
			continue;

		if (it->second->GetClassName() == CImageQuad::pszClassName)
		{
			arrayImageQuads.Add(it->second);
			//nImageQuads ++;
			continue;
		}
	
		if (bWithGLNames)
			glLoadName(it->first);

		CActor *pActor = it->second;

		if (pActor->IsAttachedToCameraSpace())
			MatrixAndVectorTo4x4(fActorTransform, pActor->mRotation, pActor->vOrigin + GetActor(PA_CAMERA)->vOrigin);
		else
			Transfom3x3AndVectorBy3x3(fActorTransform, pActor->mRotation, pActor->vOrigin, *pCameraTransform);

		glLoadMatrixd(fActorTransform);

		pActor->Render();
	}

	glPopMatrix(); 	// pop identity matrix

	////////////// render meshes stored in poly buffer

	siSceneInfo.pPolyBuffer->Render(bWithGLNames); // this doesn't change modelview matrix

	////////////// render image quads

	if (arrayImageQuads.GetCount())
	{
		this->Timer.Start();

		glPushMatrix();

		arrayImageQuads.Sort(ImageQuadCompare);
		for (unsigned iImage = 0; iImage < arrayImageQuads.GetCount(); iImage++)
		{
			CActor *pActor = arrayImageQuads[iImage];

			if (pActor->IsAttachedToCameraSpace())
				MatrixAndVectorTo4x4(fActorTransform, pActor->mRotation, pActor->vOrigin + GetActor(PA_CAMERA)->vOrigin);
			else
				Transfom3x3AndVectorBy3x3(fActorTransform, pActor->mRotation, pActor->vOrigin, *pCameraTransform);

			glLoadMatrixd(fActorTransform);

			if (bWithGLNames)
				glLoadName(pActor->GetID());

			pActor->Render();
		}
		arrayImageQuads.Empty();

		glPopMatrix();

		wxLogDebug(wxString::FromAscii("> Images sorted and drawn in %d ms"), this->Timer.GetInterval());
		this->Timer.Stop();
	}
	////////////// render 2d objects


	for (it = map2DActors.begin(); it != map2DActors.end(); it++)
	{
		if (bWithGLNames)
			glLoadName(it->first);

		CActor *pActor = it->second;
		pActor->Render();
	}
}

bool CScene::CommandAddActor(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	wxString	strActor;

	if (!paramsIn.GetString(wxString::FromAscii("class"), &strActor))
	{
		paramsOut.SetString(wxString::FromAscii("error"),
                            wxString::FromAscii("No 'class' specified"));
		return false;
	}

	CActor *pActor = CreateActorInstance(strActor, iActorCounter);
	if (!pActor)
	{
		paramsOut.SetString(wxString::FromAscii("error"),
                            wxString::FromAscii("Failed to create actor (probably invalid class)"));
		return false;
	}

	/// save 3d and 2d actors to corresponding list
	if (pActor->GetClassName() == CText::pszClassName)	// 2d
		map2DActors[iActorCounter] = pActor;
	else
		mapActors[iActorCounter] = pActor;

	paramsOut.SetInt(wxString::FromAscii("actor"), iActorCounter);
	iActorCounter++;

	return true;
}

bool CScene::CommandRemoveActor(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	int iActorID;

	if (!paramsIn.GetInt(wxString::FromAscii("actor"), &iActorID))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Actor id ('actor') required"));
		return false;
	}
	
	HashMapActors::iterator it = mapActors.find(iActorID);

	if (it != mapActors.end())
	{
		delete it->second;
		mapActors.erase(it);
	}
	else
	{
		it = map2DActors.find(iActorID);
		if (it != map2DActors.end())
		{
			delete it->second;
			map2DActors.erase(it);
		}
		else
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Invalid actor"));
			return false;
		}
	}

	return true;
}

CActor*	CScene::GetActor(int iActorID)
{
	HashMapActors::iterator it = mapActors.find(iActorID);

	//if (it == mapActors.end())	// no actor found
	//	return NULL;

	//return it->second;

	if (it != mapActors.end())
		return it->second;
	else
	{
		it = map2DActors.find(iActorID);
		if (it != map2DActors.end())
			return it->second;
		else
			return NULL;
	}
}

bool CScene::ProcessCommand(char *pCommand, bool *pRes, CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	if (CCommandHandler::ProcessCommand(pCommand, pRes, paramsIn, paramsOut))
		return true;
	else
	{
		int iActorID;
		if (!paramsIn.GetInt(wxString::FromAscii("actor"), &iActorID))
			return false;
		
		CActor *pActor = GetActor(iActorID);
		if (!pActor)
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Invalid actor id"));
			return false;
		}
		return pActor->ProcessCommand(pCommand, pRes, paramsIn, paramsOut);
	}
}

//int	CScene::GetActorsCount()
//{
//	return mapActors.size() + map2DActors.size();
//}

bool CScene::CommandTransparency(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	int	bEnable = paramsIn.GetInt(wxString::FromAscii("enable"), true);
	HashMapActors::iterator it = mapActors.begin();
	
	if ((bEnable!=0) == siSceneInfo.bTransparentMeshEnabled)
		return true;

	if (bEnable)
	{
		for (; it != mapActors.end(); it++)
		{
			/// todo: if classes derived from CMesh will exist, implement simple RTTI
			if (it->second->GetClassName() == CMesh::pszClassName)
			{
				siSceneInfo.pPolyBuffer->RegisterObject((CMesh*)it->second);
			}
		}
		siSceneInfo.bTransparentMeshEnabled = true;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		for (; it != mapActors.end(); it++)
		{
			if (it->second->GetClassName() == CMesh::pszClassName)
			{
				CMesh *pMesh = (CMesh*)it->second;
				if (pMesh->pVertices && pMesh->pTriangles)
					siSceneInfo.pPolyBuffer->UnregisterObject(pMesh);
			}
		}
		siSceneInfo.bTransparentMeshEnabled = false;
		glDisable(GL_BLEND);
	}
	return true;
}

bool CScene::CommandBackground(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CDoubleArray *pColor = paramsIn.GetArray(wxString::FromAscii("color"));
	if (pColor && (pColor->GetNumberOfItems() == 3 || pColor->GetNumberOfItems() == 4)){
		double *fColor = pColor->GetPointer();
		if(pColor->GetNumberOfItems() == 3)
			glClearColor(fColor[0], fColor[1], fColor[2], 1.0);
		else
			glClearColor(fColor[0], fColor[1], fColor[2], fColor[3]);
		return true;
	}else{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("color field must be 1x3 (RGB) or 1x4 (RGBA)"));
		return false;
	}
}	

bool CScene::CommandEnableOriginArrows(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	bEnableArrows = (paramsIn.GetInt(wxString::FromAscii("enable"), TRUE) != FALSE); // '!=FALSE' is to avoid VC++ warning
	return true;
}

bool CScene::CommandEnable3DCursor(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	bEnable3DCursor = (paramsIn.GetInt(wxString::FromAscii("enable"), TRUE) != FALSE); // '!=FALSE' is to avoid VC++ warning
	return true;
}

int	CMPFUNC_CONV CScene::ImageQuadCompare(CActor **pA1, CActor **pA2)
{
	double	z1 = (*pA1)->GetCameraRelativeOrigin().z;
	double	z2 = (*pA2)->GetCameraRelativeOrigin().z;
	
	if (z1 < z2)
		return -1;
	return 1;
}

bool CScene::CommandGetNumActors(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
//	paramsOut.SetInt("n_total", mapActors.size() + map2DActors.size());


	HashMapActors::iterator	it;

	int	nClientActors = 0;

	for (it = mapActors.begin(); it != mapActors.end(); it++)
	{
		if (it->first >= PA_PREDEFINED_COUNT)
				nClientActors++;
	}

	for (it = map2DActors.begin(); it != map2DActors.end(); it++)
	{
		if (it->first >= PA_PREDEFINED_COUNT)
				nClientActors++;
	}

	paramsOut.SetInt(/*"n_client"*/wxString::FromAscii("n"), nClientActors);

	return true;
}
