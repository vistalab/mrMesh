#include "stdafx.h"
#include "roi.h"

long filesize(FILE *stream)
{
	long curpos, length;

	curpos = ftell(stream);
	fseek(stream, 0L, SEEK_END);
	length = ftell(stream);
	fseek(stream, curpos, SEEK_SET);
	return length;
}

CROI::CROI()
{
	bLoaded = false;
	pClassification = NULL;
}

CROI::~CROI()
{
	Free();
}


bool CROI::Load(const wxString &strFilename)
{
	Free();

	bool	bRes	= false;
	Voxel	*pVolume	= NULL;
	FILE	*f = fopen(strFilename.ToAscii(), "rb");
	
	
	if (!f)
		return false;

	do{
		int	iSize = filesize(f);
		if (iSize < 18)
			break;

		pVolume = new Voxel[iSize/8];
		if (!pVolume)
			break;

		if (fscanf(f, "%d", &pVolume[0].who_knows) < 1)
			break;
		if (fscanf(f, "%d %d %d", &pVolume[0].x, &pVolume[0].y, &pVolume[0].z) < 1)
			break;
		
		color[0] = pVolume[0].x;
		color[1] = pVolume[0].y;
		color[2] = pVolume[0].z;

		if (fscanf(f, "%d %d %d %d", &pVolume[0].x, &pVolume[0].y, &pVolume[0].z, &pVolume[0].who_knows) < 1)
			break;

		bounds[0] = bounds[1] = pVolume[0].x;
		bounds[2] = bounds[3] = pVolume[0].y;
		bounds[4] = bounds[5] = pVolume[0].z;

		nItems = 1;
		while (fscanf(f, "%d %d %d %d", &pVolume[nItems].x, &pVolume[nItems].y, &pVolume[nItems].z, &pVolume[nItems].who_knows) > 0)
		{
			if (pVolume[nItems].x < bounds[0])
				bounds[0] = pVolume[nItems].x;
			if (pVolume[nItems].x > bounds[1])
				bounds[1] = pVolume[nItems].x;
			
			if (pVolume[nItems].y < bounds[2])
				bounds[2] = pVolume[nItems].y;
			if (pVolume[nItems].y > bounds[3])
				bounds[3] = pVolume[nItems].y;

			if (pVolume[nItems].z < bounds[4])
				bounds[4] = pVolume[nItems].z;
			if (pVolume[nItems].z > bounds[5])
				bounds[5] = pVolume[nItems].z;

			nItems++;
		}
		size[0] = bounds[1] - bounds[0] + 1;
		size[1] = bounds[3] - bounds[2] + 1;
		size[2] = bounds[5] - bounds[4] + 1;
		// have read .roi into arrays and calculated min and max coords

		// allocate memory for bounding cube and fill ROI with 1's
		int	iClassSize = size[0]*size[1]*size[2];
		pClassification = new unsigned char[iClassSize];
		if (!pClassification)
			break;
		memset(pClassification, 0, iClassSize);

		for (int i = 0; i<nItems; i++)
		{
			pClassification[ClassIndex(pVolume[i].x, pVolume[i].y, pVolume[i].z)] = 1;
		}

		bRes = true;
		bLoaded = true;
	}while(0);
	
	fclose(f);
	if (pVolume)
		delete[] pVolume;
	return bRes;
}

void CROI::Free()
{
	if (!bLoaded)
		return;
	
	if (pClassification)
	{
		delete[] pClassification;
		pClassification = NULL;
	}
	bLoaded = false;
}

bool CROI::PointInROI(double x, double y, double z)
{
	wxASSERT(bLoaded);
	wxASSERT(pClassification);

	if ((x < bounds[0]) || (x > bounds[1]) ||
		(y < bounds[2]) || (y > bounds[3]) ||
		(z < bounds[4]) || (z > bounds[5]))
		return false;

	return	pClassification[ClassIndex((int)x, (int)y, (int)z)]!=0;
}
