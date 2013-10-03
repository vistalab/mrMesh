#include "stdafx.h"
#include "grayVolume.h"
#include "helpers.h"
#include "progressindicator.h"

/* Classification. */
/* 
 * Internal classification tags. 
 * The classification tag is 8-bits wide.  The most significant
 * bit is use to determine selection.  The next upper 3 bits
 * are used to specify the class (white=1, gray=2, CSF=3, tmp=4).
 * The lower 4 bits are used to specify other information.
 * For example, the lower 4 bits for gray matter tags
 * could be used to describe up to 16 layers of gray.  The
 * lower 4 bits for white matter tags could be used to specify
 * tags for up to 16 white matter connected components.
 */
#define SELECT_MASK	0x80
#define CLASS_MASK	0x70

#define WHITE_CLASS	(1<<4)
#define GRAY_CLASS	(2<<4)
#define CSF_CLASS	(3<<4)
#define TMP_CLASS1	(4<<4)

#define	ISOLEVEL_VALUE	0.5f
#define INNER_VALUE		0
#define OUTER_VALUE		1

CGrayVolume::CGrayVolume()
{
	bLoaded = false;
	pClassData = NULL;
//	pEdges = NULL;
}

CGrayVolume::~CGrayVolume()
{
	Free();
}

bool CGrayVolume::LoadGray(const wxString &strFileName, CParametersMap &paramsOut)
{
	Free();

	FILE *f = fopen(strFileName.ToAscii(), "rb");
	if (!f)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Cannot open file"));
		return false;
	}

	GrayHeader	hdr;

	if (fread(&hdr, 5*sizeof(int), 1, f) < 1)
	{
		fclose(f);
		return false;
	}
	SWAP_BYTES_IN_ARRAY_ON_LE(&hdr.iSizeX, 5);

	vtkUnsignedCharArray *pClassValues = NULL;
	GraphNode	*pNodes = NULL;

	try{
		pNodes = new GraphNode[hdr.nNodes];
		//pEdges = new GraphEdge[nNodes];

		fread(pNodes, 6*sizeof(int)*hdr.nNodes, 1, f);
		SWAP_BYTES_IN_ARRAY_ON_LE((int *)pNodes, hdr.nNodes*6);

		pClassData = vtkStructuredPoints::New();

		pClassData->SetDimensions(hdr.iSizeX, hdr.iSizeY, hdr.iSizeZ);
		pClassData->SetOrigin(0, 0, 0);
		pClassData->SetSpacing(1, 1, 1);
		
		pClassValues = vtkUnsignedCharArray::New();

		pClassValues->SetNumberOfValues(hdr.iSizeX * hdr.iSizeY * hdr.iSizeZ);
		memset(pClassValues->GetPointer(0), OUTER_VALUE, hdr.iSizeX * hdr.iSizeY * hdr.iSizeZ);

		for (int i=0; i<hdr.nNodes; i++)
		{
			int iIndex =	pNodes[i].z * hdr.iSizeX * hdr.iSizeY +
							pNodes[i].y * hdr.iSizeX +
							pNodes[i].x;
			pClassValues->SetValue(iIndex, INNER_VALUE);
		}

		pClassData->GetPointData()->SetScalars(pClassValues);
		//pClassValues->Delete();	//ok to delete?

		fclose(f);
		delete[] pNodes;

		bLoaded = true;
		return true;
	}
	catch(...)
	{
		Free();
		fclose(f);
		if (pClassValues)
			pClassValues->Delete();

		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Not enough memory"));

		return false;
	}
}

bool CGrayVolume::LoadClass(const wxString &strFileName, CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	Free();

	FILE *f = fopen(strFileName.ToAscii(), "rb");
	if (!f)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Cannot open file"));
		return false;
	}

	vtkUnsignedCharArray	*pClassValues = NULL;

	char	*pBuffer = NULL;

	char	cMatterMask = WHITE_CLASS;

	wxString	strMatter;
	if (paramsIn.GetString(wxString::FromAscii("matter_class"), &strMatter))
	{
		if (strMatter.CmpNoCase(wxString::FromAscii("gray")) == 0)
			cMatterMask = GRAY_CLASS;
		else if (strMatter.CmpNoCase(wxString::FromAscii("csf")) == 0)
			cMatterMask = CSF_CLASS;
	}

	try{
		int iDataOffs;
		ClassHeader hdr;

		if (!ReadClassHeader(f, hdr, &iDataOffs)) 
		{
			fclose(f);
			paramsOut.AppendString(wxString::FromAscii("error"),
                                   wxString::FromAscii("Failed to read header"));
			return false;
		}
		fseek(f, iDataOffs, SEEK_SET);

		int	iVOISizeX = hdr.Bounds[1]-hdr.Bounds[0]+1 + 2;	//2 is for 1-voxel border on each side
		int	iVOISizeY = hdr.Bounds[3]-hdr.Bounds[2]+1 + 2;
		int	iVOISizeZ = hdr.Bounds[5]-hdr.Bounds[4]+1 + 2;
	
		pClassData = vtkStructuredPoints::New();
		pClassData->SetDimensions(iVOISizeX, iVOISizeY, iVOISizeZ);
		pClassData->SetOrigin(hdr.Bounds[0]-1, hdr.Bounds[2]-1, hdr.Bounds[4]-1);
		pClassData->SetSpacing(1, 1, 1);

		int	iTotalSize	= hdr.Size[0] * hdr.Size[1] * hdr.Size[2];

		pBuffer = new char[iTotalSize];

		pClassValues = vtkUnsignedCharArray::New();

		pClassValues->SetNumberOfValues(iVOISizeX * iVOISizeY * iVOISizeZ);
		memset(pClassValues->GetPointer(0), OUTER_VALUE, iVOISizeX * iVOISizeY * iVOISizeZ);

		if (fread(pBuffer, iTotalSize, 1, f) < 1) throw 0x17;
		fclose(f);

		// move voxel data from pBuffer to pClassValues
		// TODO: optimize this stuff below
		int	iSrcIndex;
		int iDstIndex;
		for (int iDstZ = 1; iDstZ < iVOISizeZ-1; iDstZ++)
		{
			for (int iDstY = 1; iDstY < iVOISizeY-1; iDstY++)
			{
				iDstIndex = (iDstZ * iVOISizeX * iVOISizeY) + (iDstY * iVOISizeX) + 1;
				iSrcIndex = (hdr.Bounds[4] + iDstZ-1) * hdr.Size[0] * hdr.Size[1] +
					(hdr.Bounds[2] + iDstY-1) * hdr.Size[0] +
					hdr.Bounds[0];

				for (int iDstX = 1; iDstX < iVOISizeX-1; iDstX++)
				{
					if ((pBuffer[iSrcIndex] & CLASS_MASK) == cMatterMask)
						pClassValues->SetValue(iDstIndex, INNER_VALUE);
					
					iDstIndex++;
					iSrcIndex++;
				}
			}
		}

		pClassData->GetPointData()->SetScalars(pClassValues);

		pClassValues->Delete();	//is this ok?
		delete[] pBuffer;

		bLoaded = true;
		return true;
	}
	catch(...)
	{
		if (pBuffer)
			delete[] pBuffer;
		if (pClassValues)
			pClassValues->Delete();

		fclose(f);

		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Not enough memory"));

		return false;
	}
}

bool CGrayVolume::CreateFromArray(CParametersMap &paramsIn, CParametersMap &paramsOut)
{
	Free();
	vtkUnsignedCharArray	*pClassValues = NULL;
	double					fTemp;

	CDoubleArray *pArray = paramsIn.GetArray(wxString::FromAscii("voxels"));
	if (!pArray)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("No array named 'voxels' supplied"));
		return false;
	}

	wxASSERT(pArray->GetNumberOfDimensions() == 3);

	if (pArray->GetNumberOfDimensions() != 3)
	{
		paramsOut.AppendString(wxString::FromAscii("error"),
                               wxString::FromAscii("Array must be in 3-dimensional"));
		return false;
	}

	CDoubleArray	*pScalesArray = paramsIn.GetArray(wxString::FromAscii("scale"));
	if (pScalesArray)
	{
		if (pScalesArray->GetNumberOfItems() != 3)
		{
			paramsOut.AppendString(wxString::FromAscii("warning"),
                                   wxString::FromAscii("'scale' dimensions are invalid - array ignored"));
			pScalesArray = NULL;
		}
	}
	
	try
	{
		pClassValues = vtkUnsignedCharArray::New();

		const int	*pDims = pArray->GetSizes();
		int			iSizes[3] = {pDims[0]+2, pDims[1]+2, pDims[2]+2};
		int			nTotalValues = iSizes[0] * iSizes[1] * iSizes[2];

		pClassValues->SetNumberOfValues(nTotalValues);
		memset(pClassValues->GetPointer(0), OUTER_VALUE, nTotalValues);

		pClassData = vtkStructuredPoints::New();
		pClassData->SetDimensions(iSizes[0], iSizes[1], iSizes[2]);

		if (!pScalesArray)
		{
			pClassData->SetOrigin(-1, -1, -1);
			pClassData->SetSpacing(1, 1, 1);
		}
		else
		{
			double *pCellSize = pScalesArray->GetPointer();
			pClassData->SetOrigin(-pCellSize[0], -pCellSize[1], -pCellSize[2]);
			pClassData->SetSpacing(pCellSize[0], pCellSize[1], pCellSize[2]);
		}
/*
		int iBaseOffset = 1 + iSizes[0] + iSizes[0]*iSizes[1];
		for (int i=0; i<pArray->GetNumberOfItems(); i++)
		{
			pArray->GetAtAbsoluteIndex(&fTemp, i);
			pClassValues->SetValue(i + iBaseOffset, (fTemp > 0) ? 0 : 2);
		}
*/
		int	iSrcIndex;
		int iDstIndex;

		for (int iSrcZ = 0; iSrcZ < pDims[2]; iSrcZ++)
		{
			for (int iSrcY = 0; iSrcY < pDims[1]; iSrcY++)
			{
				iSrcIndex = iSrcZ * pDims[1] * pDims[0] + iSrcY * pDims[0];
				iDstIndex = (iSrcZ+1) * iSizes[1] * iSizes[0] + (iSrcY+1) * iSizes[0] + 1;

				for (int iSrcX = 0; iSrcX < pDims[0]; iSrcX++)
				{
					pArray->GetAtAbsoluteIndex(&fTemp, iSrcIndex);
					pClassValues->SetValue(iDstIndex, (fTemp > 0) ? INNER_VALUE : OUTER_VALUE);

					iSrcIndex++;
					iDstIndex++;
				}
			}
		}

		pClassData->GetPointData()->SetScalars(pClassValues);

		pClassValues->Delete();

		bLoaded = true;
		
		return true;
	}
	catch(...)
	{
		if (pClassValues)
			pClassValues->Delete();
		paramsOut.SetString(wxString::FromAscii("error"),
                            wxString::FromAscii("Not enough memory"));
		return false;
	}
}

/// TODO: use CParametersMap to parse header here
bool CGrayVolume::ReadClassHeader(FILE *f, ClassHeader &hdr, int *piHeaderSize)
{
	wxASSERT(f);
	wxASSERT(piHeaderSize);

	char	*pBuf = new char[1024];

	if (!pBuf)
		return false;
	
	bool bRes = false;

	do{
		if (fread(pBuf, 1024, 1, f) < 1)
			break;

		int	iPos = 0;
		int	nProcessed;

		if (sscanf(pBuf, "version=%d%n", &hdr.iVerMajor, &nProcessed) < 1) break;
		iPos += nProcessed+1;

		if (sscanf(pBuf+iPos, "minor=%d%n", &hdr.iVerMinor, &nProcessed) < 1) break;
		iPos += nProcessed+1;

		if (sscanf(pBuf+iPos, "voi_xmin=%d%n", &hdr.Bounds[0], &nProcessed) < 1) break;
		iPos += nProcessed+1;

		if (sscanf(pBuf+iPos, "voi_xmax=%d%n", &hdr.Bounds[1], &nProcessed) < 1) break;
		iPos += nProcessed+1;

		if (sscanf(pBuf+iPos, "voi_ymin=%d%n", &hdr.Bounds[2], &nProcessed) < 1) break;
		iPos += nProcessed+1;

		if (sscanf(pBuf+iPos, "voi_ymax=%d%n", &hdr.Bounds[3], &nProcessed) < 1) break;
		iPos += nProcessed+1;

		if (sscanf(pBuf+iPos, "voi_zmin=%d%n", &hdr.Bounds[4], &nProcessed) < 1) break;
		iPos += nProcessed+1;

		if (sscanf(pBuf+iPos, "voi_zmax=%d%n", &hdr.Bounds[5], &nProcessed) < 1) break;
		iPos += nProcessed+1;

		if (sscanf(pBuf+iPos, "xsize=%d%n", &hdr.Size[0], &nProcessed) < 1) break;
		iPos += nProcessed+1;
		if (sscanf(pBuf+iPos, "ysize=%d%n", &hdr.Size[1], &nProcessed) < 1) break;
		iPos += nProcessed+1;
		if (sscanf(pBuf+iPos, "zsize=%d%n", &hdr.Size[2], &nProcessed) < 1) break;
		iPos += nProcessed+1;

		int		iTemp = 0;
		double	fTemp = 0;

		if (sscanf(pBuf+iPos, "csf_mean=%d%n", &iTemp, &nProcessed) < 1) break;
		iPos += nProcessed+1;
		hdr.csf = iTemp;

		if (sscanf(pBuf+iPos, "gray_mean=%d%n", &iTemp, &nProcessed) < 1) break;
		iPos += nProcessed+1;
		hdr.gray = iTemp;

		if (sscanf(pBuf+iPos, "white_mean=%d%n", &iTemp, &nProcessed) < 1) break;
		iPos += nProcessed+1;
		hdr.white = iTemp;

		if (sscanf(pBuf+iPos, "stdev=%d%n", &iTemp, &nProcessed) < 1) break;
		iPos += nProcessed+1;
		if (sscanf(pBuf+iPos, "confidence=%g%n", &fTemp, &nProcessed) < 1) break;
		iPos += nProcessed+1;
		if (sscanf(pBuf+iPos, "smoothness=%d%n", &iTemp, &nProcessed) < 1) break;
		iPos += nProcessed+1;

		*piHeaderSize = iPos;
		bRes = true;
	}while(0);

	delete[] pBuf;

	return bRes;
}

void CGrayVolume::Free()
{
	if (pClassData)
	{
		pClassData->Delete();
		pClassData = NULL;
	}
}


vtkPolyData* CGrayVolume::BuildMesh()
{
	if (!bLoaded)
		return NULL;

	wxASSERT(pClassData);

	vtkMarchingCubes		*pMC	= NULL;
	vtkPolyData				*pOut	= NULL;

	try{
		pMC = vtkMarchingCubes::New();
		pMC->SetInput(pClassData);
		pMC->SetValue(0, ISOLEVEL_VALUE);

		pMC->ComputeGradientsOff();
		pMC->ComputeNormalsOff();
		pMC->ComputeScalarsOff();

		CProgressIndicator *pPI = CProgressIndicator::GetInstance();
		void *pProgressTask = pPI->StartNewTask((vtkProcessObject*)pMC, wxString::FromAscii("Building mesh"));

		wxASSERT(pMC->GetNumberOfContours() == 1);

		pMC->Update();

		pPI->EndTask(pProgressTask);

		pOut = vtkPolyData::New();
		pOut->ShallowCopy(pMC->GetOutput());

		ReverseTriangles(pOut);

		pMC->Delete();

		return pOut;
	}
	catch(...)
	{
		if (pMC)
			pMC->Delete();
		if (pOut)
			pOut->Delete();
		return NULL;
	}
}

void CGrayVolume::ReverseTriangles(vtkPolyData *pPD)
{
	int nPolys = pPD->GetNumberOfPolys();
	
/*
	vtkCellArray *pVTKPolys = pPD->GetPolys();
	for (int iPoly = 0; iPoly < nPolys; iPoly++)
	{
		pVTKPolys->ReverseCell(iPoly);	//something wrong with ReverseCell
	}
*/
	vtkIdType *pPolys = pPD->GetPolys()->GetPointer();
	for (int iPoly = 0; iPoly < nPolys; iPoly++)
	{
		vtkIdType nVertices = *pPolys;
		if (nVertices == 3)
		{
			vtkIdType iLast = pPolys[3];
			pPolys[3] = pPolys[1];
			pPolys[1] = iLast;
		}
		pPolys += nVertices+1;
	}
}
