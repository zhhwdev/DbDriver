#include "StdAfx.h"
#include "PgDbRecordset.h"
#include <Winsock2.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void  array_reverse(byte* arr,int num)
{
	for(int i=0;i<num/2;i++)
	{
		char c=arr[i];
		arr[i]=arr[num-1-i];
		arr[num-1-i]=c;		
	}
}

#define POSTGRES_EPOCH_JDATE	2451545 /* == date2j(2000, 1, 1) */

static void PGj2date(int jd, int *year, int *month, int *day)
{
	unsigned int julian;
	unsigned int quad;
	unsigned int extra;
	int			y;

	julian = jd;
	julian += 32044;
	quad = julian / 146097;
	extra = (julian - quad * 146097) * 4 + 3;
	julian += 60 + quad * 3 + extra / 146097;
	quad = julian / 1461;
	julian -= quad * 1461;
	y = julian * 4 / 1461;
	julian = ((y != 0) ? ((julian + 305) % 365) : ((julian + 306) % 366))
		+ 123;
	y += quad * 4;
	*year = y - 4800;
	quad = julian * 2141 / 65536;
	*day = julian - 7834 * quad / 256;
	*month = (quad + 10) % 12 + 1;

	return;
}	/* j2date() */


#define USECS_PER_SEC	1000000
#define USECS_PER_MIN   ((__int64) 60 * USECS_PER_SEC)
#define USECS_PER_HOUR  ((__int64) 3600 * USECS_PER_SEC)
#define USECS_PER_DAY   ((__int64) 3600 * 24 * USECS_PER_SEC)

static void PGdt2timeInt8(__int64 jd, int *hour, int *min, int *sec, double *fsec)
{
	__int64		time;

	time = jd;

	*hour = (int) (time / USECS_PER_HOUR);
	time -= (__int64) (*hour) * USECS_PER_HOUR;
	*min = (int) (time / USECS_PER_MIN);
	time -=  (__int64) (*min) * USECS_PER_MIN;
	*sec = (int)time / USECS_PER_SEC;
	*fsec = (double)(time - *sec * USECS_PER_SEC);
}	/* dt2time() */

#define TMODULO(t,q,u) \
do { \
	(q) = ((t) / (u)); \
	if ((q) != 0) (t) -= ((q) * (u)); \
} while(0)

/* Coming from timestamp2tm() in pgsql/src/backend/utils/adt/timestamp.c */

static int PGTimeStamp2DMYHMS(__int64 dt, int *year, int *month, int *day,
							  int* hour, int* min, int* sec)
{
	__int64 date;
	__int64 time;
	double fsec;

	time = dt;
	TMODULO(time, date, USECS_PER_DAY);

	if (time < 0)
	{
		time += USECS_PER_DAY;
		date -= 1;
	}

	/* add offset to go from J2000 back to standard Julian date */
	date += POSTGRES_EPOCH_JDATE;

	/* Julian day routine does not work for negative Julian days */
	if (date < 0 || date > (double) INT_MAX)
		return -1;

	PGj2date((int) date, year, month, day);
	PGdt2timeInt8(time, hour, min, sec, &fsec);

	return 0;
}

typedef short NumericDigit;

typedef struct NumericVar
{
        int			ndigits;		/* # of digits in digits[] - can be 0! */
        int			weight;			/* weight of first digit */
        int			sign;			/* NUMERIC_POS, NUMERIC_NEG, or NUMERIC_NAN */
        int			dscale;			/* display scale */
        NumericDigit *digits;		/* base-NBASE digits */
} NumericVar;

#define NUMERIC_POS			0x0000
#define NUMERIC_NEG			0x4000
#define NUMERIC_NAN			0xC000

#define DEC_DIGITS	4
/*
* get_str_from_var() -
*
*	Convert a var to text representation (guts of numeric_out).
*	CAUTION: var's contents may be modified by rounding!
*	Returns a palloc'd string.
*/
static char * PGGetStrFromBinaryNumeric(NumericVar *var)
{
        char	   *str;
        char	   *cp;
        char	   *endcp;
        int			i;
        int			d;
        NumericDigit dig;
        NumericDigit d1;
        
        int dscale = var->dscale;

        /*
        * Allocate space for the result.
        *
        * i is set to to # of decimal digits before decimal point. dscale is the
        * # of decimal digits we will print after decimal point. We may generate
        * as many as DEC_DIGITS-1 excess digits at the end, and in addition we
        * need room for sign, decimal point, null terminator.
        */
        i = (var->weight + 1) * DEC_DIGITS;
        if (i <= 0)
                i = 1;

        str = new char[i + dscale + DEC_DIGITS + 2];
        cp = str;

        /*
        * Output a dash for negative values
        */
        if (var->sign == NUMERIC_NEG)
                *cp++ = '-';

        /*
        * Output all digits before the decimal point
        */
        if (var->weight < 0)
        {
                d = var->weight + 1;
                *cp++ = '0';
        }
        else
        {
                for (d = 0; d <= var->weight; d++)
                {
                        dig = (d < var->ndigits) ? var->digits[d] : 0;
                        SWAP16PTR(&dig);
                        /* In the first digit, suppress extra leading decimal zeroes */
                        {
                                bool		putit = (d > 0);

                                d1 = dig / 1000;
                                dig -= d1 * 1000;
                                putit |= (d1 > 0);
                                if (putit)
                                        *cp++ = (char)(d1 + '0');
                                d1 = dig / 100;
                                dig -= d1 * 100;
                                putit |= (d1 > 0);
                                if (putit)
                                        *cp++ = (char)(d1 + '0');
                                d1 = dig / 10;
                                dig -= d1 * 10;
                                putit |= (d1 > 0);
                                if (putit)
                                        *cp++ = (char)(d1 + '0');
                                *cp++ = (char)(dig + '0');
                        }
                }
        }
        
                /*
        * If requested, output a decimal point and all the digits that follow it.
        * We initially put out a multiple of DEC_DIGITS digits, then truncate if
        * needed.
        */
        if (dscale > 0)
        {
                *cp++ = '.';
                endcp = cp + dscale;
                for (i = 0; i < dscale; d++, i += DEC_DIGITS)
                {
                        dig = (d >= 0 && d < var->ndigits) ? var->digits[d] : 0;
                        SWAP16PTR(&dig);
                        d1 = dig / 1000;
                        dig -= d1 * 1000;
                        *cp++ = (char)(d1 + '0');
                        d1 = dig / 100;
                        dig -= d1 * 100;
                        *cp++ = (char)(d1 + '0');
                        d1 = dig / 10;
                        dig -= d1 * 10;
                        *cp++ = (char)(d1 + '0');
                        *cp++ = (char)(dig + '0');
                }
                cp = endcp;
        }

        /*
        * terminate the string and return it
        */
        *cp = '\0';
        return str;
}


CPgDbRecordset::CPgDbRecordset(PGresult* pRes)
{
	m_pResult = pRes;
	//memcpy_s(m_pResult, sizeof(m_pResult),pRes);
	m_nCurrentRow = 0;
}

CPgDbRecordset::~CPgDbRecordset(void)
{

}

string CPgDbRecordset::GetFieldName(long lIndex)
{
	ASSERT(m_pResult != NULL);

	return PQfname(m_pResult, lIndex);
}

int CPgDbRecordset::GetIndex(char* strFieldName)
{
	ASSERT(m_pResult!=NULL);

	return PQfnumber(m_pResult,strFieldName);
}

int CPgDbRecordset::GetFieldType(long lIndex)
{
	ASSERT(m_pResult!=NULL);
	int field_type=PQftype(m_pResult, lIndex);
	int rtn=FLD_UNKNOWN;
	switch(field_type)
	{
	case 16:	return FLD_BOOL;
	case 17:	return FLD_BLOB;
	case 18:	return FLD_CHAR;
	case 20:	return FLD_INT8;
	case 21:	return FLD_INT2;
	case 23:	return FLD_INT4;
	case 700:	return FLD_FLOAT;
	case 701:	return FLD_DOUBLE;
	case 1082:	return FLD_DATE;
	case 1083:	return FLD_TIME;
	case 1114:	return FLD_DATETIME;
	case 1700:	return FLD_NUMERIC;
	case 19:	//name
	case 25:
	case 1043:	return FLD_VARCHAR;
	}
	return rtn;
}

int CPgDbRecordset::GetFieldType(char* strFieldName)
{
	ASSERT(m_pResult != NULL);
	int nIndex = PQfnumber(m_pResult,strFieldName);
	return GetFieldType(nIndex);
}

int  CPgDbRecordset::GetFieldDefineSize(long lIndex)
{
	ASSERT(m_pResult != NULL);	
	int field_type=PQftype(m_pResult, lIndex);
	switch(field_type)
	{
	case 16:			return 1;
	case 17:			return -1;
	case 18:			return 1;
	case 21:			return 2;
	case 23:			return 4;
	case 20:			return 8;
	case 700:			return 4;
	case 701:			return 8;
	case 1043:			return ::PQfmod(m_pResult, lIndex)-4;
	case 1082:			break;
	case 1083:			break;
	case 1114:			break;
	case 1700:			break;
	default:			break;
	}		
	int size=PQfsize(m_pResult, lIndex);	
	return size;
}

int  CPgDbRecordset::GetFieldDefineSize(char* strFieldName)
{
	ASSERT(m_pResult != NULL);
	long lIndex = PQfnumber(m_pResult,strFieldName);
	return GetFieldDefineSize(lIndex); 
}

bool CPgDbRecordset::IsFieldNull(char* strFieldName)
{
	ASSERT(m_pResult != NULL);
	int nIndex = PQfnumber(m_pResult,strFieldName);

	if(nIndex>= 0 && PQgetisnull(m_pResult,m_nCurrentRow,nIndex) == 1)
		return TRUE;

	return FALSE;
}

bool CPgDbRecordset::IsFieldNull(int nIndex)
{
	ASSERT(m_pResult != NULL);

	if(PQgetisnull(m_pResult,m_nCurrentRow,nIndex) == 1)
		return TRUE;

	return FALSE;
}

bool CPgDbRecordset::IsNull()
{
	if(m_pResult == NULL)
		return true;

	return false;
}
bool CPgDbRecordset::IsEmpty()
{
	if(m_pResult!=NULL && (PQntuples(m_pResult) == 0))
		return true;

	return false;
}

bool CPgDbRecordset::IsNullEmpty()
{
	if (m_pResult==NULL || (PQntuples(m_pResult) == 0))
		return true;

	return false;
}

//判断执行是否成功
bool CPgDbRecordset::IsSuccess()
{
	ExecStatusType s=::PQresultStatus( m_pResult );
	return  (s==PGRES_COMMAND_OK || s==PGRES_TUPLES_OK);
}

void CPgDbRecordset::clear()
{
	PQclear(m_pResult);
}

int CPgDbRecordset::GetFieldNum()
{
	ASSERT(m_pResult!=NULL);

	return PQnfields(m_pResult);
}

int CPgDbRecordset::GetRowNum()
{
	ASSERT(m_pResult!=NULL);
	return PQntuples(m_pResult); 
}

bool CPgDbRecordset::IsEOF()
{
	ASSERT(m_pResult!=NULL);	
	return m_nCurrentRow == PQntuples(m_pResult);
}

bool CPgDbRecordset::IsBOF()
{
	return	m_nCurrentRow == 0;
}

void  CPgDbRecordset::MoveNext()
{
	m_nCurrentRow++;
}

void CPgDbRecordset::MoveFirst() 
{
	m_nCurrentRow = 0;
}

void CPgDbRecordset::MovePrevious() 
{
	m_nCurrentRow --;
}

void CPgDbRecordset::MoveLast() 
{
	ASSERT(m_pResult!=NULL);

	int nRowCount = PQntuples(m_pResult); 
	m_nCurrentRow = nRowCount - 1;
}

long CPgDbRecordset::GetCursorPos()
{
	return m_nCurrentRow;
}

long CPgDbRecordset::SetCursorPos(long i)
{
	if(m_pResult == NULL)
		return 0;
	m_nCurrentRow = i;
	return 1;
}


int CPgDbRecordset::GetValueSize(long lIndex)
{
	if (m_pResult == NULL)
		return -1;
	return PQgetlength(m_pResult, m_nCurrentRow, lIndex);
}

int CPgDbRecordset::GetValueSize(char* lpszFieldName)
{
	long lIndex = PQfnumber(m_pResult, lpszFieldName);
	return GetValueSize(lIndex);
}


bool CPgDbRecordset::GetValue(int nColumns, bool& bValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		bool* l  = (bool*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		bValue=  *l;		
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		if(strValue[0]=='t')
			bValue = true;
		else
			bValue = false;
	}
	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, bool& bValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, bValue, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, byte& nValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		byte* l  = (byte*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		nValue=  *l;	
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		nValue = atoi(strValue);
	}
	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, byte& nValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, nValue, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, short& nValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		//short* l  = (short*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		//nValue=  ntohs(*l);	
		 memcpy( &nValue, PQgetvalue( m_pResult,m_nCurrentRow,nColumns ), sizeof(short) );
         SWAP16PTR(&nValue);
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		nValue = atoi(strValue);
	}
	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, short& nValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, nValue, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, long& lValue, bool &nIsNull)
{
	//需要更改，与操作系统有关
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		//long* l  = (long*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		//lValue=  ntohl(*l);	
		memcpy( &lValue, PQgetvalue(m_pResult,m_nCurrentRow,nColumns), sizeof(long));
		SWAP32PTR(&lValue);
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		lValue = atol(strValue);
	}

	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, long& lValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, lValue, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, __int64& nValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		unsigned int nVal[2];
		memcpy( nVal, PQgetvalue(m_pResult,m_nCurrentRow,nColumns), 8 );
		SWAP32PTR(&nVal[0]);
		SWAP32PTR(&nVal[1]);
		nValue = (__int64) ((((unsigned __int64)nVal[0]) << 32) | nVal[1]);
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		nValue = atoi(strValue);
	}

	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, __int64& nValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, nValue, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, string& strValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	strValue = PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, string& strValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, strValue, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, int& nValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		//int* l  = (int*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		//nValue=  ntohl(*l);
		memcpy( &nValue, PQgetvalue(m_pResult,m_nCurrentRow,nColumns), sizeof(int));
		SWAP32PTR(&nValue);
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		nValue = atoi(strValue);
	}

	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, int& nValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, nValue, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, float& fValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		memcpy( &fValue, PQgetvalue(m_pResult,m_nCurrentRow,nColumns), sizeof(float) );
        SWAP32PTR(&fValue);	
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		fValue = (float)atof(strValue);
	}

	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, float& fValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, fValue, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, double& dValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		/*char val[9];
		memcpy(val,PQgetvalue(m_pResult,m_nCurrentRow,nColumns),8);
		array_reverse(val,8);
		memcpy(&dValue,val,8);*/
		int fld_type=GetFieldType(nColumns);
		if(fld_type==FLD_DOUBLE)
		{
			memcpy(&dValue, PQgetvalue(m_pResult,m_nCurrentRow,nColumns),8);
			SWAPDOUBLE(&dValue);
		}
		else if(fld_type==FLD_NUMERIC)
		{
			unsigned short sLen, sSign, sDscale;
			short sWeight;
			char* pabyData = PQgetvalue( m_pResult,m_nCurrentRow,nColumns );
			memcpy( &sLen, pabyData, sizeof(short));
			pabyData += sizeof(short);
			SWAP16PTR(&sLen);
			memcpy( &sWeight, pabyData, sizeof(short));
			pabyData += sizeof(short);
			SWAP16PTR(&sWeight);
			memcpy( &sSign, pabyData, sizeof(short));
			pabyData += sizeof(short);
			SWAP16PTR(&sSign);
			memcpy( &sDscale, pabyData, sizeof(short));
			pabyData += sizeof(short);
			SWAP16PTR(&sDscale);
		
			NumericVar var;
			var.ndigits = sLen;
			var.weight = sWeight;
			var.sign = sSign;
			var.dscale = sDscale;
			var.digits = (NumericDigit*)pabyData;
			char* str = PGGetStrFromBinaryNumeric(&var);
			dValue=atof(str);
			delete[] str;			
		}
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		dValue = (float)atof(strValue);
	}
	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, double& dValue, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, dValue, nIsNull);
}

long ScanLong( const char *pszString, int nMaxLength )
{
	long    iValue;
	char    *pszValue = new char[nMaxLength + 1];

	/* -------------------------------------------------------------------- */
	/*      Compute string into local buffer, and terminate it.             */
	/* -------------------------------------------------------------------- */
	strcpy_s( pszValue, nMaxLength,pszString);
	pszValue[nMaxLength] = '\0';

	/* -------------------------------------------------------------------- */
	/*      Use atol() to fetch out the result                              */
	/* -------------------------------------------------------------------- */
	iValue = atol( pszValue );

	delete []pszValue;
	return iValue;
}

bool ParseDateFromStr( const char *pszInput, tm &t)

{
	int bGotSomething = false;

	t.tm_year = 0;
	t.tm_mon = 0;
	t.tm_mday = 0;
	t.tm_hour = 0;
	t.tm_min = 0;
	t.tm_sec= 0;
	t.tm_isdst = 0;

	/* -------------------------------------------------------------------- */
	/*      Do we have a date?                                              */
	/* -------------------------------------------------------------------- */
	while( *pszInput == ' ' )
		pszInput++;

	if( strstr(pszInput,"-") != NULL || strstr(pszInput,"/") != NULL )
	{
		t.tm_year = atoi(pszInput);
		if( t.tm_year < 100 && t.tm_year >= 30 )
			t.tm_year += 1900;
		else if( t.tm_year < 30 && t.tm_year >= 0 )
			t.tm_year += 2000;

		while( *pszInput >= '0' && *pszInput <= '9' ) 
			pszInput++;
		if( *pszInput != '-' && *pszInput != '/' )
			return false;
		else 
			pszInput++;

		t.tm_mon = atoi(pszInput);
		if( t.tm_mon > 12 )
			return false;

		while( *pszInput >= '0' && *pszInput <= '9' ) 
			pszInput++;
		if( *pszInput != '-' && *pszInput != '/' )
			return false;
		else 
			pszInput++;

		t.tm_mday = atoi(pszInput);
		if( t.tm_mday > 31 )
			return false;

		while( *pszInput >= '0' && *pszInput <= '9' )
			pszInput++;

		bGotSomething = true;
	}

	/* -------------------------------------------------------------------- */
	/*      Do we have a time?                                              */
	/* -------------------------------------------------------------------- */
	while( *pszInput == ' ' )
		pszInput++;

	if( strstr(pszInput,":") != NULL )
	{
		t.tm_hour = atoi(pszInput);
		if( t.tm_hour > 23 )
			return false;

		while( *pszInput >= '0' && *pszInput <= '9' ) 
			pszInput++;
		if( *pszInput != ':' )
			return false;
		else 
			pszInput++;

		t.tm_min = atoi(pszInput);
		if( t.tm_min > 59 )
			return false;

		while( *pszInput >= '0' && *pszInput <= '9' ) 
			pszInput++;
		if( *pszInput != ':' )
			return false;
		else 
			pszInput++;

		t.tm_sec = atoi(pszInput);
		if( t.tm_sec > 59 )
			return false;

		while( (*pszInput >= '0' && *pszInput <= '9')
			|| *pszInput == '.' )
			pszInput++;

		bGotSomething = true;
	}

	// No date or time!
	if( !bGotSomething )
		return false;

	/* -------------------------------------------------------------------- */
	/*      Do we have a timezone?                                          */
	/* -------------------------------------------------------------------- */
	while( *pszInput == ' ' )
		pszInput++;

	if( *pszInput == '-' || *pszInput == '+' )
	{
		// +HH integral offset
		if( strlen(pszInput) <= 3 )
			t.tm_isdst = (100 + atoi(pszInput) * 4);

		else if( pszInput[3] == ':'  // +HH:MM offset
			&& atoi(pszInput+4) % 15 == 0 )
		{
			t.tm_isdst = (int)(100 
				+ atoi(pszInput+1) * 4
				+ (atoi(pszInput+4) / 15));

			if( pszInput[0] == '-' )
				t.tm_isdst = -1 * (t.tm_isdst - 100) + 100;
		}
		else if( isdigit(pszInput[3]) && isdigit(pszInput[4])  // +HHMM offset
			&& atoi(pszInput+3) % 15 == 0 )
		{
			t.tm_isdst = (int)(100 
				+ static_cast<int>(ScanLong(pszInput+1,2)) * 4
				+ (atoi(pszInput+3) / 15));

			if( pszInput[0] == '-' )
				t.tm_isdst = -1 * (t.tm_isdst - 100) + 100;
		}
		else if( isdigit(pszInput[3]) && pszInput[4] == '\0'  // +HMM offset
			&& atoi(pszInput+2) % 15 == 0 )
		{
			t.tm_isdst = (int)(100 
				+ static_cast<int>(ScanLong(pszInput+1,1)) * 4
				+ (atoi(pszInput+2) / 15));

			if( pszInput[0] == '-' )
				t.tm_isdst = -1 * (t.tm_isdst - 100) + 100;
		}
		// otherwise ignore any timezone info.
	}

	return true;
}
bool CPgDbRecordset::GetValue(int nColumns, tm& time, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;

	if(PQgetisnull(m_pResult,m_nCurrentRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	memset(&time,0,sizeof(tm));
	if(PQfformat( m_pResult, nColumns ) == 1)
	{
		short len = (int)PQgetlength(m_pResult, m_nCurrentRow, nColumns);
		int field_type =::PQftype(m_pResult,nColumns);

		int nVal, nYear, nMonth, nDay;
		int nHour, nMinute, nSecond;
		double dfsec;
		if(len==4 && field_type==1082) //日期
		{
			memcpy( &nVal, PQgetvalue( m_pResult, m_nCurrentRow, nColumns ), sizeof(int) );
            SWAP32PTR(&nVal);
            PGj2date(nVal + POSTGRES_EPOCH_JDATE, &nYear, &nMonth, &nDay);
			time.tm_year=nYear;
			time.tm_mon=nMonth;
			time.tm_mday=nDay;
		}
		else if(len==8 && field_type==1083)//一日内时间
		{
			unsigned int nVal[2];
			__int64 llVal;
			memcpy( nVal, PQgetvalue( m_pResult, m_nCurrentRow, nColumns  ), 8 );
			SWAP32PTR(&nVal[0]);
			SWAP32PTR(&nVal[1]);
			llVal = (__int64) ((((unsigned __int64)nVal[0]) << 32) | nVal[1]);
			PGdt2timeInt8(llVal, &nHour, &nMinute, &nSecond, &dfsec);
			time.tm_hour=nHour;
			time.tm_min=nMinute;
			time.tm_sec=nSecond;
			time.tm_isdst= (int)dfsec;
		}
		else if(len==8 && field_type==1114)//包括日期和时间
		{
			unsigned int nVal[2];
			__int64 llVal;
			memcpy( nVal, PQgetvalue( m_pResult, m_nCurrentRow, nColumns ), 8 );
			SWAP32PTR(&nVal[0]);
			SWAP32PTR(&nVal[1]);
			llVal = (__int64) ((((unsigned __int64)nVal[0]) << 32) | nVal[1]);
			if (PGTimeStamp2DMYHMS(llVal, &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond) == 0)
			{
				time.tm_year=nYear;
				time.tm_mon=nMonth;
				time.tm_mday=nDay;
				time.tm_hour=nHour;
				time.tm_min=nMinute;
				time.tm_sec=nSecond;
				time.tm_isdst=0;
			}
		}		
	}
	else
	{
		char* strValue  = PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
		if(!strValue)
			return false;
		ParseDateFromStr(strValue, time);
	}
	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, tm& time, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, time, nIsNull);
}


bool CPgDbRecordset::GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;
	if (PQgetisnull(m_pResult, m_nCurrentRow, nColumns))
		nIsNull = true;

	*bDat  = (byte*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
	*len = (long)PQgetlength(m_pResult, m_nCurrentRow, nColumns);

	return true;
}

bool CPgDbRecordset::GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	if(strFieldName == "")
		return false;
	int nIndex =PQfnumber(m_pResult,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, bDat, len, nIsNull);
}

bool CPgDbRecordset::GetValue(int nColumns, char* strValue, short len,bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	nIsNull = false;
	if (PQgetisnull(m_pResult, m_nCurrentRow, nColumns))
	{
		nIsNull = true;
		return true;
	}
	char* szData  = (char*)PQgetvalue(m_pResult,m_nCurrentRow,nColumns);
	short len0 = (int)PQgetlength(m_pResult, m_nCurrentRow, nColumns)+1;

	strcpy_s(strValue, min(len,len0), szData);
	return true;
}
bool CPgDbRecordset::GetValue(char* strFldName, char* strValue, short len, bool &nIsNull)
{
	ASSERT(m_pResult != NULL);
	if(strFldName == "")
		return false;
	int nIndex =PQfnumber(m_pResult,strFldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, strValue, len, nIsNull);
}
