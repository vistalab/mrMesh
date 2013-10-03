#include "stdafx.h"
#include "matrix3x3.h"

void MatrixAndVectorTo4x4(double *fMatrix4x4, const CMatrix3x3 &mRotation, const CVector &vOrigin)
{
	fMatrix4x4[3] = fMatrix4x4[7] = fMatrix4x4[11] = 0;
	fMatrix4x4[15] = 1;

	for (int iRow = 0; iRow < 3; iRow++)
		for (int iCol = 0; iCol < 3; iCol++)
			fMatrix4x4[iRow*4 + iCol] = mRotation.fElements[iRow*3 + iCol];

	fMatrix4x4[12] = vOrigin.x;
	fMatrix4x4[13] = vOrigin.y;
	fMatrix4x4[14] = vOrigin.z;
}

//CVector operator *(const CVector &v, const CMatrix3x3 &m)
//{
//	const double *pM = m.fElements;
//	return CVector(
//			v.x * pM[0] + v.y * pM[3] + v.z * pM[6],
//			v.x * pM[1] + v.y * pM[4] + v.z * pM[7],
//			v.x * pM[2] + v.y * pM[5] + v.z * pM[8]
//		);
//}

void Transfom3x3AndVectorBy3x3(double *pOut4x4, const CMatrix3x3 &mSrc, const CVector &vSrc, const CMatrix3x3 &mRotation)
{
	const double *fSrc	= mSrc.fElements;
	const double *fT	= mRotation.fElements;

	int	iDstIndex = 0;
	for (int iSrcRow = 0; iSrcRow < 3; iSrcRow++)
	{
		for (int iSrcCol = 0; iSrcCol < 3; iSrcCol++)
		{
			pOut4x4[iDstIndex] = 0;
			for (int iItem = 0; iItem < 3; iItem++)
			{
				pOut4x4[iDstIndex] += fSrc[iSrcRow*3 + iItem] * fT[iSrcCol + iItem*3];
			}
			iDstIndex++;
		}
		iDstIndex++;
	}

	for (int iCol = 0; iCol < 3; iCol++)
	{
		pOut4x4[12+iCol] = vSrc.x * fT[iCol] + vSrc.y * fT[iCol+3] + vSrc.z * fT[iCol+6];
	}
	//pOut4x4[12] = vSrc.x * fT[0] + vSrc.y * fT[3] + vSrc.z * fT[6];
	//pOut4x4[13] = vSrc.x * fT[1] + vSrc.y * fT[4] + vSrc.z * fT[7];
	//pOut4x4[14] = vSrc.x * fT[2] + vSrc.y * fT[5] + vSrc.z * fT[8];

	pOut4x4[3] = pOut4x4[7] = pOut4x4[11] = 0;
	pOut4x4[15] = 1;
}
