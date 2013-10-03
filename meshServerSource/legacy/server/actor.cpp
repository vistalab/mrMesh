#include "stdafx.h"
#include "actor.h"

BEGIN_HANDLER_TABLE(CActor)
	COMMAND_HANDLER((char* const)"set", CActor::SetProperties)
	COMMAND_HANDLER((char* const)"get", CActor::GetProperties)
//	COMMAND_HANDLER((char* const)"show", CActor::CommandShow)
END_HANDLER_TABLE()

CActor::CActor(const SceneInfo *pSI, int _iActorID)
:iActorID(_iActorID),
bVisible(true),
bInCameraSpace(false),
pSceneInfo(pSI)
{
}

CActor::~CActor()
{
}

bool CActor::GetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	if (paramsIn.GetInt(_T("get_origin"), FALSE) || paramsIn.GetInt(_T("get_all"), FALSE))
	{
		CDoubleArray *pArray = new CDoubleArray;
		pArray->Create(1, 3);

		double *pV = (double *)vOrigin;
		double *pA = pArray->GetPointer();
		
		for (int iComponent = 0; iComponent < 3; iComponent++)
			pA[iComponent] = pV[iComponent];

		paramsOut.SetArray(_T("origin"), pArray);
	}

	if (paramsIn.GetInt(_T("get_rotation"), FALSE) || paramsIn.GetInt(_T("get_all"), FALSE))
	{
		CDoubleArray *pArray = new CDoubleArray;
		pArray->Create(2, 3, 3);

		double *pM = mRotation.fElements;
		double *pA = pArray->GetPointer();
		
		for (int iElement = 0; iElement < 9; iElement++)
			pA[iElement] = pM[iElement];

		paramsOut.SetArray(_T("rotation"), pArray);
	}
	if (paramsIn.GetInt(_T("get_camera_space"), FALSE) || paramsIn.GetInt(_T("get_all"), FALSE))
	{
		paramsOut.SetInt(_T("camera_space"), bInCameraSpace);
	}
	return true;
}

bool CActor::SetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CDoubleArray *pOriginArray = paramsIn.GetArray(_T("origin"));
	
	if (pOriginArray)
	{
		if (pOriginArray->GetNumberOfItems() != 3)
			paramsOut.AppendString(_T("error"), _T("'origin' must be size of [3] - ignored"));
		else
		{
			double *pOrg = pOriginArray->GetPointer();
			
			vOrigin.x = pOrg[0];
			vOrigin.y = pOrg[1];
			vOrigin.z = pOrg[2];
		}
	}

	CDoubleArray *pRotationArray = paramsIn.GetArray(_T("rotation"));

	if (pRotationArray)
	{
		if ((pRotationArray->GetNumberOfDimensions()!=2) ||
			(pRotationArray->GetSizes()[0] != 3) ||
			(pRotationArray->GetSizes()[1] != 3))
		{
			paramsOut.AppendString(_T("error"), _T("'rotation' must be [3x3] - ignored"));
		}
		else
		{
			double *pM = mRotation.fElements;
			double *pA = pRotationArray->GetPointer();

			for (int i=0; i<9; i++)
				pM[i] = pA[i];
		}
	}

	int	iValue;

	if (paramsIn.GetInt(_T("camera_space"), &iValue))
	{
		AttachToCameraSpace(iValue != 0);
	}

	return true;
}

//bool CActor::CommandShow(CParametersMap &paramsIn, CParametersMap &paramsOut)
//{
//	Show(paramsIn.GetInt("show", 1)!=0);
//	return true;
//}

CVector	CActor::GetCameraRelativeOrigin()
{
	const CActor *pCamera = (const CActor *)pSceneInfo->pCamera;
	if (IsAttachedToCameraSpace())
		return vOrigin;
	else
//		return vOrigin * pCamera->mRotation - pCamera->vOrigin;
		return (vOrigin  - pCamera->vOrigin) * pCamera->mRotation;
}
