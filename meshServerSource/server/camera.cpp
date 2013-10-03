#include "stdafx.h"
#include "camera.h"
#include "text.h"
#include "transformarrow.h"

const char* CCamera::pszClassName = "camera";

CCamera::CCamera(const SceneInfo *pSI, int iActorID)
:CActor(pSI, iActorID)
{
	//SetViewport(0, 0, 1, 1);
	// causes a call with wrong gl context under linux

	fViewportWidth	= 1;
	fViewportHeight	= 1;


	fFarClip = 1000;
	fNearClip = -1000;
}

CCamera::~CCamera()
{
}

void CCamera::Zoom(double fZoomFactor)
{
	double fAspect = fViewportWidth / fViewportHeight;

	double fNewWidth		= fFrustumWidth / fZoomFactor;
	double fNewHeight	= fFrustumHeight / fZoomFactor;

	const double	fMinWidth = 2.0f;
	const double fMaxWidth = 2500.0f;

	if (fNewWidth < fMinWidth)
	{
		fNewWidth = fMinWidth;
		fNewHeight = fNewWidth / fAspect;
	}

	if (fNewWidth > fMaxWidth)
	{
		fNewWidth = fMaxWidth;
		fNewHeight = fNewWidth / fAspect;
	}

	fFrustumWidth = fNewWidth;
	fFrustumHeight = fNewHeight;
}

void CCamera::UpdateProjection()
{
	int	iPrevMatrixMode;
	glGetIntegerv(GL_MATRIX_MODE, &iPrevMatrixMode);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

//	glOrtho(fLeftClip, fRightClip, fBottomClip, fTopClip, fNearClip, fFarClip);
	glOrtho(
		vOrigin.x - fFrustumWidth/2,  vOrigin.x + fFrustumWidth/2, 
		vOrigin.y - fFrustumHeight/2, vOrigin.y + fFrustumHeight/2, 
		fNearClip, fFarClip
	);

	glMatrixMode(iPrevMatrixMode);

	/// TODO: list of event callbacks
//	CText::fFontScaleFactor = fFrustumHeight / fViewportHeight;
//	CTransformArrow::fArrowLength = fFrustumHeight;
}

void CCamera::SetViewport(int x, int y, int w, int h)
{
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;

	fViewportWidth	= double(w);
	fViewportHeight	= double(h);

	glViewport(x, y, w, h);

	double fAspect = fViewportWidth / fViewportHeight;
	fFrustumWidth = fFrustumHeight * fAspect;

	UpdateProjection();
}

void CCamera::Move(double fX, double fY)
{
	vOrigin.x -= fX * fFrustumWidth;
	vOrigin.y += fY * fFrustumHeight;
}

void CCamera::SetFrustum(double fWidth, double fHeight, double fNear, double fFar)
{
	this->fFrustumWidth		= fWidth;
	this->fFrustumHeight	= fHeight;
	this->fNearClip			= fNear;
	this->fFarClip			= fFar;
}

void CCamera::GetFrustum(double *pWidth, double *pHeight, double *pNearClip, double *pFarClip)
{
	*pWidth		= this->fFrustumWidth;
	*pHeight	= this->fFrustumHeight;
	*pNearClip	= this->fNearClip;
	*pFarClip	= this->fFarClip;
}

bool CCamera::GetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::GetProperties(paramsIn, paramsOut);

	if (paramsIn.GetInt(_T("get_frustum"), false) || paramsIn.GetInt(_T("get_all"), false))
	{
		CDoubleArray *pFrustumArray = new CDoubleArray;
		if (!pFrustumArray || !pFrustumArray->Create(1, 4))
		{
			paramsOut.AppendString(_T("error"), _T("Not enough memory"));
			if (pFrustumArray)
				delete pFrustumArray;
		}
		else
		{
			pFrustumArray->SetAtAbsoluteIndex(fFrustumWidth, 0);
			pFrustumArray->SetAtAbsoluteIndex(fFrustumHeight, 1);
			pFrustumArray->SetAtAbsoluteIndex(fNearClip, 2);
			pFrustumArray->SetAtAbsoluteIndex(fFarClip, 3);
			
			paramsOut.SetArray(_T("frustum"), pFrustumArray);
		}
	}
	return true;
}

bool CCamera::SetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::SetProperties(paramsIn, paramsOut);

	CDoubleArray *pFrustum = paramsIn.GetArray(_T("frustum"));

	if (pFrustum)
	{
		if (pFrustum->GetNumberOfItems()!=4)
		{
			paramsOut.AppendString(_T("error"), _T("'frustum' must contain 4 values"));
		}
		else
		{
			double *pF = pFrustum->GetPointer();

			double fAspect = fViewportWidth / fViewportHeight;
			double w  = pF[1] * fAspect;

			SetFrustum(w, pF[1], pF[2], pF[3]);
			UpdateProjection();
		}
	}

	double	fFrustumHeight = 0;

	if (paramsIn.GetFloat(_T("frustum_height"), &fFrustumHeight))
	{
			double fAspect = fViewportWidth / fViewportHeight;
			double w  = fFrustumHeight * fAspect;

			SetFrustum(w, fFrustumHeight, fNearClip, fFarClip);
			UpdateProjection();
	}
	return true;
}
