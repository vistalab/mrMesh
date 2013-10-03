
/*
 * HISTORY:
 *  2004.06.03 RFD: Added position (the cursor origin) as a return value in get_selection.
 *                  Added the new command "set_selection".
 *
 */

#include "stdafx.h"
#include "cursor.h"
#include "mesh.h"

const char* CCursor::pszClassName = "cursor";

BEGIN_HANDLER_TABLE(CCursor)
	COMMAND_HANDLER((char* const)"get_selection", CCursor::CommandGetSelection)
  COMMAND_HANDLER((char* const)"set_selection", CCursor::CommandSetSelection)
END_HANDLER_TABLE()

CCursor::CCursor(const SceneInfo *pSI, int iActorID)
:CActor(pSI, iActorID)
{
	uDisplayList = glGenLists(1);
	wxLogDebug(wxString::FromAscii("uList = %u\n"), uDisplayList);
	
	CompileList();

	OnSelectActor(NULL);
}

CCursor::~CCursor()
{
	glDeleteLists(uDisplayList, 1);
}

void CCursor::Render()
{
//	glCallList(uDisplayList);
  DrawArrows(); 	//linux issues
}

void CCursor::DrawArrows()
{
	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_LIGHTING);

	glLineWidth(2.0f);

	double	fLen = 25.0f;
	double	fArrowX = 3.0f;
	double	fArrowY = 1.0f;

	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
		//X axis

		glColor3f(1, 0, 0);
		glVertex3f(0,0,0);
		glVertex3f(fLen, 0, 0);

		// arrowhead
		glVertex3f(fLen-fArrowX, -fArrowY, 0);
		glVertex3f(fLen, 0, 0);
		glVertex3f(fLen-fArrowX, fArrowY, 0);
		glVertex3f(fLen, 0, 0);

		//Y axis

		glColor3f(0, 1, 0);
		glVertex3f(0,0,0);
		glVertex3f(0, fLen, 0);

		// arrowhead
		glVertex3f(-fArrowY, fLen-fArrowX, 0);
		glVertex3f(0, fLen, 0);
		glVertex3f(fArrowY, fLen-fArrowX, 0);
		glVertex3f(0, fLen, 0);

		//Z axis

		glColor3f(0, 0, 1);
		glVertex3f(0,0,0);
		glVertex3f(0, 0, fLen);

		// arrowhead
		glVertex3f(0, -fArrowY, fLen-fArrowX);
		glVertex3f(0, 0, fLen);
		glVertex3f(0, fArrowY, fLen-fArrowX);
		glVertex3f(0, 0, fLen);

		// negative directions

		glColor3f(0.3f, 0, 0);
		glVertex3f(0,0,0);
		glVertex3f(-fLen, 0, 0);

		glColor3f(0, 0.3f, 0);
		glVertex3f(0,0,0);
		glVertex3f(0, -fLen, 0);

		glColor3f(0, 0, 0.3f);
		glVertex3f(0,0,0);
		glVertex3f(0, 0, -fLen);
	glEnd();

	glPopAttrib();	//GL_ENABLE_BIT  
}

void CCursor::CompileList()
{
	glNewList(uDisplayList, GL_COMPILE);

  DrawArrows();
	
	glEndList();
}

void CCursor::OnSelectActor(CActor *pActor)
{
	if (!pActor)
	{
		iSelectedActor = -1;
		iSelectedVertex = -1;
		return;

	}

	iSelectedActor = pActor->GetID();


	if (pActor->GetClassName() != CMesh::pszClassName)
	{
		iSelectedVertex = -1;
		return;
	}

	CMesh *pMesh = (CMesh *)pActor;
	iSelectedVertex = pMesh->FindClosestVertex(
			(vOrigin - pMesh->vOrigin) * pMesh->mRotation
		);


	if (iSelectedVertex != -1)
	{
		CVector v = pMesh->pVertices[iSelectedVertex].vCoord * pMesh->mRotation + pMesh->vOrigin;
		wxLogDebug(wxString::FromAscii("Mesh vertex = {%f, %f, %f}\n"), v.x, v.y, v.z);
	}
}

bool CCursor::CommandGetSelection(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	paramsOut.SetInt(wxString::FromAscii("actor"), iSelectedActor);
	paramsOut.SetInt(wxString::FromAscii("vertex"), iSelectedVertex);
  // 2004.06.03 RFD: Added position (the cursor origin) as a return value.
	CDoubleArray *v = new CDoubleArray;
  v->Create(1,3);
  v->SetValue(vOrigin.x, 0);
	v->SetValue(vOrigin.y, 1);
	v->SetValue(vOrigin.z, 2);
  paramsOut.SetArray(wxString::FromAscii("position"), v);
	return true;
}


bool CCursor::CommandSetSelection(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CDoubleArray *pPosition = paramsIn.GetArray(wxString::FromAscii("position"));
  if (!pPosition){
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("No poition given- set position field to a 3x1."));
		return false;
  }
  if (pPosition->GetSizes()[0] != 3){
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Invalid position array- must be 3x1"));
		return false;
	}
	double *fPosition = pPosition->GetPointer();
	vOrigin = CVector(fPosition[0], fPosition[1], fPosition[2]);
  //delete pPosition;
	return true;
}
