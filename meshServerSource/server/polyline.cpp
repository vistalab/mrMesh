#include "stdafx.h"
#include "polyline.h"

const char* CPolyLine::pszClassName = "polyline";

CPolyLine::CPolyLine(const SceneInfo *pSI, int iActorID)
:CActor(pSI, iActorID)
{
	nPoints = 0;
	pPoints = NULL;
	fLineWidth = 1.0f;
}

CPolyLine::~CPolyLine()
{
	Destroy();
}

void CPolyLine::Render()
{
	glColor4ubv(cColor);
	glLineWidth(fLineWidth);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_STRIP);
	// 2004.07.22 RFD: modified the loop below to allow multiple 
	// polylines to be rendered with one call by adding the 'breakpoint'
	// code of '999'.
	int i,rc;
	for (i=0; i<nPoints; i++){
		if(pPoints[i].x==999.0 && pPoints[i].y==999.0 && pPoints[i].z==999.0){
			glEnd();
			rc = (unsigned char)(16.0*rand()/RAND_MAX+0.5) - 8;
			glColor4ub(cColor.r+rc, cColor.g+rc, cColor.b+rc, cColor.a);
			glBegin(GL_LINE_STRIP);
		}else{
			glVertex3dv(pPoints[i]);
		}
	}
	glEnd();
}

void CPolyLine::Destroy()
{
	if (pPoints)
	{
		delete[] pPoints;
		pPoints = NULL;
		nPoints = 0;
	}
}

bool CPolyLine::SetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::SetProperties(paramsIn, paramsOut);

	//--------- points data

	CDoubleArray *pPointsArray = paramsIn.GetArray(wxString::FromAscii("points"));
	if (pPointsArray)
	{
		if ((pPointsArray->GetNumberOfDimensions() != 2) || (pPointsArray->GetSizes()[0]!=3))
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("'points' must be [3xN]"));
		else if (pPointsArray->GetSizes()[1] < 2)
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Line must have at least 2 points"));
		else
		{
			this->Destroy();

			nPoints = pPointsArray->GetSizes()[1];
			pPoints = new CVector[nPoints];

			if (!pPoints)
			{
				paramsOut.AppendString(wxString::FromAscii("error"),
                                       wxString::FromAscii("Not enough memory"));
				nPoints = 0;
			}
			else
			{
				double *pData = pPointsArray->GetPointer();
				for (int i = 0; i < nPoints; i++)
				{
					pPoints[i].x = pData[0];
					pPoints[i].y = pData[1];
					pPoints[i].z = pData[2];
					pData += 3;
				}
			}
		}
	}

	//----------- color

	CDoubleArray *pColorArray = paramsIn.GetArray(wxString::FromAscii("color"));
	if (pColorArray)
	{
		int nComp = pColorArray->GetNumberOfItems();
		if ((nComp != 3) && (nComp != 4))
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("'color' must be size of [3] or [4]"));
		}
		else
		{
			double *pData = pColorArray->GetPointer();
			for (int i=0; i<nComp; i++)
				cColor.m_ucColor[i] = (unsigned char)pData[i];
			if (nComp == 3)
				cColor.a = 255;
		}
	}

	//------------ width

	double fNewWidth;
	if (paramsIn.GetFloat(wxString::FromAscii("width"), &fNewWidth))
		fLineWidth = fNewWidth;

	return true;
}

bool CPolyLine::GetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::GetProperties(paramsIn, paramsOut);

	//--------- points data

	if (pPoints)
	{
		if (paramsIn.GetInt(wxString::FromAscii("get_points"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
		{
			CDoubleArray *pPointsArray = NULL;
			try
			{
				pPointsArray = new CDoubleArray;
				pPointsArray->Create(2, 3, nPoints);
				
				double *pData = pPointsArray->GetPointer();

				for (int i=0; i<nPoints; i++)
				{
					pData[0] = pPoints[i].x;
					pData[1] = pPoints[i].y;
					pData[2] = pPoints[i].z;
					pData += 3;
				}
			}
			catch(...)
			{
				if (pPointsArray)
				{
					delete pPointsArray;
					pPointsArray = NULL;
				}
				paramsOut.AppendString(wxString::FromAscii("error"),
                                       wxString::FromAscii("Memory error"));
			}

			if (pPointsArray)
				paramsOut.SetArray(wxString::FromAscii("points"),
                                   pPointsArray);
		}
	}

	//----------- color

	if (paramsIn.GetInt(wxString::FromAscii("get_color"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
	{
		CDoubleArray *pColorArray = cColor.ToArray();
		if (pColorArray)
			paramsOut.SetArray(wxString::FromAscii("color"), pColorArray);
		else
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Memory error"));
	}

	//------------ width

	if (paramsIn.GetInt(wxString::FromAscii("get_width"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
		paramsOut.SetFloat(wxString::FromAscii("width"), fLineWidth);

	return true;
}
