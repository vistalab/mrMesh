#include "stdafx.h"
#include "transformarrow.h"

#include "camera.h"

//double CTransformArrow::fArrowLength = 1.0f;
const char* CTransformArrow::pszClassName = "arrow";

CTransformArrow::CTransformArrow(const SceneInfo *pSI, int iActorID, const CVector &vArrow)
:CActor(pSI, iActorID),
vDirection(vArrow),
cColor(200, 255, 50)

{
	vDirection.Normalize();
	
	bVisible = false;
}

CTransformArrow::~CTransformArrow()
{
}

void CTransformArrow::Render()
{
	if (!bVisible)
		return;

	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glColor3ubv(cColor);
	
	int iRenderMode;

	glGetIntegerv(GL_RENDER_MODE, &iRenderMode);

	if (iRenderMode == GL_SELECT)
		glLineWidth(10.0f);
	else
		glLineWidth(3.0f);

	double fLen = pSceneInfo->pCamera->GetViewportHeight();
	
	glBegin(GL_LINES);
		glVertex3f(0, 0, 0);
//		glVertex3f(vDirection.x * fArrowLength, vDirection.y * fArrowLength, vDirection.z * fArrowLength);
		glVertex3f(vDirection.x * fLen, vDirection.y * fLen, vDirection.z * fLen);
	glEnd();

	glPopAttrib();	//GL_ENABLE_BIT
}

void CTransformArrow::Show(bool bShow)
{
	bVisible = bShow;
}
