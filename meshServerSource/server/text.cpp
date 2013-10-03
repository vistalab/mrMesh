#include "stdafx.h"
#include "text.h"
#include "camera.h"

const char* CText::pszClassName = "text";

int		CText::iTextActorsCounter = 0;
GLuint	CText::uFontTexture = -1;
GLuint	CText::uFontListsBase = -1;

CText::CText(const SceneInfo *pSI, int iActorID)
:CActor(pSI, iActorID)
{
	iGlyphSize		= 16;
	iCharSpacing	= 12;

	if (!iTextActorsCounter)
	{
		glGenTextures(1, &uFontTexture);
		if (!LoadFont())
			wxMessageDialog(NULL, wxString::FromAscii("Font bitmap was not loaded"),
                            wxString::FromAscii("Error"), wxOK | wxCENTRE).ShowModal();
	}
	iTextActorsCounter ++;
}

CText::~CText()
{
	iTextActorsCounter --;
	if (!iTextActorsCounter)
	{
		glDeleteLists(uFontListsBase, 256);
		glDeleteTextures(1, &uFontTexture);
	}
}

bool CText::LoadFont()
{
	wxImage image;

	if (!image.LoadFile(wxGetCwd()+wxString::FromAscii("/data/font.bmp")))
		return false;

	glPushAttrib(GL_ENABLE_BIT);

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, uFontTexture);

	// GL_NEAREST gives us perfect font sharpness
	// we also get no texture distortion because of controlling fFontScaleFactor
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// wxImage seems to be flipped so mirror it manually
	int	nTotalBytes = image.GetWidth() * image.GetHeight() * 3;
	int	nScanlineWidth = image.GetWidth()*3;
	unsigned char *pTexture = new unsigned char[nTotalBytes];
	for (int iImageRow = 0; iImageRow < image.GetHeight(); iImageRow++)
	{
		memmove(
			pTexture + iImageRow*nScanlineWidth, 
			image.GetData() + nTotalBytes - (iImageRow+1)*nScanlineWidth,
			nScanlineWidth
		);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, 3, image.GetWidth(), image.GetHeight(), 0, 
		GL_RGB, GL_UNSIGNED_BYTE, /*image.GetData()*/pTexture);
	delete[] pTexture;

	// generate display lists

	uFontListsBase = glGenLists(256);

	int		iRow, iCol;
	double	fTexX, fTexY;

	//double fGlyphSize = double(iGlyphSize);

	for (iRow = 0, fTexY = 0.0f; iRow < 16; iRow++, fTexY += 0.0625f)
	{
		for (iCol = 0, fTexX = 0.0f; iCol < 16; iCol++, fTexX += 0.0625f)
		{
			glNewList(uFontListsBase + iRow*16 + iCol, GL_COMPILE);
				glNormal3f(0, 0, 1.0f);
				glBegin(GL_QUADS);
					glTexCoord2f(fTexX, 1.0f - fTexY - 0.0625f);
					glVertex2i(0, 0);
			
					glTexCoord2f(fTexX + 0.0625f, 1.0f - fTexY - 0.0625f);
					glVertex2i(iGlyphSize, 0);

					glTexCoord2f(fTexX + 0.0625f, 1 - fTexY);
					glVertex2i(iGlyphSize, iGlyphSize);

					glTexCoord2f(fTexX, 1.0f - fTexY);
					glVertex2i(0, iGlyphSize);
				glEnd();
				glTranslatef(double(iCharSpacing), 0.0f, 0.0f);
			glEndList();
		}
	}

	glPopAttrib();	//GL_ENABLE_BIT

	return true;
}

bool CText::GetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::GetProperties(paramsIn, paramsOut);

	if (paramsIn.GetInt(wxString::FromAscii("get_text"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
	{
		paramsOut.SetString(wxString::FromAscii("text"), strText);
	}
	if (paramsIn.GetInt(wxString::FromAscii("get_color"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
	{
		CDoubleArray *pColorArray = cColor.ToArray();
		if (pColorArray)
			paramsOut.SetArray(wxString::FromAscii("color"), pColorArray);
		else
			paramsOut.AppendString(wxString::FromAscii("error"), wxString::FromAscii("Memory error"));
	}

	return true;
}

bool CText::SetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::SetProperties(paramsIn, paramsOut);

	wxString	strValue;
	CDoubleArray	*pArray = NULL;

	if (paramsIn.GetString(wxString::FromAscii("text"), &strValue))
	{
		strText = strValue;
	}
	
	pArray = paramsIn.GetArray(wxString::FromAscii("color"));
	if (pArray)
	{
		if (!cColor.FromArray(pArray))
			paramsOut.AppendString(wxString::FromAscii("error"),
                            wxString::FromAscii("Invalid array: 'color'"));
	}

	return true;
}

void CText::Render()
{
	if (strText.IsEmpty())
		return;

	glPushAttrib(GL_ENABLE_BIT);
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	if (IsAttachedToCameraSpace())
		glDisable(GL_DEPTH_TEST);
	else
		glDepthFunc(GL_LEQUAL);

	glBindTexture(GL_TEXTURE_2D, uFontTexture);
	glColor4ubv(cColor);

	glPushMatrix();

	double	fFrustumHeight = pSceneInfo->pCamera->GetFrustumHeight();

	if (IsAttachedToCameraSpace())
	{
		CVector vPosition = vOrigin;
		glTranslatef(
			vPosition.x * fFrustumHeight + pSceneInfo->pCamera->vOrigin.x, 
			vPosition.y * fFrustumHeight + pSceneInfo->pCamera->vOrigin.y, 
			0);
	}
	else
	{
		CVector vPosition = vOrigin * pSceneInfo->pCamera->mRotation;
		glTranslatef(vPosition.x, vPosition.y, vPosition.z);
	}

	double fScale = fFrustumHeight / pSceneInfo->pCamera->GetViewportHeight();
	glScalef(fScale, fScale, 1.0f);

	glTranslatef( - double(strText.Len() * iCharSpacing / 2), -double(iCharSpacing/2), 0);

	glListBase(uFontListsBase);
	glCallLists(strText.Len(), GL_UNSIGNED_BYTE, strText.GetData());

	glPopMatrix();

	glPopAttrib();	//GL_DEPTH_BUFFER_BIT
	glPopAttrib();	//GL_ENABLE_BIT
}

void CText::Move(CVector &vDelta)
{
	// RFD: needed to create CVector separately for gcc.
	CVector cv;
	if (IsAttachedToCameraSpace())
		cv = vDelta / pSceneInfo->pCamera->GetFrustumHeight();
	else
		cv = vDelta;
	CActor::Move(cv);
}
