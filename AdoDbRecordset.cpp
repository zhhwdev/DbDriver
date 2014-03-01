#include "StdAfx.h"
#include "AdoDbRecordset.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CString IntToStr(int nVal)
{
	CString strRet;
	char buff[10];

	_itoa_s(nVal, buff, 10);
	strRet = buff;
	return strRet;
}

CString LongToStr(long lVal)
{
	CString strRet;
	char buff[20];

	_ltoa_s(lVal, buff, 10);
	strRet = buff;
	return strRet;
}

CAdoRecordset::CAdoRecordset(_RecordsetPtr pRecordset)
{
	m_pRecordset = pRecordset;
	m_blobData=NULL;
}

CAdoRecordset::~CAdoRecordset(void)
{
	if(m_blobData)
		delete[] m_blobData;
	m_blobData=NULL;
}

// 字段名 
string CAdoRecordset::GetFieldName(long lIndex)
{
	ASSERT(m_pRecordset != NULL);
	string strFieldName;

	try
	{
		strFieldName = m_pRecordset->Fields->GetItem(_variant_t(lIndex))->GetName();
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());		strFieldName.empty();
		return strFieldName;
	}
	return strFieldName;
}

//暂时没有实现。。。。。。。
int CAdoRecordset::GetIndex(char* strFieldName)
{
	ASSERT(m_pRecordset != NULL);
	int  nIndex = -1;
	_variant_t vt;

	try
	{
		vt = m_pRecordset->Fields->GetItem(_variant_t(strFieldName));
		if (vt.vt != VT_NULL && vt.vt != VT_EMPTY)
			nIndex = vt.iVal;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
	}

	return nIndex;
}

// 字段数据类型 
int CAdoRecordset::GetFieldType(long lIndex)
{
	ASSERT(m_pRecordset != NULL);
	try 
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lIndex))->GetType();
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return adEmpty;
	}
}

int CAdoRecordset::GetFieldType(char* lpszFieldName)
{
	ASSERT(m_pRecordset != NULL);
	try 
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lpszFieldName))->GetType();
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return adEmpty;
	}	
}

// 字段定义长度
int  CAdoRecordset::GetFieldDefineSize(long lIndex)
{	
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lIndex))->DefinedSize;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return -1;
	}
}

int  CAdoRecordset::GetFieldDefineSize(char* lpszFieldName)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lpszFieldName))->DefinedSize;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return -1;
	}
}

//字段判断是否为NULL
bool CAdoRecordset::IsFieldNull(char* lpFieldName)
{
	_variant_t vtFld;
	vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
	return vtFld.vt == VT_NULL;
}

bool CAdoRecordset::IsFieldNull(int nIndex)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
	return vtFld.vt == VT_NULL;
}

bool CAdoRecordset::IsNull()
{
	return (m_pRecordset == NULL);
}

bool CAdoRecordset::IsEmpty()
{
	if (m_pRecordset!=NULL && GetRowNum()<1)
		return true;

	return false;
}

bool CAdoRecordset::IsNullEmpty()
{
	if (m_pRecordset == NULL || GetRowNum()<1)
		return true;

	return false;
}
//判断执行是否成功
bool CAdoRecordset::IsSuccess()
{
	return false;
}

void CAdoRecordset::clear()
{
	m_pRecordset->Close();
}


bool CAdoRecordset::IsEOF()
{
	return m_pRecordset->adoEOF == VARIANT_TRUE;	
}

bool CAdoRecordset::IsBOF()
{
	return m_pRecordset->BOF == VARIANT_TRUE;
}

void CAdoRecordset::MoveFirst() 
{
	ASSERT(m_pRecordset!=NULL);
	m_pRecordset->MoveFirst();
}

void CAdoRecordset::MoveNext()
{
	ASSERT(m_pRecordset!=NULL);
	m_pRecordset->MoveNext();
}

void CAdoRecordset::MovePrevious() 
{
	ASSERT(m_pRecordset!=NULL);
	m_pRecordset->MovePrevious();
}

void CAdoRecordset::MoveLast() 
{
	ASSERT(m_pRecordset!=NULL);
	m_pRecordset->MoveLast();
}

long CAdoRecordset::GetCursorPos()
{
	return m_pRecordset->GetAbsolutePosition();
}

long CAdoRecordset::SetCursorPos(long i)
{
	if (m_pRecordset == NULL)
		return 0;
	m_pRecordset->PutAbsolutePosition((enum PositionEnum)i);
	return 1;
}

int CAdoRecordset::GetFieldNum()
{
	ASSERT(m_pRecordset!=NULL);
	return m_pRecordset->Fields->GetCount();
}

int CAdoRecordset::GetRowNum()
{
	ASSERT(m_pRecordset!=NULL);

	int nRows = m_pRecordset->GetRecordCount();
	if(nRows == -1)
	{
		nRows = 0;
		if(m_pRecordset->adoEOF != VARIANT_TRUE)
			m_pRecordset->MoveFirst();

		while(m_pRecordset->adoEOF != VARIANT_TRUE)
		{
			nRows++;
			m_pRecordset->MoveNext();
		}
		if(nRows > 0)
			m_pRecordset->MoveFirst();
	}

	return nRows;
}

int CAdoRecordset::GetValueSize(long lIndex)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lIndex))->ActualSize;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return -1;
	}
}
int CAdoRecordset::GetValueSize(char* lpszFieldName)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lpszFieldName))->ActualSize;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return -1;
	}
}

bool CAdoRecordset::GetValue(int nColumns, _variant_t& vt, bool &nIsNull )
{
	_variant_t vtIndex;
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nColumns;
	nIsNull=false;

	try
	{
		vt = m_pRecordset->GetCollect(vtIndex);
	}
	catch (_com_error &e)
	{		
		TRACE("GetValue error %s", e.ErrorMessage());
		return false;
	}
	return true;
}
bool CAdoRecordset::GetValue(char* strFieldName, _variant_t& vt, bool &nIsNull )
{
	nIsNull=false;
	try
	{
		vt = m_pRecordset-> GetCollect(_variant_t(strFieldName));
	}
	catch (_com_error &e)
	{		
		TRACE("GetValue error %s", e.ErrorMessage());
		return false;
	}
	return true;
}

bool CAdoRecordset::GetValue(int nColumns, bool& bValue, bool &nIsNull )
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	bValue=vt.boolVal==1;
	return rtn;
}
bool CAdoRecordset::GetValue(char* strFieldName, bool& bValue, bool &nIsNull )
{
	_variant_t vt;
	bool rtn=GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	bValue=vt.boolVal==1;
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, byte& bValue, bool &nIsNull )
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	bValue=vt.bVal;
	return rtn;
}

bool CAdoRecordset::GetValue(char* strFieldName, byte& bValue, bool &nIsNull )
{
	_variant_t vt;
	bool rtn=GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	bValue=vt.bVal;
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, short& nValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	nValue=vt.iVal;
	return rtn;
}

bool CAdoRecordset::GetValue(char* strFieldName, short& nValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	nValue=vt.iVal;
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, int& nValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	nValue=vt.intVal;
	return rtn;
}

bool CAdoRecordset::GetValue(char* strFieldName, int& nValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	nValue=vt.intVal;
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, long& lValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}

	lValue=vt.lVal;
	return rtn;
}

bool CAdoRecordset::GetValue(char* strFieldName, long& lValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	lValue=vt.lVal;
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, __int64& lValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	lValue=vt.llVal;
	return rtn;
}

bool CAdoRecordset::GetValue(char* strFieldName, __int64& lValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	lValue=vt.llVal;
	return rtn;
}
bool CAdoRecordset::GetValue(int nColumns, float& fValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	fValue=vt.fltVal;
	return rtn;
}

bool CAdoRecordset::GetValue(char* strFieldName, float& fValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}

	fValue=vt.fltVal;
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, double& dValue, bool &nIsNull)
{
	_variant_t vt;
	double val = (double)NULL; 

	bool rtn=GetValue(nColumns,vt,nIsNull);

	switch(vt.vt)  
	{
	case VT_R4:   
		val = vt.fltVal;   
		break;   
	case VT_R8:   
		val = vt.dblVal;   
		break;   
	case VT_DECIMAL:    
		val = vt.decVal.Lo32;   
		val *= (vt.decVal.sign == 128)? -1 : 1;   
		val /= pow(10.0, vt.decVal.scale);    
		break;   
	case VT_UI1:   
		val = vt.iVal;   
		break;   
	case VT_I2:   
	case VT_I4:   
		val = vt.lVal;   
		break;   
	case VT_INT:   
		val = vt.intVal;   
		break;   
	case VT_CY:   //Added by John Andy Johnson!!!!   
		vt.ChangeType(VT_R8);   
		val = vt.dblVal;   
		break;   
	case VT_NULL:   
	case VT_EMPTY:   
		val = 0;   
		break;   
	default:   
		val = vt.dblVal;  
	}

	dValue = val;
	return rtn;
}

bool CAdoRecordset::GetValue(char* strFieldName, double& dValue, bool &nIsNull)
{
	_variant_t vt;
	double val(NULL);
	bool rtn=GetValue(strFieldName,vt,nIsNull);

	switch(vt.vt)  
	{
	case VT_R4:   
		val = vt.fltVal;   
		break;   
	case VT_R8:   
		val = vt.dblVal;   
		break;   
	case VT_DECIMAL:    
		val = vt.decVal.Lo32;   
		val *= (vt.decVal.sign == 128)? -1 : 1;   
		val /= pow(10.0, vt.decVal.scale);    
		break;   
	case VT_UI1:   
		val = vt.iVal;   
		break;   
	case VT_I2:   
	case VT_I4:   
		val = vt.lVal;   
		break;   
	case VT_INT:   
		val = vt.intVal;   
		break;   
	case VT_CY:   //Added by John Andy Johnson!!!!   
		vt.ChangeType(VT_R8);   
		val = vt.dblVal;   
		break;   
	case VT_NULL:   
	case VT_EMPTY:   
		val = 0;   
		break;   
	default:   
		val = vt.dblVal;  
	}

	dValue = val;
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, string& strValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	char *temp=_com_util::ConvertBSTRToString(vt.bstrVal);
	strValue=temp;
	return rtn;
}

bool CAdoRecordset::GetValue(char* strFieldName, string& strValue, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	char *temp=_com_util::ConvertBSTRToString(vt.bstrVal);
	strValue=temp;
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, char* strValue, short len,bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	char *temp=_com_util::ConvertBSTRToString(vt.bstrVal);
	lstrcpy(strValue,temp);
	return rtn;
}
bool CAdoRecordset::GetValue(char* strFldName, char* strValue, short len, bool &nIsNull)
{
	_variant_t vt;
	bool rtn=GetValue(strFldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}
	char *temp=_com_util::ConvertBSTRToString(vt.bstrVal);
	lstrcpy(strValue,temp);
	return rtn;
}

bool CAdoRecordset::GetValue(int nColumns, tm& time, bool &nIsNull)
{
	_variant_t vt;
	bool rtn = GetValue(nColumns,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}

	SYSTEMTIME dt;
	VariantTimeToSystemTime(vt.date, &dt);
	time.tm_year = dt.wYear;
	time.tm_mon = dt.wMonth;
	time.tm_mday = dt.wDay;
	time.tm_hour = dt.wHour;
	time.tm_min = dt.wMinute;
	time.tm_sec = dt.wSecond;
	time.tm_isdst = dt.wMilliseconds;
	return rtn;	
}

bool CAdoRecordset::GetValue(char* strFieldName, tm& time, bool &nIsNull)
{
	_variant_t vt;
	bool rtn= GetValue(strFieldName,vt,nIsNull);
	if(vt.vt == VT_NULL || vt.vt == VT_EMPTY)  
	{
		nIsNull = true;
		return true;
	}

	SYSTEMTIME dt;
	VariantTimeToSystemTime(vt.date, &dt);
	time.tm_year = dt.wYear;
	time.tm_mon = dt.wMonth;
	time.tm_mday = dt.wDay;
	time.tm_hour = dt.wHour;
	time.tm_min = dt.wMinute;
	time.tm_sec = dt.wSecond;
	time.tm_isdst = dt.wMilliseconds;
	return rtn;	
}


bool CAdoRecordset::GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull)
{
	try
	{
		if(m_pRecordset->adoEOF||m_pRecordset->BOF) 
			return false;

		_variant_t vtIndex;
		vtIndex.vt = VT_I2;
		vtIndex.iVal = nColumns;
		_variant_t vt;

		//得到数据的长度
		long lDataSize = m_pRecordset->Fields->GetItem(vtIndex)->ActualSize;
		vt = m_pRecordset->GetFields()->GetItem(vtIndex)->GetChunk(lDataSize);
		if(vt.vt == (VT_ARRAY | VT_UI1))
		{	
			if(m_blobData = new byte[lDataSize+1])				///重新分配必要的存储空间
			{	
				byte *pBuf = NULL;
				SafeArrayAccessData(vt.parray,(void **)&pBuf);
				memcpy(m_blobData,pBuf,lDataSize);				///复制数据到缓冲区g_pBinData
				SafeArrayUnaccessData (vt.parray);
				*len = lDataSize;
				*bDat = m_blobData;
			}
			else
			{
				*bDat = NULL;
				*len = 0;
			}
		}
		else
		{
			nIsNull=true;
			*bDat = NULL;
			*len = 0;
			return false;
		}
	}
	catch(_com_error &e)
	{
		TRACE("ado getvalue for binary error:%s", e.ErrorMessage());
		return false;
	}
	return true;
}

bool CAdoRecordset::GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull)
{
	try
	{
		_variant_t vt;
		//得到数据的长度
		long lDataSize = m_pRecordset->Fields->GetItem(_variant_t(strFieldName))->ActualSize;
		vt = m_pRecordset->GetFields()->GetItem(_variant_t(strFieldName))->GetChunk(lDataSize);
		if(vt.vt == (VT_ARRAY | VT_UI1))
		{	
			if(m_blobData = new byte[lDataSize+1])				///重新分配必要的存储空间
			{	
				byte *pBuf = NULL;
				SafeArrayAccessData(vt.parray,(void **)&pBuf);
				memcpy(m_blobData,pBuf,lDataSize);				///复制数据到缓冲区g_pBinData
				SafeArrayUnaccessData (vt.parray);
				*len = lDataSize;

				*bDat = m_blobData;
			}
			else
			{
				*bDat = NULL;
				*len = 0;
			}
		}
		else
		{
			nIsNull=true;
			*bDat = NULL;
			*len = 0;
			return false;
		}
	}
	catch(_com_error &e)
	{
		TRACE("ado getvalue for binary error:%s", e.ErrorMessage());
		return false;
	}
	return true;
}

