#include "stdafx.h"
#include "helpers.h"

#ifndef min
	#define min(a,b) (a<b)?(a):(b)
#endif

void SwizzleInts(int *list, int n)
{
	unsigned long *p=(unsigned long *)list;
	while (n--)
	{
		unsigned long a=((*p)&0xFF000000)>>24;
		unsigned long b=((*p)&0x00FF0000)>>8;
		(*p)&=0x0000FFFF;
		(*p)|=(((*p)&0x0000FF00)<<8)|(((*p)&0x000000FF)<<24);
		(*p)&=0xFFFF0000;
		(*p)|=a|b;
		p++;
	}
}

void CalculateNormal(const double *p1, const double *p2, const double *p3, double *n, double *A/*=NULL*/)
{
	// Calculates the Normal to a triangle by calculating the
	// cross product of any two sides. The magnitude of this
	// cross product is twice the area of the triangle, and the
	// direction of the cross product is the normal to the triangle.
	// Can optionally store the area at the same time.
	double	v1[3],
			v2[3],
			len;
	enum {x=0,y=1,z=2};

	// Calculate vectors defining two of the sides
	v1[x] = p1[x] - p2[x];
	v1[y] = p1[y] - p2[y];
	v1[z] = p1[z] - p2[z];

	v2[x] = p2[x] - p3[x];
	v2[y] = p2[y] - p3[y];
	v2[z] = p2[z] - p3[z];

	// Take the cross product of the two vectors
	// This gives the normal to the triangle
	n[x] = v1[y]*v2[z] - v1[z]*v2[y];
	n[y] = v1[z]*v2[x] - v1[x]*v2[z];
	n[z] = v1[x]*v2[y] - v1[y]*v2[x];

	// Normalise the vector ( this gives twice the area)
	len = sqrtf( n[x]*n[x] + n[y]*n[y] + n[z]*n[z] );

	// Maybe store the area if requested
	if (A)
		*A=len/2;

	// Normalise the normal
	if (len!=0.0f)
	{
		double inv_len = 1.0f/len;
		n[x] *= inv_len;
		n[y] *= inv_len;
		n[z] *= inv_len;
	}
}

void NormalizeVector(double *pVector3)
{
	double fLen = sqrt(pVector3[0]*pVector3[0] + pVector3[1]*pVector3[1] + pVector3[2]+pVector3[2]);

	if (fLen == 0.0)
		return;

	double fInvLen = 1.0/fLen;

	pVector3[0] *= fInvLen;
	pVector3[1] *= fInvLen;
	pVector3[2] *= fInvLen;
}

int _ShellSortIncrement(long inc[], long size)
{
	int p1, p2, p3, s;

	p1 = p2 = p3 = 1;
	s = -1;
	do {
		if (++s % 2) 
		{
			inc[s] = 8*p1 - 6*p2 + 1;
		}
		else 
		{
			inc[s] = 9*p1 - 9*p3 + 1;
			p2 *= 2;
			p3 *= 2;
		}
		p1 *= 2;
	} while(3*inc[s] < size);  

	return s > 0 ? --s : 0;
}

template<class T>
void ShellSort(T a[], long size) {
	long inc, i, j, seq[40];
	int s;

	// calculating increments sequence
	s = _ShellSortIncrement(seq, size);
	while (s >= 0) {
		// sorting with 'inc[]' increments
		inc = seq[s--];

		for (i = inc; i < size; i++) {
			T temp = a[i];
			for (j = i-inc; (j >= 0) && (a[j] > temp); j -= inc)
				a[j+inc] = a[j];
			a[j+inc] = temp;
		}
	}
}

void ShellSortInt(int a[], long size, SortCallback compare_proc, void *pParam)
{
	long inc, i, j, seq[40];
	int s;

	// calculating increments sequence
	s = _ShellSortIncrement(seq, size);
	while (s >= 0) {
		// sorting with 'inc[]' increments
		inc = seq[s--];

		for (i = inc; i < size; i++) {
			int temp = a[i];
			for (j = i-inc; (j >= 0) && /*(a[j] > temp)*/compare_proc(a[j], temp, pParam); j -= inc)
				a[j+inc] = a[j];
			a[j+inc] = temp;
		}
	}
}

void Dump(const char *pszPrefix, void *pData, int iLen)
{
	static int	iLog	= 0;
	wxString	strFN	= wxString::Format(wxString::FromAscii("%s_%04d.txt"), pszPrefix, iLog);
	iLog++;

	FILE *f = fopen(strFN.ToAscii(), "wb");
	fwrite(pData, iLen, 1, f);
	fclose(f);
}

bool SendInParts(wxSocketBase *sock, char *pData, int iLen, int iPacketSize)
{
	do{
		sock->WriteMsg(pData, min(iLen, iPacketSize));
		if (sock->Error())
		{
			wxASSERT(0);			
//			Log(wxString::Format("Failed to send data (%d bytes)", min(iLen, iPacketSize)));
			return false;
		}
		iLen	-= iPacketSize;
		pData	+= iPacketSize;
	}while (iLen > 0);
	
	return true;
}

bool ReceiveInParts(wxSocketBase *sock, char *pBuf, int iLen, int iPacketSize)
{
	char *pData = pBuf;
	do{
		sock->ReadMsg(pData, min(iLen, iPacketSize));
		if (sock->Error())
		{
			wxASSERT(0);			
			return false;
		}
		iLen	-= iPacketSize;
		pData	+= iPacketSize;
	}while (iLen > 0);

	return true;
}
