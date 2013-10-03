#include "stdafx.h"
#include "imagequad.h"

const char* CImageQuad::pszClassName = "image";

CImageQuad::CImageQuad(const SceneInfo *pSI, int iActorID)
:CActor(pSI, iActorID), bAlphaChannel(false), fWidth(1.0), fHeight(1.0)
{
	glGenTextures(1, &uTextureID);
}

CImageQuad::~CImageQuad()
{
	glDeleteTextures(1, &uTextureID);
}

void CImageQuad::Render()
{
	float	fHalfWidth	= fWidth / 2;
	float	fHalfHeight	= fHeight / 2;

	glPushAttrib(GL_ENABLE_BIT);

	glEnable(GL_TEXTURE_2D);
//	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);

	glColor3f(1,1,1);
	glBindTexture(GL_TEXTURE_2D, uTextureID);

	if (bAlphaChannel)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glBegin(GL_QUADS);
    glNormal3f(0, 0, -1.0f);
    glTexCoord2f(0, 0);
    glVertex2f(-fHalfWidth, -fHalfHeight);
    glTexCoord2f(0, 1);
    glVertex2f(-fHalfWidth,  fHalfHeight);
    glTexCoord2f(1, 1);
    glVertex2f( fHalfWidth,  fHalfHeight);
    glTexCoord2f(1, 0);
    glVertex2f( fHalfWidth, -fHalfHeight);
	glEnd();

	glPopAttrib();	//GL_ENABLE_BIT
}

bool CImageQuad::SetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::SetProperties(paramsIn, paramsOut);

	double		fValue;
	wxString	strValue;

	////-------- texture from file

	if (paramsIn.GetFloat(wxString::FromAscii("width"), &fValue))
		fWidth = (float)fValue;
	if (paramsIn.GetFloat(wxString::FromAscii("height"), &fValue))
		fHeight = (float)fValue;

	if (paramsIn.GetString(wxString::FromAscii("texture_file"), &strValue))
	{
		wxImage image;
		if (!image.LoadFile(strValue))
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Failed to load file"));
		else
		{
			SetTexture(image.GetWidth(), image.GetHeight(), 3, GL_UNSIGNED_BYTE, image.GetData());
		}
	}

	////-------- user-supplied texture

	CDoubleArray *pTexture = paramsIn.GetArray(wxString::FromAscii("texture"));
	if (pTexture)
	{
		int iTexW,
			iTexH;

		if (!paramsIn.GetInt(wxString::FromAscii("tex_width"), &iTexW) || !paramsIn.GetInt(wxString::FromAscii("tex_height"), &iTexH))
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("'tex_width' and 'tex_height' parameters are required"));
		}
		else
		if ((pTexture->GetNumberOfDimensions() != 2) ||
			((pTexture->GetSizes()[0] != 3) && (pTexture->GetSizes()[0] != 4)) ||
			(pTexture->GetSizes()[1] != iTexW*iTexH))
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Invalid size of 'texture' array"));
		}
		else
		{
		    double *dTex = pTexture->GetPointer();
		    int nVals = iTexW*iTexH*pTexture->GetSizes()[0];
		    float *fTex = (float *)malloc(nVals*sizeof(float));
		    for(int i=0; i<nVals; i++) fTex[i] = (float)dTex[i];
		    SetTexture(iTexW, iTexH, pTexture->GetSizes()[0], GL_FLOAT, fTex);
		    free(fTex);
			//SetTexture(iTexW, iTexH, pTexture->GetSizes()[0], GL_DOUBLE, pTexture->GetPointer());
		}
	}
	return true;
}

bool CImageQuad::GetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::GetProperties(paramsIn, paramsOut);

	if (paramsIn.GetInt(wxString::FromAscii("get_width"), FALSE) || paramsIn.GetInt(wxString::FromAscii("get_all"), FALSE))
		paramsOut.SetFloat(wxString::FromAscii("width"), fWidth);

	if (paramsIn.GetInt(wxString::FromAscii("get_width"), FALSE) || paramsIn.GetInt(wxString::FromAscii("get_all"), FALSE))
		paramsOut.SetFloat(wxString::FromAscii("width"), fWidth);
	
	return true;
}

void CImageQuad::SetTexture(int iWidth, int iHeight, int nColorComp, int iFormat, void *pImage)
{
	glPushAttrib(GL_ENABLE_BIT);

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, uTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	wxASSERT((nColorComp == 3) || (nColorComp == 4));

	bAlphaChannel = (nColorComp == 4);
	int iPixelFormat = bAlphaChannel ? GL_RGBA : GL_RGB;

	glTexImage2D(GL_TEXTURE_2D, 0, nColorComp, iWidth, iHeight, 0, iPixelFormat, iFormat, pImage);

	glPopAttrib();	//GL_ENABLE_BIT
}
