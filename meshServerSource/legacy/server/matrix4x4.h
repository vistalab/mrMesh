#ifndef _MATRIX4X4_H_
#define _MATRIX4X4_H_


class CMatrix4x4
{
public:
	double	fElements[16];

public:
	CMatrix4x4()
	{
		Identity();
	}
	CMatrix4x4(const CMatrix4x4 &m)
	{
		memmove(fElements, m.fElements, 16*sizeof(double));
	}
	CMatrix4x4(
		double a0, double a1, double a2, double a3,
		double a4, double a5, double a6, double a7,
		double a8, double a9, double a10, double a11,
		double a12, double a13, double a14, double a15
	)
	{
		fElements[0] = a0; fElements[1] = a1; fElements[2] = a2; fElements[3] = a3; 
		fElements[4] = a4; fElements[5] = a5; fElements[6] = a6; fElements[7] = a7;
		fElements[8] = a8; fElements[9] = a9; fElements[10] = a10; fElements[11] = a11;
		fElements[12] = a12; fElements[13] = a13; fElements[14] = a14; fElements[15] = a15;
	}
	CMatrix4x4(const double *fData)
	{
		memmove(fElements, fData, 16*sizeof(double));
	}

public:
	void Zero()
	{
		memset(fElements, 0, 16*sizeof(double));
	}
	void Identity()
	{
		memset(fElements, 0, 16*sizeof(double));
		fElements[0] = fElements[5] = fElements[10] = fElements[15] = 1.0f;
	}

	CMatrix4x4 operator *(CMatrix4x4 &m)
	{
		CMatrix4x4 out;

		double *a = fElements;
		const double *b = m.fElements;

		double *c = out.fElements;

		int	iOut = 0;
		for (int i=0; i<4; i++)
		{
			for (int j=0; j<4; j++)
			{
				c[iOut] = 0;
				for (int k=0; k<4; k++)
					c[iOut] += a[i*4+k] * b[k*4+j];

				iOut++;
			}
		}
		return out;
	}

	static CMatrix4x4 RotationX(double fAngle)
	{
		double fSinA = sin(fAngle);
		double fCosA = cos(fAngle);

		return CMatrix4x4(
			1, 0, 0, 0,
			0, fCosA, -fSinA, 0,
			0, fSinA, fCosA, 0,
			0, 0, 0, 1);
	}

	static CMatrix4x4 RotationY(double fAngle)
	{
		double fSinA = sin(fAngle);
		double fCosA = cos(fAngle);

		return CMatrix4x4(
			fCosA, 0, -fSinA, 0, 
			0, 1, 0, 0,
			fSinA, 0, fCosA, 0,
			0, 0, 0, 1);
	}

	static CMatrix4x4 RotationZ(double fAngle)
	{
		double fSinA = sin(fAngle);
		double fCosA = cos(fAngle);

		return CMatrix4x4(
			fCosA, -fSinA, 0, 0,
			fSinA, fCosA, 0, 0, 
			0, 0, 1, 0,
			0, 0, 0, 1);
	}

	void	Translate(double fX, double fY, double fZ)
	{
		fElements[12] += fX;
		fElements[13] += fY;
		fElements[14] += fZ;
	}
};

#endif //_MATRIX4X4_H_
