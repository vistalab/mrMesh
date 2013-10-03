
/*

2009.12 RK: Modified to support unicode and ascii messages. Debugged the communication problems
            between Matlab and mrMeshSrv.
            The functions that communicate with the outside world (CreateFromString
            and FormatString) use uint8 ascii format, which is used for socket communication
            with Matlab. The internal operations within CParametersMap, however, use wxString
            format for highest compatibility with wxWidgets. undefining ASCII_STRING will make
            CParametersMap use uint8 unicode format in its communications with the ouside world.
            Undefine ASCII_STRING when Matlab is ready to receive unicode!
            If you need to modify the code avoid using sprintf and functions like it. See
            CParametersMap::MemCpy_CondConv and its use in the code.
*/


#include "stdafx.h"
#include "parametersmap.h"
#include "helpers.h"

const char* CParametersMap::pszFloatFormat = "%g";      //"%f" is another alternative

    /* for variable names and string data use ascii format. this is what Matlab expects for now.
       undefine ASCII_STRING when the Matlab side is changed to use unicode */
#define ASCII_STRING
    /* for array data use binary arrays to avoid the unicode/ascii headache */
#define BINARY_ARRAYS


CParametersMap::CParametersMap()
{
}

CParametersMap::~CParametersMap()
{
	CleanupBinaryParams();
	CleanupStringParams();
	CleanupArrayParams();
}

void CParametersMap::CleanupBinaryParams()
{
	HashMapBinaryParams::iterator it;

	for (it = mapBinary.begin(); it != mapBinary.end(); it++)
	{
		free(it->second);
	}
	mapBinary.clear();
}

void CParametersMap::CleanupArrayParams()
{
	HashMapArrayParams::iterator it;

	for (it = mapArrays.begin(); it != mapArrays.end(); it++)
	{
		delete it->second;
	}
	mapArrays.clear();
}

void CParametersMap::CleanupStringParams()
{
	mapStrings.clear();
}

bool CParametersMap::CreateFromString(char *pBuf, int iBufLen)
{
	wxASSERT(pBuf);

	if (!pBuf)
		return false;

	// variables to hold values during iteration
	int			iBinaryLen;
	wxString	strName;
	wxString	strValue;
	BinaryData	*pData = NULL;
	
	CDoubleArray	*pArray = NULL;
        int             nDimensions = 0;
        const int       nMaxDimensions = 3;
        int             iArraySizes[3];
        int             iArrayItem = 0;
        int             arraySize = 0;
	
	CleanupBinaryParams();
	CleanupStringParams();
	CleanupArrayParams();

	strName.Alloc(32);
	strValue.Alloc(128);

	states state = S_SKIP_TO_NAME;

	for (char *pCursor = pBuf; (state != S_FINISH) && (state != S_ERROR) && (pCursor - pBuf < iBufLen);)
	{
		switch (state)
		{
		case S_SKIP_TO_NAME:
			if (wxIsalpha(*pCursor) || (*pCursor == '_'))
			{
                                state = S_NAME;
                                break;
			}
			if (!*pCursor)		{state = S_FINISH; break;}
			pCursor++;
			break;
		
		case S_NAME:
			if (!*pCursor)		{state = S_ERROR; break;}
                        if (wxIsalnum(*pCursor) || (*pCursor == '_'))
			{
                                strName += *pCursor;
				pCursor++;
			}
			else
				state = S_SKIP_TO_DATA;
			break;
		
		case S_SKIP_TO_DATA:
			if (!*pCursor)		{state = S_ERROR; break;}
			
			else if (*pCursor == '=')
			{
				state = S_SKIP_TO_STRING;
				strValue = wxString::FromAscii("");
			}
			else if (*pCursor == ':')
			{
				state = S_BINARY_LEN;
				iBinaryLen = 0;
			}
			else if (*pCursor == '[')
			{
				state = S_ARRAY_SIZES;
				nDimensions = 0;
				strValue = wxString::FromAscii("");
			}
			pCursor++;
			break;
		
		case S_SKIP_TO_STRING:
			if (!*pCursor)			{state = S_ERROR; break;}
			if (*pCursor == '\'')	{state = S_QUOTED_STRING; pCursor++; break;}
                        if (wxIsalnum(*pCursor) || (*pCursor == '_'))	{state = S_STRING; break;}
			pCursor++;
			break;

		case S_STRING:
			if (!wxIsspace(*pCursor) && (*pCursor != 0))
			{
				strValue += *pCursor;
			}
			else
			{
				strName.MakeLower();
				mapStrings[strName] = strValue;
				state = (*pCursor) ? S_SKIP_TO_NAME : S_FINISH;
				strName = wxString::FromAscii("");
			}
			pCursor++;
			break;

		case S_QUOTED_STRING:
			if ((*pCursor != '\'') && (*pCursor != 0))
			{
                                strValue += *pCursor;
			}
			else
			{
				strName.MakeLower();
				mapStrings[strName] = strValue;

				state = (*pCursor) ? S_SKIP_TO_NAME : S_FINISH;
				strName = wxString::FromAscii("");
			}
			pCursor++;
			break;

		case S_BINARY_LEN:
			if (!*pCursor)		{state = S_ERROR; break;}

			strValue = wxString::FromAscii("");
                        while (!wxIsdigit(*pCursor))
			{
				pCursor++;
				if (!*pCursor){state = S_ERROR; break;}
			}
			while (wxIsdigit(*pCursor))
			{
                                strValue += *pCursor;
				pCursor++;
				if (!*pCursor){state = S_ERROR; break;}
			}

			if (wxSscanf(strValue, wxString::FromAscii("%d"), &iBinaryLen) < 1)
			{
				state = S_FINISH;
				break;
			}
			strValue = wxString::FromAscii("");

			if (!*pCursor)		{state = S_ERROR; break;}

			pCursor++;
			state = S_BINARY;
			break;

		case S_BINARY:
			if (!*pCursor)		{state = S_ERROR; break;}

			pData = (BinaryData *)malloc(sizeof(BinaryData)+iBinaryLen-1);
			
                        if (!pData)		{state = S_ERROR; break;}
			
			pData->len = iBinaryLen;
			memmove(pData->data, pCursor, iBinaryLen);
			
			strName.MakeLower();
			mapBinary[strName] = pData;

			strName = wxString::FromAscii("");

			pCursor += iBinaryLen;	//WARNING: if pCursor is not char *, but e.g. WORD *, some conversion operations are needed
			state = (*pCursor) ? S_SKIP_TO_NAME : S_FINISH;
			break;
		
		case S_ARRAY_SIZES:
			if (!*pCursor)		{state = S_ERROR; break;}
			
                        if (wxIsspace(*pCursor))
			{
				pCursor++;
				break;
			}
                        if (wxIsdigit(*pCursor))
			{
                                strValue += *pCursor;
			}
			else if (*pCursor == ',' || *pCursor == ']')
			{
				if (nDimensions < nMaxDimensions)
				{
					long lValue;
					strValue.ToLong(&lValue);
					iArraySizes[nDimensions] = lValue;
					nDimensions++;
					strValue = wxString::FromAscii("");
				}
				if (*pCursor == ']')
				{
#ifdef BINARY_ARRAYS
					state = S_SKIP_TO_BINARY_ARRAY;
#else
					state = S_SKIP_TO_ARRAY;
#endif
					
					pArray = new CDoubleArray;
					iArrayItem = 0;
					if (!pArray || 
						!(pArray->Create(nDimensions, iArraySizes[0], iArraySizes[1], iArraySizes[2])))
					{
						state = S_ERROR;
						if (pArray)
							delete pArray;
						break;
					}
					mapArrays[strName] = pArray;
					strName = wxString::FromAscii("");
				}
			}

			pCursor++;
			break;

		case S_SKIP_TO_ARRAY:
			if (!*pCursor) 		{state = S_ERROR; break;}

			if (IsCharNumeric(*pCursor))
			{
				state = S_ARRAY_ITEMS;
				break;
			}
			pCursor++;

			break;

		case S_SKIP_TO_BINARY_ARRAY:
			if (!*pCursor) 		{state = S_ERROR; break;}

			if (*pCursor == '\'')
			{
				state = S_ARRAY_BINARY_ITEMS;
			}
			pCursor++;

			break;

		case S_ARRAY_BINARY_ITEMS:
			arraySize = pArray->GetNumberOfItems ();
                        memcpy(pArray->GetPointer(), pCursor, arraySize*sizeof(double));
			pCursor += arraySize*sizeof(double)+1;
			strValue = wxString::FromAscii("");
			if (!*pCursor)
			{
				state = S_FINISH;
			}
			else
			{
				state = S_SKIP_TO_NAME;
			}
			break;

		case S_ARRAY_ITEMS:
			if (IsCharNumeric(*pCursor))
			{
                                strValue += *pCursor;
				pCursor++;
			}
			else
			{
				double fTemp;
				strValue.ToDouble(&fTemp);
				pArray->SetAtAbsoluteIndex((double)fTemp, iArrayItem);
				strValue = wxString::FromAscii("");
				iArrayItem++;

				if (!*pCursor)
				{
					state = S_FINISH;
					break;
				}
				else if (*pCursor != ';')
				{
					state = S_SKIP_TO_NAME;
				}
				
				pCursor++;
			}
			break;

		case S_FINISH:
			break;
		}
	}

	return (state != S_ERROR);
}


//copy the string from source to dest in a format that is good for socket communication
// convert the string to ascii format if needed
// RK 12/21/2009. Added this function.
int len(wxString &src)
{
        int nBytes;

#ifdef ASCII_STRING
const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(src);
const char *tmp_str = (const char *)tmp_buf;
nBytes = strlen(tmp_str);
#else
        nBytes = strlen(src.mb_str());
#endif
        return nBytes;
}

int CParametersMap::MemCpy_CondConv(char *dest, wxString &src)
{
        int nBytes;

        wxASSERT(dest);

#ifdef ASCII_STRING
const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(src);
const char *tmp_str = (const char *)tmp_buf;
nBytes = strlen(tmp_str);
if (nBytes>0 && dest!=NULL)   memmove(dest, tmp_str, nBytes);
//        nBytes = strlen(src.mb_str());
//        if (nBytes>0 && dest!=NULL)   memmove(dest, src.mb_str(), nBytes);
#else
        nBytes = strlen(src.mb_str());
        if (nBytes>0 && dest!=NULL)   memmove(dest, src.mb_str(), nBytes);
#endif
        return nBytes;
}


char* CParametersMap::FormatString(int *pLen)
{
	wxASSERT(pLen);

	*pLen = 0;

	wxString	strStringParams;
	FormatStringsMap(strStringParams);

	wxString	strArrayParams;

#ifndef BINARY_ARRAYS
	FormatArraysMap(strArrayParams);
#endif

	HashMapBinaryParams::iterator it2;

	int		iBufLen = 0;
	char	*pOutBuf = NULL;

	// estimate required buffer size
	for (it2 = mapBinary.begin(); it2 != mapBinary.end(); it2++)
	{
//		iBufLen += it2->first.Len() + 16 + it2->second->len;	//name : size data\n; size = max 10 chars + 3 (" : ") + 1 ('\n')
iBufLen += len(it2->first) + 16 + it2->second->len;	//name : size data\n; size = max 10 chars + 3 (" : ") + 1 ('\n')    //RK
        }
//	iBufLen += strStringParams.Len() + strArrayParams.Len() + 1; //str1 + str2 + '\0'
iBufLen += len(strStringParams) + len(strArrayParams) + 1; //str1 + str2 + '\0'     //RK

#ifdef BINARY_ARRAYS
	int	nBinaryBufLen = 0;
	char *pBinaryArraysBuf = NULL;

	FormatArraysMapBinary(&pBinaryArraysBuf, &nBinaryBufLen);
	iBufLen += nBinaryBufLen;
#endif

        if (iBufLen < 2)
		return NULL;

        // create & fill out buffer

	pOutBuf = new char[iBufLen];

	if (!pOutBuf)
		return NULL;

	int	iPos = 0;
        for (it2 = mapBinary.begin(); it2 != mapBinary.end(); it2++) {
                wxString temp = wxString::Format(wxString::FromAscii("%s : %d "),it2->first.GetData(),it2->second->len);
                iPos += MemCpy_CondConv(pOutBuf+iPos, temp);
                memmove(pOutBuf+iPos, it2->second->data, it2->second->len);
                iPos += it2->second->len;
	}

        iPos += MemCpy_CondConv(pOutBuf+iPos, strStringParams);
        iPos += MemCpy_CondConv(pOutBuf+iPos, strArrayParams);

#ifdef BINARY_ARRAYS
	if (nBinaryBufLen)
        {
                memmove(pOutBuf+iPos, pBinaryArraysBuf, nBinaryBufLen);
                iPos += nBinaryBufLen;
		delete[] pBinaryArraysBuf;
	}
#endif

	pOutBuf[iPos] = 0;

	*pLen = iPos+1;

	wxASSERT(iPos == iBufLen-1);

	return pOutBuf;

	//<test>
	//int		iLength = 30000000;
	//char	*pBuf	= new char[iLength];
	//
	//*pLen = iLength;

	//for (int i=0; i<iLength; i++)
	//{
	//	pBuf[i] = 'a';//rand()&0xFF;
	//}
	//return pBuf;
	//</test>
}

int	CParametersMap::GetInt(const wxString &strName, int iDefaultValue)
{
	HashMapStringParams::iterator it;

	it = mapStrings.find(strName);
	if (it != mapStrings.end())
	{
		long	iValue;
		if (it->second.ToLong(&iValue))
			return iValue;
		else
		{
			double temp;
			if (it->second.ToDouble(&temp))
				return int(temp+0.5);
		}
	}

	return iDefaultValue;
}

double CParametersMap::GetFloat(const wxString &strName, double fDefaultValue)
{
	HashMapStringParams::iterator it;

	it = mapStrings.find(strName);
	if (it != mapStrings.end())
	{
		double	fValue;
		if (it->second.ToDouble(&fValue))
			return (double)fValue;
	}

	return fDefaultValue;
}

bool CParametersMap::GetInt(const wxString &strName, int *pValue)
{
	HashMapStringParams::iterator it;

	it = mapStrings.find(strName);
	if (it == mapStrings.end())
		return false;

	long	lValue;
	if (it->second.ToLong(&lValue))
	{
		*pValue = lValue;
		return true;
	}

	double fValue;
	if (it->second.ToDouble(&fValue))
	{
		*pValue = int(fValue+0.5);
		return true;
	}

	return false;
}

bool CParametersMap::GetFloat(const wxString &strName, double *pValue)
{
	HashMapStringParams::iterator it;

	it = mapStrings.find(strName);
	if (it == mapStrings.end())
		return false;

	double	fValue;
	if (!it->second.ToDouble(&fValue))
		return false;

	*pValue = (double)fValue;

	return true;
}

bool CParametersMap::GetString(const wxString &strName, wxString *pstrValue)
{
	HashMapStringParams::iterator it;

	it = mapStrings.find(strName);
	if (it == mapStrings.end())
		return false;

	*pstrValue = it->second;

	return true;
}

wxString CParametersMap::GetString(const wxString &strName, const wxString &strDefault)
{
	HashMapStringParams::iterator it;

	it = mapStrings.find(strName);
	if (it != mapStrings.end())
	{
		return it->second;
	}

	return strDefault;
}

CDoubleArray* CParametersMap::GetArray(const wxString &strName)
{
	HashMapArrayParams::iterator it;

	it = mapArrays.find(strName);
	if (it != mapArrays.end())
	{
		return it->second;
	}

	return NULL;
}

void	CParametersMap::SetInt(const wxString &strName, int iValue)
{
	mapStrings[strName] = wxString::Format(wxString::FromAscii("%d"), iValue);
}

void	CParametersMap::SetFloat(const wxString &strName, double fValue)
{
	mapStrings[strName] = wxString::Format(wxString::FromAscii(pszFloatFormat), fValue);
}

void	CParametersMap::SetArray(const wxString &strName, CDoubleArray *pArray)
{
	// delete old array if exists
	HashMapArrayParams::iterator it;

	it = mapArrays.find(strName);
	if (it != mapArrays.end())
	{
		wxASSERT(it->second);
		if (it->second)
			delete it->second;
	}

	// set new array
	mapArrays[strName] = pArray;
}

void	CParametersMap::SetString(const wxString &strName, const wxString &strValue)
{
	mapStrings[strName] = strValue;
}

void CParametersMap::FormatArraysMap(wxString &strResult)
{
	HashMapArrayParams::iterator it;

	int		iStrLen = 0;

	for (it = mapArrays.begin(); it != mapArrays.end(); it++)
	{
		iStrLen += it->first.Len() + 20 + /*8*/12 * it->second->GetNumberOfItems();
	}

	strResult.Alloc(iStrLen);

	CDoubleArray	*pArray;
	int			i;

	for (it = mapArrays.begin(); it != mapArrays.end(); it++)
	{		
		pArray = it->second;
		strResult += it->first + wxString::FromAscii("[");

		for (i=0; i<pArray->GetNumberOfDimensions(); i++)
		{
			strResult += wxString::Format(wxString::FromAscii("%d"), pArray->GetSizes()[i]);
			if (i < pArray->GetNumberOfDimensions()-1)
				strResult += wxString::FromAscii(",");
			else
				strResult += wxString::FromAscii("] = ");
		}

		for (i=0; i<pArray->GetNumberOfItems(); i++)
		{
			double fValue;
			pArray->GetAtAbsoluteIndex(&fValue, i);

                        strResult += wxString::Format(wxString::FromAscii(pszFloatFormat), fValue);

			if (i < pArray->GetNumberOfItems()-1)
                                strResult += wxT(';');
			else
                                strResult += wxT('\n');
		}
	}
}

void CParametersMap::FormatArraysMapBinary(char **pResult, int *pLength)
{
	wxASSERT(pResult);
	wxASSERT(pLength);

	int	iBufLen = 0;

	HashMapArrayParams::iterator it;

	wxString strResult;
	CDoubleArray* pArray;

	for (it = mapArrays.begin(); it != mapArrays.end(); it++)
	{
		pArray = it->second;
		strResult = it->first + wxString::FromAscii("[");
                for (int i=0; i<pArray->GetNumberOfDimensions(); i++)
		{
			strResult += wxString::Format(wxString::FromAscii("%d"), pArray->GetSizes()[i]);
			if (i < pArray->GetNumberOfDimensions()-1)
				strResult += wxString::FromAscii(",");
			else
				strResult += wxString::FromAscii("] = ");
		}
//                iBufLen += strResult.Len() + sizeof(double) * it->second->GetNumberOfItems() + 1 /*'\n'*/ + 2 /*'\''*/;
iBufLen += len(strResult) + sizeof(double) * it->second->GetNumberOfItems() + 1 /*'\n'*/ + 2 /*'\''*/;  //RK
        }

	*pResult	= NULL;
	*pLength	= iBufLen;

	if (!iBufLen)
		return;

	//iBufLen += 1; //<- for trailing zero

        char* pBuf	= new char[iBufLen];        //RK 12/21/2009, must be deleted by the calling routine, not in this function
	if (!pBuf)
	{
		*pLength = 0;
		return;
	}
	*pResult = pBuf;

	int iPos = 0;

	for (it = mapArrays.begin(); it != mapArrays.end(); it++)
	{
                int nItems = it->second->GetNumberOfItems();

		pArray = it->second;
		strResult = it->first + wxString::FromAscii("[");
                for (int i=0; i<pArray->GetNumberOfDimensions(); i++)
		{
			strResult += wxString::Format(wxString::FromAscii("%d"), pArray->GetSizes()[i]);
			if (i < pArray->GetNumberOfDimensions()-1)
				strResult += wxString::FromAscii(",");
			else
				strResult += wxString::FromAscii("] = ");
		}

                iPos += MemCpy_CondConv(pBuf+iPos, strResult);

		pBuf[iPos++] = '\'';

		double *pArraySrc = it->second->GetPointer();
		double *pDst = (double*)(pBuf+iPos);
		
		for (int i=0; i<nItems; i++)
			pDst[i] = (double)pArraySrc[i];

		iPos += sizeof(double)*nItems;

		pBuf[iPos++] = '\'';
		pBuf[iPos++] = '\n';
	}
	
	wxASSERT(iPos == iBufLen);
}

void CParametersMap::FormatStringsMap(wxString &strResult)
{
        char format1[] = "%s = %s\n";   //RK 12/21/2009, added these constants to avoid the deprecated
        char format2[] = "%s = '%s'\n"; //               coversion from string constant to ‘char*’

	HashMapStringParams::iterator it;

        char *pszFormat;
	for (it = mapStrings.begin(); it != mapStrings.end(); it++)
	{
		// test whether to quote string or not
                pszFormat = format1;
		for (unsigned int iPos = 0; iPos < it->second.Len(); iPos++)
		{
			if (wxIsspace(it->second[iPos]))
			{
                                pszFormat = format2;
				break;
			}
		}

		strResult += wxString::Format(wxString::FromAscii(pszFormat), it->first.GetData(), it->second.GetData());
	}
}

bool CParametersMap::IsCharNumeric(char c)
{
	if (c >= '0' && c <='9')
		return true;

	static char num[]="+-.eE";
	char *pC = num;
	while (*pC)
	{
		if (*pC == c)
			return true;
		pC++;
	}
	return false;
}

void CParametersMap::AppendString(const wxString &strName, const wxString &strValue)
{
	wxString s = GetString(strName, wxString::FromAscii(""));
	if (!s.IsEmpty())
		s += wxString::FromAscii(", ");
	SetString(strName, s+strValue);
}
