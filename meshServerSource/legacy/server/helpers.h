#ifndef _HELPERS_H_
#define _HELPERS_H_

/// Changes byte-order in the array
void SwizzleInts(int *list, int n);

void CalculateNormal(const double *p1, const double *p2, const double *p3, double *n, double *pTriangleArea=NULL);
void NormalizeVector(float *pVector3);

#ifdef WORDS_BIGENDIAN
	#define SWAP_BYTES_IN_ARRAY_ON_BE(list, n) SwizzleInts(list, n)
	#define SWAP_BYTES_IN_ARRAY_ON_LE(list, n) {}
#else
	#define SWAP_BYTES_IN_ARRAY_ON_LE(list, n) SwizzleInts(list, n)
	#define SWAP_BYTES_IN_ARRAY_ON_BE(list, n) {}
#endif

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

/*
#define FastInvSqrt(x)	(1.0f/sqrtf(x))
#define FastSqrt(x)		sqrtf(x)
#define FastInvSqrtLt(x) (1.0f/sqrtf(x))
#define FastSqrtLt(x)	sqrtf(x)
*/
typedef bool (*SortCallback)(int item1, int item2, void *pParam);
template<class T> void ShellSort(T a[], long size);
void ShellSortInt(int a[], long size, SortCallback compare_proc, void *pParam);

template<class T> void Clamp(T &x, T min, T max)
{
	if (x < min)
		x = min;
	else if (x > max)
		x = max;
}

void Dump(const char *pszPrefix, void *pData, int iLen);

bool SendInParts(wxSocketBase *sock, char *pData, int iLen, int iPacketSize = 1024*1024);
bool ReceiveInParts(wxSocketBase *sock, char *pBuf, int iLen, int iPacketSize = 1024*1024);
#endif //_HELPERS_H_
