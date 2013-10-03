#include "stdafx.h"

#include "mesh.h"
#include "roi.h"
#include "grayVolume.h"
#include "vtkfilter.h"

#include "mrm_io.h"

#include "vtkTubeFilter.h"
#include "vtkTriangleFilter.h"

const char* CMesh::pszClassName = "mesh";
CColor	CMesh::cDefaultColor(192, 192, 192, 255);

BEGIN_HANDLER_TABLE(CMesh)
	/// redeclare set/get since virtual ProcessCommand uses class handler table
	COMMAND_HANDLER((char* const)"set", CMesh::SetProperties)
	COMMAND_HANDLER((char* const)"get", CMesh::GetProperties)

	COMMAND_HANDLER((char* const)"cube", CMesh::CommandCube)
	COMMAND_HANDLER((char* const)"tube", CMesh::CommandTube)
	COMMAND_HANDLER((char* const)"grid", CMesh::CommandGrid)

	COMMAND_HANDLER((char* const)"open_gray", CMesh::CommandOpenGray)
	COMMAND_HANDLER((char* const)"open_class", CMesh::CommandOpenClass)
	COMMAND_HANDLER((char* const)"open_mrm", CMesh::CommandOpenMRM)
	COMMAND_HANDLER((char* const)"save_mrm", CMesh::CommandSaveMRM)

	COMMAND_HANDLER((char* const)"paint", CMesh::CommandPaint)
	COMMAND_HANDLER((char* const)"reset_color", CMesh::CommandResetColor)
	COMMAND_HANDLER((char* const)"apply_roi", CMesh::CommandApplyROI)
	COMMAND_HANDLER((char* const)"curvatures", CMesh::CommandCurvatures)

	COMMAND_HANDLER((char* const)"decimate", CMesh::CommandDecimate)
	COMMAND_HANDLER((char* const)"smooth", CMesh::CommandSmooth)

	COMMAND_HANDLER((char* const)"modify_mesh", CMesh::CommandModifyMesh)
	COMMAND_HANDLER((char* const)"set_mesh", CMesh::CommandSetMesh)
	COMMAND_HANDLER((char* const)"build_mesh", CMesh::CommandBuildMesh)
END_HANDLER_TABLE()

CMesh::CMesh(const SceneInfo *pSI, int iActorID)
:CActor(pSI, iActorID)
{
	pVertices = NULL;
	pTriangles = NULL;

	nVertices = 0;
	nTriangles = 0;
}

CMesh::~CMesh()
{
	Destroy();
}

void CMesh::Destroy()
{
	if (pVertices)
	{
		wxASSERT(pTriangles);

		if (pSceneInfo->bTransparentMeshEnabled)
			pSceneInfo->pPolyBuffer->UnregisterObject(this);

		delete[] pVertices;
		delete[] pTriangles;

		pVertices = NULL;
		pTriangles = NULL;
	}
	wxASSERT(!pTriangles);
	nVertices = nTriangles = 0;
}


bool CMesh::CommandCube(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	Destroy();

	nVertices = 3;
	nTriangles = 1;
	pVertices = new Vertex[nVertices];
        pTriangles = new Triangle_uint32[nTriangles];
	

	pVertices[0].vCoord = CVector(0, 20, 0);
	pVertices[1].vCoord = CVector(-10, -10, 0);
	pVertices[2].vCoord = CVector(20, 0, 10);

        CColor v0Color(50, 200, 250, 120);
        CColor v1Color(250, 20, 170, 200);
        CColor v2Color(200, 250, 30, 50);
	pVertices[0].cColor = v0Color;
	pVertices[1].cColor = v1Color;
	pVertices[2].cColor = v2Color;

	CVector vNormal(
		(pVertices[0].vCoord-pVertices[1].vCoord) ^
		(pVertices[2].vCoord - pVertices[1].vCoord)
	);
	vNormal.Normalize();

	int i;
	for (i=0; i<3; i++)
		pVertices[i].vNormal = vNormal;
	for (i=0; i<3; i++)
		pTriangles[0].v[i] = i;

	if (pSceneInfo->bTransparentMeshEnabled)
		pSceneInfo->pPolyBuffer->RegisterObject(this);

	return true;
}

bool CMesh::CommandTube(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	bool	bResult = false;
	int		i;

	CDoubleArray *pPointsArray = paramsIn.GetArray(wxString::FromAscii("points"));
	if (!pPointsArray){
		paramsOut.AppendString(wxString::FromAscii("error"), wxString::FromAscii("No points given"));
		return false;
	}
	if ((pPointsArray->GetNumberOfDimensions() != 2) 
			|| (pPointsArray->GetSizes()[0] != 3)){
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Invalid points array- must be 3xN"));
		return false;
	}

	double	fRadius = paramsIn.GetFloat(wxString::FromAscii("radius"), 1.0);
	int		nSides	= paramsIn.GetInt(wxString::FromAscii("sides"), 6);
	int		bCap	= paramsIn.GetInt(wxString::FromAscii("cap"), 1);
	CDoubleArray *pColor = paramsIn.GetArray(wxString::FromAscii("color"));
	
	if (nSides < 3)
		nSides = 3;

	// We now allow the client to specify multiple tubes. They do this 
	// by putting a 'break' in the points list that indicates the start 
	// of a new tube.
	double	*pPointData = pPointsArray->GetPointer();
	// Note the end of the array so we don't go past it
	double	*pPointDataEnd = pPointData+pPointsArray->GetSizes()[1]*3;
	double *tmpPtr = pPointData;
	int nPoints;
	double breakVal = 999;
	bool done=false, newTube;
	vtkIdType     *pIdList = NULL;
	vtkPoints		  *pPoints = NULL;
	vtkCellArray	*pLines	 = NULL;
	vtkPolyData		*pPD		= NULL;
	vtkPolyData		*pOut		= NULL;
	vtkTubeFilter	*pTubeFilter =	NULL;
	vtkCleanPolyData *pPDcleaner		= NULL;

	try{
		pOut	= vtkPolyData::New();
		pPD		= vtkPolyData::New();
		pTubeFilter	= vtkTubeFilter::New();
		pPoints	= vtkPoints::New(VTK_DOUBLE);
		pLines	= vtkCellArray::New();

		// The idea is to build a multi-tube object by creating a set of separate lines 
		// and then telling vtk to turn them into tubes.
		int iter=0;
		while(!done){
			iter++;
			// fill in points coords
			//

			// Skip extraneous leading newTube markers
			while(pPointData[0]==breakVal && pPointData[1]==breakVal 
						&& pPointData[2]==breakVal && pPointData<pPointDataEnd){
				pPointData += 3;
			}
			if(pPointData>=pPointDataEnd)
				done = true;

			// Count the points for this tube
			newTube = false;
			nPoints = 0;
			tmpPtr = pPointData;
			while(!done && !newTube){
				if(tmpPtr[0]==breakVal && tmpPtr[1]==breakVal && tmpPtr[2]==breakVal){
					newTube = true;
				}else{
					nPoints++;
				}
				tmpPtr += 3;
				if(tmpPtr>=pPointDataEnd) 
					done = true;
			}
			// Now that we know how many, we can build the points and line for this tube
			if(nPoints>1){
				//wxLogMessage(wxString::Format("Adding new tube with %d points", nPoints));

				pIdList = new vtkIdType[nPoints];
				//pPoints->SetNumberOfPoints(nPoints);
				for (i = 0; i < nPoints; i++){
					//pPoints->SetPoint(i, pPointData);
					// We use insert point here since we don't know the total number of 
					// points across all tubes. This is considerably slower than SetPoint 
					// (which requires pre-allocation via pPoints->SetNumberOfPoints(nPoints))
					// and thus we should consider adding code to compute the total number
					// of points ahead of time.
					pIdList[i] = pPoints->InsertNextPoint(pPointData);	
					pPointData += 3;
				}

				// build up the set of lines. We saved the subset of all the points that
				// make up this line (pIdList).
				//
				pLines->InsertNextCell(nPoints, pIdList);

				delete pIdList;
				
			} // end if(nPoints>1)
			// Skip the newTube marker at the end of this tube
			pPointData += 3;
			if(pPointData>=pPointDataEnd)
				done = true;
		} // end while(!done)

		//wxLogMessage(wxString::Format("Finished building lines."));

		// Build a polyData structure with the points and lines
		pPD->SetPoints(pPoints);
		pPD->SetLines(pLines);

		// We use vtkCleanPolyData to remove duplicate points since vtkTubeFilter 
		// will refuse to do anything if there are any duplicates.

		pPDcleaner = vtkCleanPolyData::New();
		pPDcleaner->SetInput(pPD);
		pPDcleaner->SetAbsoluteTolerance(0.05);
		pPDcleaner->ConvertLinesToPointsOn();

		// Now, wrap a tube around each line
		pTubeFilter->SetInput(pPDcleaner->GetOutput());
		pTubeFilter->SetNumberOfSides(nSides);
		pTubeFilter->SetRadius(fRadius);
		pTubeFilter->SetCapping(bCap);
		pTubeFilter->Update();
		pOut->ShallowCopy(pTubeFilter->GetOutput());

		//wxLogMessage(wxString::Format("Finished building tubes."));
		
		// <temporay> - return strips support to mesh
		vtkTriangleFilter *pTriFilter = vtkTriangleFilter::New();
		pTriFilter->SetInput(pOut);
		pTriFilter->Update();
		pOut->ShallowCopy(pTriFilter->GetOutput());
		pTriFilter->Delete();
		// <temporary>

		bResult = CreateFromVtkPolyData(pOut, paramsOut);

		// set color if needed
		if (pColor && pColor->GetNumberOfItems() == 4){
			double *fColorData = pColor->GetPointer();
			CColor cColor((unsigned char)fColorData[0], (unsigned char)fColorData[1], 
										(unsigned char)fColorData[2], (unsigned char)fColorData[3]);
			ResetColor(cColor);
		}
	}catch(...){
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Not enough memory"));
	}

	if(pOut) pOut->Delete();
	if(pPoints) pPoints->Delete();
	if(pLines) pLines->Delete();
	if(pPD) pPD->Delete();
	if(pPDcleaner) pPDcleaner->Delete();
	if(pTubeFilter) pTubeFilter->Delete();

	return bResult;
}

bool CMesh::CreateFromVtkPolyData(vtkPolyData *pPD, CParametersMap &paramsOut)
{
	Destroy();

	try
	{
		int iV;

		nVertices	= pPD->GetNumberOfPoints();
		nTriangles	= pPD->GetNumberOfPolys();

		if ((nVertices < 3) || (nTriangles < 1))
		{
			nVertices = 0;
			nTriangles = 0;
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("VTK mesh is not valid"));
			return false;
		}

		pVertices	= new Vertex[nVertices];
                pTriangles	= new Triangle_uint32[nTriangles];

		vtkDataArray	*pNormals	= pPD->GetPointData()->GetNormals();
		vtkDataArray	*pColors	= pPD->GetPointData()->GetScalars();

		if (!pNormals)
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("No normals in vtk data"));

		if (pColors && (pColors->GetNumberOfComponents() != 3))
			pColors = NULL;

		// transfer vertices
		for (iV = 0; iV < nVertices; iV++)
		{
			pPD->GetPoint((vtkIdType)iV, pVertices[iV].vCoord);
			if (pNormals)
				pNormals->GetTuple((vtkIdType)iV, pVertices[iV].vNormal);
			if (pColors)
			{
				pVertices[iV].cColor.m_ucColor[0] = (unsigned char)pColors->GetComponent(iV, 0);
				pVertices[iV].cColor.m_ucColor[1] = (unsigned char)pColors->GetComponent(iV, 1);
				pVertices[iV].cColor.m_ucColor[2] = (unsigned char)pColors->GetComponent(iV, 2);
			}
			else
				pVertices[iV].cColor = cDefaultColor;
		}

		// transfer triangles
		vtkIdType	*pPolys = pPD->GetPolys()->GetPointer();
		int	iTriangleIndex = 0;
		for (int iT = 0; iT < nTriangles; iT++)
		{
			vtkIdType nVerticesCount = *pPolys;
			if (nVerticesCount != 3)
			{
				nTriangles --;	//discard non-triangles
				// warning: memory allocated for such polys will remain allocated
			}
			else
			{
				pTriangles[iTriangleIndex].v[0] = pPolys[1];
				pTriangles[iTriangleIndex].v[1] = pPolys[2];
				pTriangles[iTriangleIndex].v[2] = pPolys[3];
				iTriangleIndex ++;
			}
			pPolys += nVerticesCount + 1;
		}

		if (pSceneInfo->bTransparentMeshEnabled)
			pSceneInfo->pPolyBuffer->RegisterObject(this);
		return true;
	}	
	catch(...)
	{
		if (pVertices)
			delete[] pVertices;
		if (pTriangles)
			delete[] pTriangles;
		nVertices = nTriangles = 0;

		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Not enough memory"));
		return false;
	}
}

vtkPolyData* CMesh::BuildVtkPolyDataTriangles()
{
	vtkPolyData		*pPD		= NULL;
	vtkPoints		*pPoints	= NULL;
	vtkCellArray	*pPolys		= NULL;
	vtkDoubleArray	*pNormals	= NULL;
	vtkUnsignedCharArray	*pColors = NULL;

	try
	{
		pPD			= vtkPolyData::New();
		pPoints		= vtkPoints::New();
		pPolys		= vtkCellArray::New();
		pNormals	= vtkDoubleArray::New();
		pColors		= vtkUnsignedCharArray::New();

		pPoints->SetDataType(VTK_DOUBLE);
		pPoints->SetNumberOfPoints(nVertices);
		
		pNormals->SetNumberOfComponents(3);
		pNormals->SetNumberOfTuples(nVertices);
		pNormals->Allocate(3*nVertices);

		pColors->SetNumberOfComponents(3);
		pColors->SetNumberOfTuples(nVertices);
		pColors->Allocate(3*nVertices);

		pPolys->EstimateSize(nTriangles, 3);

		for (int iV = 0; iV < nVertices; iV++)
		{
			pPoints->SetPoint(iV, pVertices[iV].vCoord);
			pNormals->SetTuple(iV, pVertices[iV].vNormal);

			for (int iC=0; iC<3; iC++)
				pColors->InsertComponent(iV, iC, pVertices[iV].cColor[iC]);
		}
		for (int iT = 0; iT < nTriangles; iT++)
		{
                            //RK 12/21/2009: convert pTriangles from Triangle_uint32 to Triangle
                            //because vtkCellArray::InsertNextCell accepts only vtkIdType!
                        Triangle dummy;
                        dummy.v[0] = pTriangles[iT].v[0];
                        dummy.v[1] = pTriangles[iT].v[1];
                        dummy.v[2] = pTriangles[iT].v[2];
                            //now insert 
			pPolys->InsertNextCell(3, dummy.v);
		}
		
		pPD->SetPoints(pPoints);
		pPD->GetPointData()->SetNormals(pNormals);
		pPD->GetPointData()->SetScalars(pColors);
		pPD->SetPolys(pPolys);

		return pPD;
	}
	catch(...)
	{
		if (pPD)
			pPD->Delete();
		if (pPoints)
			pPoints->Delete();
		if (pPolys)
			pPolys->Delete();
		if (pNormals)
			pNormals->Delete();
		if (pColors)
			pColors->Delete();
		return NULL;
	}
}

bool CMesh::CommandOpenGray(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	wxString	strFileName;
	CGrayVolume	gray;
	vtkPolyData	*pVTKMesh = NULL;
	wxString	pszErrorMsg;
	bool		bOK = false;

	if (!paramsIn.GetString(wxString::FromAscii("filename"), &strFileName))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("No filename specified"));
		return false;
	}

	Destroy();

	do{
		if (!gray.LoadGray(strFileName, paramsOut))
		{
			pszErrorMsg = wxString::FromAscii("Failed to load file");
			break;
		}

		pVTKMesh = gray.BuildMesh();
		if (!pVTKMesh)
		{
			pszErrorMsg = wxString::FromAscii("Failed to build vtk mesh");
			break;
		}

		// 2003.09.25 RFD: added pre-smoothing option (currently uses same params 
		// as post smoothing- may want to FIX THIS to allow separate params)
		if (paramsIn.GetInt(wxString::FromAscii("do_smooth_pre"), TRUE))
			if(!CVTKFilter::Smooth(pVTKMesh, paramsIn)) { wxASSERT(0); }
		if (paramsIn.GetInt(wxString::FromAscii("do_decimate"), TRUE))
			if(!CVTKFilter::DecimatePolyData(pVTKMesh, paramsIn)) { wxASSERT(0); }
		if (paramsIn.GetInt(wxString::FromAscii("do_smooth"), TRUE))
			if(!CVTKFilter::Smooth(pVTKMesh, paramsIn)) { wxASSERT(0); }
		if(!CVTKFilter::BuildNormals(pVTKMesh, paramsIn)) { wxASSERT(0); }
			 
		if (!CreateFromVtkPolyData(pVTKMesh, paramsOut))
			break;
		
		bOK = true;
	}while(0);

	if (pVTKMesh)
		pVTKMesh->Delete();

	if (!bOK)
		paramsOut.AppendString(wxString::FromAscii("error"), pszErrorMsg);

	return bOK;
}

bool CMesh::CommandOpenClass(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	wxString	strFileName;
	CGrayVolume	gray;
	vtkPolyData	*pVTKMesh = NULL;
	wxString		pszErrorMsg;
	bool		bOK = false;

	if (!paramsIn.GetString(wxString::FromAscii("filename"), &strFileName))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("No filename specified"));
		return false;
	}

	Destroy();

	do{
		if (!gray.LoadClass(strFileName, paramsIn, paramsOut))
		{
			pszErrorMsg = wxString::FromAscii("Failed to load file");
			break;
		}

		pVTKMesh = gray.BuildMesh();
		if (!pVTKMesh)
		{
			pszErrorMsg = wxString::FromAscii("Failed to build vtk mesh");
			break;
		}

		// 2003.09.25 RFD: added pre-smoothing option (currently uses same params 
		// as post smoothing- may want to FIX THIS to allow separate params)
		if (paramsIn.GetInt(wxString::FromAscii("do_smooth_pre"), TRUE))
			if(!CVTKFilter::Smooth(pVTKMesh, paramsIn)) { wxASSERT(0); }
		if (paramsIn.GetInt(wxString::FromAscii("do_decimate"), TRUE))
			if(!CVTKFilter::DecimatePolyData(pVTKMesh, paramsIn)) { wxASSERT(0); }
		if (paramsIn.GetInt(wxString::FromAscii("do_smooth"), TRUE))
			if(!CVTKFilter::Smooth(pVTKMesh, paramsIn)) { wxASSERT(0); }
		if(!CVTKFilter::BuildNormals(pVTKMesh, paramsIn)) { wxASSERT(0); }

		if (!CreateFromVtkPolyData(pVTKMesh, paramsOut))
			break;
		
		bOK = true;
	}while(0);

	if (pVTKMesh)
		pVTKMesh->Delete();

	if (!bOK)
		paramsOut.AppendString(wxString::FromAscii("error"), pszErrorMsg);

	return bOK;
}

bool CMesh::CommandGrid(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	Destroy();

	int	nPatches = 4;
	double	fPatchSize = 5;
	int nVerticesBySide = nPatches + 1;

	nVertices = nVerticesBySide * nVerticesBySide;
	nTriangles = nPatches * nPatches * 2;

	pVertices = new Vertex[nVertices];
        pTriangles = new Triangle_uint32[nTriangles];

	double fOffs = fPatchSize * nPatches / 2;

	int	i, j;

	for (i = 0; i < nVerticesBySide; i++)
	{
		for (j = 0; j < nVerticesBySide; j++)
		{
			pVertices[i*nVerticesBySide + j].vCoord.x = fPatchSize * j - fOffs;
			pVertices[i*nVerticesBySide + j].vCoord.y = fPatchSize * i - fOffs;
			pVertices[i*nVerticesBySide + j].vCoord.z = 0;

			pVertices[i*nVerticesBySide + j].vNormal = CVector(0, 0, 1);
			CColor vColor(160, 160, 160, 200);
			pVertices[i*nVerticesBySide + j].cColor = vColor;
		}
	}

	for (i = 0; i<nPatches; i++)
	{
		for (j=0; j<nPatches; j++)
		{
			pTriangles[(i*nPatches+j)*2].v[0] = i*nVerticesBySide + j;
			pTriangles[(i*nPatches+j)*2].v[1] = i*nVerticesBySide + j + 1;
			pTriangles[(i*nPatches+j)*2].v[2] = (i+1)*nVerticesBySide + j;

			pTriangles[(i*nPatches+j)*2+1].v[0] = (i+1)*nVerticesBySide + j;
			pTriangles[(i*nPatches+j)*2+1].v[1] = i*nVerticesBySide + j + 1;
			pTriangles[(i*nPatches+j)*2+1].v[2] = (i+1)*nVerticesBySide + j + 1;
		}
	}

	if (pSceneInfo->bTransparentMeshEnabled)
		pSceneInfo->pPolyBuffer->RegisterObject(this);
	
	return true;
}

bool CMesh::CommandPaint(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CDoubleArray *pColors = paramsIn.GetArray(wxString::FromAscii("colors"));
	CDoubleArray	*pCoords = paramsIn.GetArray(wxString::FromAscii("coords"));
	double		fMixFactor = paramsIn.GetFloat(wxString::FromAscii("mix_factor"), 1.0);

	Clamp(fMixFactor, 0.0, 1.0);
	
	if (!pColors || !pCoords)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("'coords' or 'colors' array was not specified"));
		return false;
	}

	bool bSingleColor = (pColors->GetNumberOfItems() == 4);
	bool bSizesMatch = (pColors->GetSizes()[1] == pCoords->GetSizes()[1]);

	if ((pCoords->GetNumberOfDimensions() != 2) || (pCoords->GetSizes()[0]!=3) ||
		(pColors->GetSizes()[0] !=4) || (!bSingleColor && !bSizesMatch))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Invalid arrays"));
		return false;
	}
	
	double	fColor[4];
	if (bSingleColor)
	{
		for (int iC = 0; iC < 4; iC++)
			pColors->GetAtAbsoluteIndex(&fColor[iC], iC);
	}

	int	iIndex;
	for (int iVertex = 0; iVertex < nVertices; iVertex++)
	{
		iIndex = FindPointInArray(pCoords, pVertices[iVertex].vCoord);
		if (iIndex != -1)
		{
			if (!bSingleColor)
			{
				for (int iC = 0; iC < 4; iC++)
					pColors->GetAtAbsoluteIndex(&fColor[iC], iC);
			}

			MixColors(pVertices[iVertex].cColor.m_ucColor, 
				fColor, pVertices[iVertex].cColor.m_ucColor,
				fMixFactor, 4);
		}
	}

	return true;
}

int	CMesh::FindPointInArray(CDoubleArray *pArray, double *pXYZ)
{
	wxASSERT(pArray->GetNumberOfDimensions()==2);
	wxASSERT(pArray->GetSizes()[0]==3);

	if (pArray->GetNumberOfDimensions()!=2)
		return -1;
	if (pArray->GetSizes()[0]!=3)
		return -1;

	double	fArrayX, fArrayY, fArrayZ;

	int	iMeshX = (int)pXYZ[0];
	int	iMeshY = (int)pXYZ[1];
	int	iMeshZ = (int)pXYZ[2];

	for (int i=0; i<pArray->GetSizes()[1]; i++)
	{
		pArray->GetValue(&fArrayX, 0, i);
		pArray->GetValue(&fArrayY, 1, i);
		pArray->GetValue(&fArrayZ, 2, i);

		if ((iMeshX == (int)fArrayX) && (iMeshY == (int)fArrayY) && (iMeshZ == (int)fArrayZ))
			return i;
	}
	return -1;
}

bool CMesh::CommandApplyROI(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CROI		roi;
	wxString	strFileName;

	if (!paramsIn.GetString(wxString::FromAscii("filename"), &strFileName))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("'filename' required"));
		return false;
	}

	if (roi.Load(strFileName))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Failed to load file"));
		return false;
	}
	
	const unsigned char	*color = roi.GetColor();
	
	double	fMixFactor = paramsIn.GetFloat(wxString::FromAscii("mix_factor"), 1.0);
	Clamp(fMixFactor, 0.0, 1.0);
	
	int iAlpha;
	bool bAlphaGiven = paramsIn.GetInt(wxString::FromAscii("alpha"), &iAlpha);

	for (int i=0; i<nVertices; i++)
	{
		Vertex	*pVertex = &pVertices[i];

		if (roi.PointInROI(pVertex->vCoord.x, pVertex->vCoord.y, pVertex->vCoord.z))
		{
			MixColors(pVertex->cColor.m_ucColor, color, pVertex->cColor.m_ucColor, fMixFactor, 3);
			if (bAlphaGiven)
				pVertex->cColor.a = iAlpha;
		}
	}

	return true;
}

void CMesh::ResetColor(CColor &cColor)
{
	for (int iV=0; iV<nVertices; iV++)
	{
		pVertices[iV].cColor = cColor;
	}
}

/*
void CMesh::MixColors(unsigned char *pDst, const unsigned char *pSrc1, const unsigned char *pSrc2, double fMixFactor, int nComponents)
{
	double fSourceFactor = 1.0f - fMixFactor;
	for (int i=0; i<nComponents; i++)
	{
		double fResult = fMixFactor * pSrc1[i] + fSourceFactor *pSrc2[i];
		Clamp(fResult, 0.0f, 255.0f);
		pDst[i] = (unsigned char)fResult;
	}
}
*/

template<class T1, class T2, class T3>
void CMesh::MixColors(T1 *pDst, const T2 *pSrc1, const T3 *pSrc2, double fMixFactor, int nComponents)
{
	double fSourceFactor = 1.0 - fMixFactor;
	for (int i=0; i<nComponents; i++)
	{
		double fResult = fMixFactor * pSrc1[i] + fSourceFactor *pSrc2[i];
		Clamp(fResult, 0.0, 255.0);
		pDst[i] = (T1)fResult;
	}
}


bool CMesh::CommandDecimate(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	vtkPolyData *pPD = BuildVtkPolyDataTriangles();
	if (!pPD)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Failed to convert to vtk mesh"));
		return false;
	}

	if (!CVTKFilter::DecimatePolyData(pPD, paramsIn))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Decimate failed"));
		return false;
	}
	if(!CVTKFilter::BuildNormals(pPD, paramsIn)) { wxASSERT(0); }

	Destroy();

	if (!CreateFromVtkPolyData(pPD, paramsOut))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Failed to convert vtk mesh"));
		return false;		
	}
	
	pPD->Delete();

	return true;
}

bool CMesh::CommandSmooth(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	vtkPolyData *pPD = BuildVtkPolyDataTriangles();
	if (!pPD)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Failed to convert to vtk mesh"));
		return false;
	}

	if (!CVTKFilter::Smooth(pPD, paramsIn))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Smooth failed"));
		return false;
	}
	if(!CVTKFilter::BuildNormals(pPD, paramsIn)) { wxASSERT(0); }

	Destroy();

	if (!CreateFromVtkPolyData(pPD, paramsOut))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Failed to convert vtk mesh"));
		return false;		
	}
	
	pPD->Delete();

	return true;
}

bool CMesh::GetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::GetProperties(paramsIn, paramsOut);

	CDoubleArray	*pArray = NULL;

	try{
		///////// vertices
		if (paramsIn.GetInt(wxString::FromAscii("get_vertices"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
		{
			if (nVertices)
			{
				pArray = new CDoubleArray;
				pArray->Create(2, 3, nVertices);
				double *pData = pArray->GetPointer();
				
				for (int iV = 0; iV < nVertices; iV++)
				{
					for (int iC = 0; iC < 3; iC++)
						pData[iC] = ((double*)pVertices[iV].vCoord)[iC];
					pData += 3;
				}
				paramsOut.SetArray(wxString::FromAscii("vertices"), pArray);
				pArray = NULL;
			}
			else
				paramsOut.AppendString(wxString::FromAscii("error"),
                                       wxString::FromAscii("Mesh is empty"));
		}

		///////// normals
		if (paramsIn.GetInt(wxString::FromAscii("get_normals"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
		{
			pArray = new CDoubleArray;
			pArray->Create(2, 3, nVertices);
			double *pData = pArray->GetPointer();
			
			for (int iV = 0; iV < nVertices; iV++)
			{
				for (int iC = 0; iC < 3; iC++)
					pData[iC] = ((double*)pVertices[iV].vNormal)[iC];
				pData += 3;
			}
			paramsOut.SetArray(wxString::FromAscii("normals"), pArray);
			pArray = NULL;
		}

		///////// colors
		if (paramsIn.GetInt(wxString::FromAscii("get_colors"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
		{
			pArray = new CDoubleArray;
			pArray->Create(2, 4, nVertices);
			double *pData = pArray->GetPointer();
			
			for (int iV = 0; iV < nVertices; iV++)
			{
				for (int iC = 0; iC < 4; iC++)
					pData[iC] = (double)pVertices[iV].cColor.m_ucColor[iC];
				pData += 4;
			}
			paramsOut.SetArray(wxString::FromAscii("colors"), pArray);
			pArray = NULL;
		}

		///////// triangles
		if (paramsIn.GetInt(wxString::FromAscii("get_triangles"), false) || paramsIn.GetInt(wxString::FromAscii("get_all"), false))
		{
			pArray = new CDoubleArray;
			pArray->Create(2, 3, nTriangles);
			double *pData = pArray->GetPointer();
			
			for (int iT = 0; iT < nTriangles; iT++)
			{
				for (int iC = 0; iC < 3; iC++)
					pData[iC] = pTriangles[iT].v[iC];
				pData += 3;
			}
			paramsOut.SetArray(wxString::FromAscii("triangles"), pArray);
			pArray = NULL;
		}
		return true;
	}
	catch(...)
	{
		if (pArray)
			delete pArray;
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Not enough memory"));
		return false;
	}
}

bool CMesh::SetProperties(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CActor::SetProperties(paramsIn, paramsOut);

	return true;
}

bool CMesh::CommandModifyMesh(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CDoubleArray	*pArrayVertices	= NULL;
	CDoubleArray *pArrayNormals	= NULL;
	CDoubleArray *pArrayColors	= NULL;

	pArrayVertices	= paramsIn.GetArray(wxString::FromAscii("vertices"));
	pArrayNormals	= paramsIn.GetArray(wxString::FromAscii("normals"));
	pArrayColors	= paramsIn.GetArray(wxString::FromAscii("colors"));

	if (!pArrayVertices && !pArrayNormals && !pArrayColors)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("No modification requested"));
		return false;
	}

	// check validity
	if ((pArrayVertices && !pArrayVertices->CheckSizes(2, 3, nVertices)) ||
		(pArrayNormals && !pArrayNormals->CheckSizes(2, 3, nVertices)) ||
		(pArrayColors && !pArrayColors->CheckSizes(2, 4, nVertices)))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("One or more of arrays have invalid size"));
		return false;
	}

	////////

	if (pArrayVertices)
	{
		double *pV = pArrayVertices->GetPointer();
		for (int iV = 0; iV < nVertices; iV++)
		{
			pVertices[iV].vCoord.x = pV[0];
			pVertices[iV].vCoord.y = pV[1];
			pVertices[iV].vCoord.z = pV[2];
			pV += 3;
		}
	}
	if (pArrayNormals)
	{
		double *pN = pArrayNormals->GetPointer();
		for (int iV = 0; iV < nVertices; iV++)
		{
			pVertices[iV].vNormal.x = pN[0];
			pVertices[iV].vNormal.y = pN[1];
			pVertices[iV].vNormal.z = pN[2];
			pN += 3;
		}
	}
	if (pArrayColors)
	{
		double *pC = pArrayColors->GetPointer();
		for (int iV = 0; iV < nVertices; iV++)
		{
			for (int iC = 0; iC < 4; iC++)
				pVertices[iV].cColor.m_ucColor[iC] = (unsigned char)pC[iC];
			pC += 4;
		}
	}

	if (pArrayVertices && !pArrayNormals)	// geometry has been changed
	{
		BuildNormalsViaVTK(paramsOut);
	}

	return true;
}

bool CMesh::BuildNormalsViaVTK(CParametersMap &paramsOut)
{
	vtkPolyData *pPD = BuildVtkPolyDataTriangles();
	if (!pPD)
	{
		paramsOut.SetString(wxString::FromAscii("warning"),
                            wxString::FromAscii("Cannot create VTK data to compute normals"));
		return false;
	}

	CParametersMap dummy;
	if (!CVTKFilter::BuildNormals(pPD, dummy))
	{
		paramsOut.SetString(wxString::FromAscii("warning"),
                            wxString::FromAscii("Failed to build normals"));
		pPD->Delete();
		return false;
	}
	
	vtkDataArray *pNormals = pPD->GetPointData()->GetNormals();
	
	wxASSERT(pNormals->GetNumberOfComponents() == 3);
	wxASSERT(pNormals->GetNumberOfTuples() == nVertices);

	if (pNormals->GetNumberOfTuples() != nVertices)
	{
		paramsOut.SetString(wxString::FromAscii("warning"),
                            wxString::FromAscii("Normals have not been created - invalid vtkDataArray size"));
		pPD->Delete();
		return false;
	}

	for (int i=0; i<nVertices; i++)
	{
		pNormals->GetTuple(i, pVertices[i].vNormal);
	}
	
	pPD->Delete();
	return true;
}

bool CMesh::CommandSetMesh(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CDoubleArray *pArrayVertices	= NULL;
	CDoubleArray *pArrayNormals	= NULL;
	CDoubleArray *pArrayColors	= NULL;
	CDoubleArray *pArrayTriangles= NULL;

	pArrayVertices	= paramsIn.GetArray(wxString::FromAscii("vertices"));
	pArrayNormals	= paramsIn.GetArray(wxString::FromAscii("normals"));
	pArrayColors	= paramsIn.GetArray(wxString::FromAscii("colors"));
	pArrayTriangles = paramsIn.GetArray(wxString::FromAscii("triangles"));

	// check validity

	if (!pArrayVertices || !pArrayTriangles)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Required parameter missing"));
		return false;
	}

	int	nNewVertices = pArrayVertices->GetSizes()[1];

	if (!pArrayVertices->CheckSizes(2, 3, nNewVertices) ||
		(pArrayNormals && !pArrayNormals->CheckSizes(2, 3, nNewVertices)) ||
		(pArrayColors && !pArrayColors->CheckSizes(2, 4, nNewVertices)) ||
		(pArrayTriangles->GetNumberOfDimensions() != 2) || (pArrayTriangles->GetSizes()[0] != 3))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("One or more arrays have invalid size"));
		return false;
	}

	Destroy();

	// parse sizes

	nVertices	= pArrayVertices->GetSizes()[1];
	nTriangles	= pArrayTriangles->GetSizes()[1];

	try{
		// allocate memory
		pVertices = new Vertex[nVertices];
                pTriangles = new Triangle_uint32[nTriangles];

		// move triangles

		double *pT = pArrayTriangles->GetPointer();
		for (int iT = 0; iT < nTriangles; iT++)
		{
			for (int iC = 0; iC < 3; iC++)
			{
				pTriangles[iT].v[iC] = int(pT[iC]+0.5f);
				wxASSERT(pTriangles[iT].v[iC] < nVertices);
			}
			
			pT += 3;
		}

		// move vertices
		double *pV = pArrayVertices->GetPointer();
		double *pN = (pArrayNormals) ? pArrayNormals->GetPointer(): NULL;
		double *pC = (pArrayColors) ? pArrayColors->GetPointer() : NULL;
		
		int iC;	// component counter

		for (int iV = 0; iV < nVertices; iV++)
		{
			for (iC = 0; iC < 3; iC++)
				((double*)(pVertices[iV].vCoord))[iC] = pV[iC];

			pV += 3;

			if (pN)
			{
				for (iC = 0; iC < 3; iC++)
					((double*)(pVertices[iV].vNormal))[iC] = pN[iC];
				pN += 3;
			}
			if (pC)
			{
				for (iC = 0; iC < 4; iC++)
					pVertices[iV].cColor.m_ucColor[iC] = (unsigned char)pC[iC];
				pC += 4;
			}
		}

		// postprocess
		if (!pArrayNormals)
		{
			//BuildMeshNormals();
			BuildNormalsViaVTK(paramsOut);
		}
//		FindBounds();

		if (pSceneInfo->bTransparentMeshEnabled)
			pSceneInfo->pPolyBuffer->RegisterObject(this);

		return true;
	}
	catch(...)
	{
		Destroy();

		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Memory error"));
		return false;
	}
}

bool CMesh::CommandBuildMesh(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CGrayVolume	gray;
	vtkPolyData *pVTKMesh;

	if (!gray.CreateFromArray(paramsIn, paramsOut))
	{
		/// error message is already set in CreateFromArray
		return false;
	}

	bool bRes = false;

	do{
		pVTKMesh = gray.BuildMesh();
		if (!pVTKMesh)
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("CGrayVolume::BuildMesh failed"));
			break;
		}

		Destroy();

		// 2003.09.25 RFD: added pre-smoothing option (currently uses same params 
		// as post smoothing- may want to FIX THIS to allow separate params)
		if (paramsIn.GetInt(wxString::FromAscii("do_smooth_pre"), TRUE))
			if (!CVTKFilter::Smooth(pVTKMesh, paramsIn)){ wxASSERT(0); }
		if (paramsIn.GetInt(wxString::FromAscii("do_decimate"), TRUE))
			if (!CVTKFilter::DecimatePolyData(pVTKMesh, paramsIn)){ wxASSERT(0); }
		if (paramsIn.GetInt(wxString::FromAscii("do_smooth"), TRUE))
			if (!CVTKFilter::Smooth(pVTKMesh, paramsIn)){ wxASSERT(0); }
			if (!CVTKFilter::BuildNormals(pVTKMesh, paramsIn)){ wxASSERT(0); }

		if (!CreateFromVtkPolyData(pVTKMesh, paramsOut))
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Cannot convert VTK data"));
			break;
		}
		
		bRes = true;
	}while (0);

	if (pVTKMesh)
		pVTKMesh->Delete();

	return bRes;
}

bool CMesh::CommandCurvatures(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	int		bModulateColor	= paramsIn.GetInt(wxString::FromAscii("modulate_color"), true);
	int		bGetValues		= paramsIn.GetInt(wxString::FromAscii("get_values"), false);
	double	fModDepth		= paramsIn.GetFloat(wxString::FromAscii("mod_depth"), 0.5);

	if (!bModulateColor && !bGetValues)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("No action requested"));
		return false;
	}

	vtkPolyData *pPD = BuildVtkPolyDataTriangles();
	if (!pPD){
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Cannot convert mesh to VTK data structures"));
		return false;
	}

	// this is necessary to reorient triangles
	if (!CVTKFilter::BuildNormals(pPD, paramsIn)) { wxASSERT(0); }

	CDoubleArray	*pCurvatures = NULL;
	pCurvatures = GetMrGrayCurvature(pPD);
	pPD->Delete();

	if (!pCurvatures){
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Error calculating curvatures"));
		return false;
	}

	wxASSERT(pCurvatures->GetSizes()[0] == nVertices);
	if (pCurvatures->GetSizes()[0] != nVertices){
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Invalid curvature values count"));
		delete pCurvatures;
		return false;
	}

	// note: curvature values computed for vertices in vtkPolyData
	// correspond ones in CMesh
	
	if (bModulateColor)
	{
		double *pCurvValues = pCurvatures->GetPointer();
		
		for (int iVertex=0; iVertex<nVertices; iVertex++)
		{
			double fFactor = (1-fModDepth) + (fModDepth*(pCurvValues[iVertex]+1.0)/2);
			for (int iComponent = 0; iComponent < 3; iComponent++)
			{
				//double fColor = 128.0 + 120.0*pCurvValues[iVertex];
				double fColor = double(pVertices[iVertex].cColor.m_ucColor[iComponent]);
				fColor *= fFactor;

				Clamp(fColor, 0.0, 255.0);
				pVertices[iVertex].cColor.m_ucColor[iComponent] = (unsigned char)(fColor);
			}
		} // </vertex traversal>
	}// </if (bModulateColor)>
	
	if (bGetValues)
		paramsOut.SetArray(wxString::FromAscii("values"), pCurvatures);
	else
		delete pCurvatures;

	return true;
}

CDoubleArray* CMesh::GetMrGrayCurvature(vtkPolyData* p)
{
	// Params: p is a vtkPolyData mesh
	// Should have ONLY TRIANGLES!
	// No other primitive is checked. Don't send in a stripped mesh.

	// Normals is an array of doubles that the normals will be written to.
	// Should have 3*n elements. (n is obtained from p)
	// Grays is an array of BYTEs that the colours will be written to.
	// n elements. 1 byte per vertex, 0-255 grayscale

	// This routine gray-codes each vertex according to the local curvature
	// Also calculates an area-weighted set of Normals

	// The "curvature" is approximated by the following formula:

	// C = R / (fabs(R) + sqrt(A)/4)

	// Where R is the signed distance between a point and the
	// average plane of its connected neighbours, and A is the
	// area of all the triangles featuring the point.

	// The plane is defined by the averages of all triangle normals
	// and centers (area weighted averages)

	int nVerticies = p->GetNumberOfPoints();
	//int nTriangles = p->GetNumberOfPolys();

	vtkDoubleArray	*pArrayNormals = NULL;

	double *Normals = NULL;
	CDoubleArray *pCurvatures = NULL;

	// Zero Normals and Colours if we have any.. note we'll need
	// a temp normal store even if they don't want it returned
/*	Normals = new double[nVerticies*3];
	memset(Normals, 0, sizeof(double)*nVerticies*3);
*/
	pArrayNormals = vtkDoubleArray::New();

	if (!pArrayNormals)
		return NULL;

	pArrayNormals->SetNumberOfComponents(3);
	pArrayNormals->SetNumberOfTuples(nVerticies);
	Normals = pArrayNormals->GetPointer(0);

	if (!Normals)
	{
		pArrayNormals->Delete();
		return NULL;
	}
	// 2003.09.24 RFD: trying this as a fix for sporatic linux curvature noise bug.
	memset(Normals, 0, sizeof(double)*nVerticies*3);

	pCurvatures = new CDoubleArray;

	if (!pCurvatures || !pCurvatures->Create(1, nVerticies))
	{
//		delete[] Normals;
		pArrayNormals->Delete();
		return NULL;
	}

	p->BuildLinks();

	double	fMax = 0.0,
			fSum = 0.0;

	for (int i=0;i<nVerticies;i++)
	{
		// For each vertex, calculate an average plane and normal

		double A=0;	// Total area of all triangles using this vertex
		double P[3]={0,0,0}; // Mean of centers of all triangles using this vertex

		// Find out which triangles use this vertex
		unsigned short nTris;
		vtkIdType *pp;
		p->GetPointCells(i,nTris,pp);
		for (int j=0;j<nTris;j++,pp++)
		{
			// This triangle uses this point. 
			vtkIdType nVerts;
			vtkIdType *Verts;
			p->GetCellPoints(*pp,nVerts,Verts);
			if (nVerts!=3)
				continue;

			double *p1=p->GetPoint(Verts[0]);
			double *p2=p->GetPoint(Verts[1]);
			double *p3=p->GetPoint(Verts[2]);

			// Get its normal and area at the same time
			double n[3],dA;
			CalculateNormal(p1,p2,p3,n,&dA);
			// Store area weighted normal
			Normals[i*3]+=n[0]*dA;
			Normals[i*3+1]+=n[1]*dA;
			Normals[i*3+2]+=n[2]*dA;
			// Add it's weighted center
			P[0]+= (p1[0]+p2[0]+p3[0])*dA;
			P[1]+= (p1[1]+p2[1]+p3[1])*dA;
			P[2]+= (p1[2]+p2[2]+p3[2])*dA;
			// Add the area to the running total
			A+=dA;
		} // Next triangle on this vertex

		// Average out all the normals
		if (A)
		{
			double RecipA = 1.0/A; 
			Normals[i*3]*=RecipA;
			Normals[i*3+1]*=RecipA;
			Normals[i*3+2]*=RecipA;
			RecipA*=1.0/3.0;
			P[0]*=RecipA;
			P[1]*=RecipA;
			P[2]*=RecipA;
		}

		// Find the signed distance to the average plane
		double *x=p->GetPoint(i);
		double *n=&(Normals[i*3]);
		double R = n[0]*(x[0]-P[0]) + n[1]*(x[1]-P[1]) + n[2]*(x[2]-P[2]);
		// Now encode the curvature
		// C = R / (fabs(R) + sqrt(A)/4)
		double C = R / (fabs(R) + sqrt(A)*0.25f);
/*
		// Convert to a byte value
		BYTE gray=(BYTE)((255.9*(C+1.0))*0.5);
		// Assign as the vertex colour
		if (Grays) Grays[i]=gray;
*/
		pCurvatures->SetAtAbsoluteIndex(C, i);

		fSum += fabs(C);
		if (fabs(C) > fMax)
			fMax = fabs(C);
	} // Next vertex

//	delete[] Normals; // Was temp in that case.
//	p->GetPointData()->SetNormals(pArrayNormals);
	pArrayNormals->Delete();

	wxLogDebug(wxString::Format(wxString::FromAscii(">> MrGray: Max = %f, Avg =%f\n"), fMax, fSum/nVerticies));

	return pCurvatures;
}

bool CMesh::CommandResetColor(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	CDoubleArray *pColor = paramsIn.GetArray(wxString::FromAscii("color"));
	
	if (!pColor || ((pColor->GetNumberOfItems()!=4) && (pColor->GetNumberOfItems()!=3)))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("'color[4]' or 'color[3]' required"));
		return false;
	}

	double *pfData = pColor->GetPointer();
	
	CColor cColor((unsigned char)pfData[0], (unsigned char)pfData[1], (unsigned char)pfData[2], 255);
	if (pColor->GetNumberOfItems() == 4)
		cColor.a = (unsigned char)pfData[3];

	ResetColor(cColor);

	return true;
}

void CMesh::Render()
{
	if (pSceneInfo->bTransparentMeshEnabled)
	{
		// do nothing for now
		// polygons from PolyDataBuffer will be rendered afterwards
		return;
	}

	if (!pVertices)	// no data to visualize
		return;

        glVertexPointer(3, GL_DOUBLE, sizeof(Vertex), &(pVertices[0].vCoord));
        glNormalPointer(GL_DOUBLE, sizeof(Vertex), &(pVertices[0].vNormal));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &(pVertices[0].cColor));

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        glDrawElements(GL_TRIANGLES, nTriangles*3, GL_UNSIGNED_INT, pTriangles);

	glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

bool CMesh::CommandOpenMRM(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	wxString	strFileName;
	MrM_Header	hdr;

	if (!paramsIn.GetString(wxString::FromAscii("filename"), &strFileName))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("'filename' required"));
		return false;
	}
	
	FILE *f = fopen(strFileName.ToAscii(), "rb");
	if (!f)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Cannot open file"));
		return false;
	}
	
	///// read file

	//bool	bResult = false;

	do{
		if (fread(&hdr, sizeof(MrM_Header), 1, f) < 1)
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Cannot read header"));
			break;
		}
		
		hdr.iFlags = wxINT32_SWAP_ON_BE(hdr.iFlags);
		hdr.nStrips = wxINT32_SWAP_ON_BE(hdr.nStrips);
		hdr.nTriangles = wxINT32_SWAP_ON_BE(hdr.nTriangles);

		if ((memcmp(hdr.Signature, "mr3DVisMesh", 11)!=0) || (memcmp(hdr.Signature, "mr3DMesh_v2", 11)!=0)
			|| !(hdr.nStrips+hdr.nTriangles))
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Invalid header"));
			break;
		}

	}while(0);

	fclose(f);

	wxASSERT(0);	//not done yet
	paramsOut.AppendString(wxString::FromAscii("error"),
                           wxString::FromAscii("Not implemented yet"));

	return false;
}

void CMesh::FindBounds(double fBounds[6]){
	if (!pVertices){
		wxASSERT(0);
		return;
	}

	fBounds[0] = pVertices[0].vCoord.x;
	fBounds[1] = pVertices[0].vCoord.x;
	fBounds[2] = pVertices[0].vCoord.y;
	fBounds[3] = pVertices[0].vCoord.y;
	fBounds[4] = pVertices[0].vCoord.z;
	fBounds[5] = pVertices[0].vCoord.z;

	for (int i=1; i<nVertices; i++){
		if (pVertices[i].vCoord.x < fBounds[0])
			fBounds[0] = pVertices[i].vCoord.x;
		if (pVertices[i].vCoord.x > fBounds[1])
			fBounds[1] = pVertices[i].vCoord.x;

		if (pVertices[i].vCoord.y < fBounds[2])
			fBounds[2] = pVertices[i].vCoord.y;
		if (pVertices[i].vCoord.y > fBounds[3])
			fBounds[3] = pVertices[i].vCoord.y;

		if (pVertices[i].vCoord.z < fBounds[4])
			fBounds[4] = pVertices[i].vCoord.z;
		if (pVertices[i].vCoord.z > fBounds[5])
			fBounds[5] = pVertices[i].vCoord.z;
	}
}

bool CMesh::CommandSaveMRM(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	if (!pVertices || !pTriangles)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("No data to save"));
		return false;
	}
	
	MrM_Header	hdr;
	wxString	strFileName;
	FILE		*f;

	if (!paramsIn.GetString(wxString::FromAscii("filename"), &strFileName))
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("filename not specified"));
		return false;
	}

	f = fopen(strFileName.ToAscii(), "wb");
	if (!f)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Cannot open file"));
		return false;
	}

	Vertex *pMRV = new Vertex[nTriangles*3];
	if (!pMRV)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Not enough memory"));
		fclose(f);
		return false;
	}

	memmove(hdr.Signature, "mr3DVisMesh", 11);
	hdr.iFlags = wxINT32_SWAP_ON_BE(MRMESH_COLOR_OK | MRMESH_NORMALS_OK);
	FindBounds(hdr.Bounds);
	hdr.nTriangles = wxINT32_SWAP_ON_BE(nTriangles);
	hdr.nStrips = 0;

	hdr.Offsets[0] = hdr.Offsets[1] = hdr.Offsets[2] = wxINT32_SWAP_ON_BE(sizeof(hdr));

	for (int iT=0, iV=0; iT<nTriangles; iT++, iV+=3)
	{
		pMRV[iV]	= pVertices[pTriangles[iT].v[0]];
		pMRV[iV+1]	= pVertices[pTriangles[iT].v[1]];
		pMRV[iV+2]	= pVertices[pTriangles[iT].v[2]];
	}

	bool bResult = false;
	do{
		if (fwrite(&hdr, sizeof(hdr), 1, f) < 1)
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Cannot write file"));
			break;
		}
		if (fwrite(pMRV, sizeof(Vertex)*nTriangles*3, 1, f) < 1)
		{
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Cannot write file"));
			break;
		}
		bResult = true;
	}while (0);

	fclose(f);
	delete[] pMRV;

	return bResult;
}

int	CMesh::FindClosestVertex(const CVector &vCoord)
{
	if (nVertices < 1)
		return -1;	//empty mesh

	int		iV = 0;
	double	fMinDistance = (vCoord - pVertices[0].vCoord).GetMagnitude();

	for (int i=1; i<nVertices; i++)
	{
		double fDistance = (vCoord - pVertices[i].vCoord).GetMagnitude();

		if (fDistance < fMinDistance)
		{
			fMinDistance = fDistance;
			iV = i;
		}
	}
	return iV;
}
