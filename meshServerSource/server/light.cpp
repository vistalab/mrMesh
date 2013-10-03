#include "stdafx.h"
#include "light.h"
#include "camera.h"

const char* CLight::pszClassName = "light";

int				CLight::iGLLightsCounter	= 0;
GLUquadricObj*	CLight::quadObjSphere		= NULL;

CLight::CLight(const SceneInfo *pSI, int iActorID)
:CActor(pSI, iActorID), bEnabled(false), iType(LT_OMNI)
{
	iGLLightID = GL_LIGHT0 + pSceneInfo->nLights;

	m_Diffuse[0]	= m_Diffuse[1]	= m_Diffuse[2]	= m_Diffuse[3]	= 1.0f;
	m_Specular[0]	= m_Specular[1]	= m_Specular[2]	= m_Specular[3]	= 1.0f;
	m_Ambient[0]	= m_Ambient[1]	= m_Ambient[2]	= 0.0f;
	m_Ambient[3]	= 1.0f;

	m_fCutoff = 180.0f;

	if (!iGLLightsCounter)
	{
		quadObjSphere = gluNewQuadric();
		gluQuadricDrawStyle(quadObjSphere, GLU_FILL);
		gluQuadricNormals(quadObjSphere, GLU_SMOOTH);
	}

	iGLLightsCounter++;

	Enable();
}

CLight::~CLight()
{
	Enable(false);

	iGLLightsCounter --;

	if (!iGLLightsCounter)
	{
		gluDeleteQuadric(quadObjSphere);
		quadObjSphere = NULL;
	}
}

void CLight::Enable(bool bEnable)
{
	if (bEnable)
	{
		glEnable(iGLLightID);
		glLightfv(iGLLightID, GL_DIFFUSE, m_Diffuse);
		glLightfv(iGLLightID, GL_SPECULAR, m_Specular);	
		glLightf(iGLLightID, GL_SPOT_CUTOFF, m_fCutoff);
	}
	else
		glDisable(iGLLightID);

	bEnabled = bEnable;
}


void CLight::Render()
{
//	if (!bEnabled)
//		return;

	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	if (iType == LT_OMNI)
	{
		float pos[4] = {0, 0, 0, 1.0f};
		glLightfv(iGLLightID, GL_POSITION, pos);
	}

	if ((iType == LT_OMNI) && (m_fCutoff > 90.0f))
	{
		float pos[4] = {0, 0, 0, 1.0f};
//		CVector vAbsPos = vOrigin;// * pSceneInfo->pCamera->mRotation;
//		double pos[4] = {vAbsPos.x, vAbsPos.y, vAbsPos.z, 1.0f};
		glLightfv(iGLLightID, GL_POSITION, pos);

		glDisable(GL_LIGHTING);
		glColor3f(1, 1, 1);
		gluSphere(quadObjSphere, 2.5f, 4, 4);
	}
	else
	{
		if (iType == LT_OMNI)
		{
			CVector vDir = CVector(0, 0, -1);// * mRotation;
			float fDir[4] = {vDir.x, vDir.y, vDir.z, 1.0f};
			glLightfv(iGLLightID, GL_SPOT_DIRECTION, fDir);

			//glDisable(GL_LIGHTING);
			//glColor3f(1,1,1);
			//glBegin(GL_LINES);
			//	glVertex3f(0, 0, 0);
			//	double fLen = 50;
			//	glVertex3f(vDir.x * fLen, vDir.y * fLen, vDir.z * fLen);
			//glEnd();
		}

		glColor3f(0.6f, 0.6f, 0.6f);
		glBegin(GL_TRIANGLE_FAN);
			glVertex3f(0, 0, 4);

			glVertex3f(-3,-3, -2);
			glVertex3f(3, -3, -2);
			glVertex3f(3, 3, -2);
			glVertex3f(-3, 3, -2);
			glVertex3f(-3, -3, -2);
		glEnd();
		
		glDisable(GL_LIGHTING);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
			glVertex3f(-3,-3, -2);
			glVertex3f(3, -3, -2);
			glVertex3f(3, 3, -2);
			glVertex3f(-3, 3, -2);
		glEnd();
	}

	glPopAttrib();
}

bool CLight::GetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::GetProperties(paramsIn, paramsOut);

	try{
		//if (paramsIn.GetInt("get_type", false) || paramsIn.GetInt("get_all", false))
		//{
		//	paramsOut.SetString("type", (iType == LT_OMNI) ? "omni" : "directed");
		//}
		if (paramsIn.GetInt(wxString::FromAscii("get_diffuse"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
		{
			CDoubleArray *pDiffuse = new CDoubleArray;
			pDiffuse->Create(1, 3);
			for (int i=0; i<3; i++)
				pDiffuse->SetAtAbsoluteIndex((double)m_Diffuse[i], i);
			paramsOut.SetArray(wxString::FromAscii("diffuse"), pDiffuse);
		}
		if (paramsIn.GetInt(wxString::FromAscii("get_ambient"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
		{
			CDoubleArray *pAmbient = new CDoubleArray;
			pAmbient->Create(1, 3);
			for (int i=0; i<3; i++)
				pAmbient->SetAtAbsoluteIndex((double)m_Ambient[i], i);
			paramsOut.SetArray(wxString::FromAscii("ambient"), pAmbient);
		}
		return true;
	}
	catch(...)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Not enough memory"));
		return false;
	}
}

bool CLight::SetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::SetProperties(paramsIn, paramsOut);

	/////--------

	wxString	strValue;

	CDoubleArray	*pArray;
	double		*pData;
/*
	if (paramsIn.GetString("type", &strValue))
	{
		if (strValue.CmpNoCase("omni") == 0)
			iType = LT_OMNI;
		else if (strValue.CmpNoCase("directed") == 0)
			iType = LT_DIRECTED;
		else
			paramsOut.AppendString("error", "Unknown light type");
	}
*/
	////// ----- color

	pArray = paramsIn.GetArray(wxString::FromAscii("diffuse"));
	if (pArray)
	{
		if (pArray->GetNumberOfItems() != 3)
			paramsOut.AppendString(wxString::FromAscii("error"), wxString::FromAscii("Invalid size of 'diffuse'"));
		else
		{
			pData = pArray->GetPointer();
			for (int i=0; i<3; i++)
				m_Diffuse[i] = (float)pData[i];
			glLightfv(iGLLightID, GL_DIFFUSE, m_Diffuse);
		}
	}

	pArray = paramsIn.GetArray(wxString::FromAscii("ambient"));
	if (pArray)
	{
		if (pArray->GetNumberOfItems() != 3)
			paramsOut.AppendString(wxString::FromAscii("error"), wxString::FromAscii("Invalid size of 'ambient'"));
		else
		{
			pData = pArray->GetPointer();
			for (int i=0; i<3; i++)
				m_Ambient[i] = (float)pData[i];
			glLightfv(iGLLightID, GL_AMBIENT, m_Ambient);	
		}
	}

	double fValue;
	if (paramsIn.GetFloat(wxString::FromAscii("cutoff"), &fValue))
	{
		m_fCutoff = (float)fValue;
		
		if ((m_fCutoff > 180.0f) || ((m_fCutoff < 180.0f) && (m_fCutoff > 90.0f)))
			m_fCutoff = 180.0f;
		else if (m_fCutoff < 0)
			m_fCutoff = 0;

		glLightf(iGLLightID, GL_SPOT_CUTOFF, m_fCutoff);
	}

	return true;
}
