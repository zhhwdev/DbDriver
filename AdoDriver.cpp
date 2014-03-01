#include "StdAfx.h"
#include "AdoDriver.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CAdoSQLDriverPrivate
{
public:
	CAdoSQLDriverPrivate() 
	{
		::CoInitialize(NULL);
		m_pConn = NULL;
		HRESULT hr=m_pConn.CreateInstance(__uuidof(Connection));

		if(FAILED(hr))
			m_pConn=NULL;
	}
	~CAdoSQLDriverPrivate()
	{
		if(m_pConn)
		{
			m_pConn.Release();
			m_pConn = NULL;
		}
	}
	_ConnectionPtr m_pConn;
	_RecordsetPtr exec(const char * stmt) const;
};

_RecordsetPtr CAdoSQLDriverPrivate::exec(const char * stmt) const
{
	_RecordsetPtr   pRecord = NULL;
	_CommandPtr pCmd;

	if(m_pConn == NULL || m_pConn->GetState() == adStateClosed)
		return NULL;

	try
	{
		pCmd.CreateInstance("ADODB.Command");
		pCmd->ActiveConnection = m_pConn;
		pCmd->CommandText = (_bstr_t)stmt;
		pCmd->CommandType = adCmdText;

		pRecord = pCmd->Execute(NULL,NULL,adCmdText);

		return pRecord;
	}
	catch (_com_error &e)
	{
		throw e;
	}

	return pRecord;
}



class CAdoSQLResultPrivate
{
public:
	CAdoSQLResultPrivate(CAdoSQLResult *qq): q(qq), driver(NULL), result(NULL), currentSize(-1), preparedQueriesEnabled(false) {}
	
	CAdoSQLResult *q;
	const CAdoSQLDriverPrivate *driver;
	_RecordsetPtr result;
	int currentSize;
	bool preparedQueriesEnabled;
	string preparedStmtId;

	bool processResults();
};

bool CAdoSQLResultPrivate::processResults()
{
	if (!result)
		return false;

	q->setSelect(true);
	q->setActive(true);

	return true;
}

static void qDeallocatePreparedStmt(CAdoSQLResultPrivate *d)
{
	string strStmt = "DEALLOCATE " + d->preparedStmtId;
	_RecordsetPtr result = d->driver->exec(strStmt.c_str());

	if (result != NULL)
		TRACE("Unable to free ado statement ");

	result.Release();
	d->preparedStmtId.clear();
}

//////////////////////////////////////////////////////////////////////////
CAdoSQLResult::CAdoSQLResult(const CAdoSQLDriver* db, const CAdoSQLDriverPrivate* p)
: CSqlRecordset(db)
{
	d = new CAdoSQLResultPrivate(this);
	d->driver = p;
	m_blobData = NULL;
	//d->preparedQueriesEnabled = db->hasFeature(QSqlDriver::PreparedQueries);
}

CAdoSQLResult::~CAdoSQLResult()
{
	cleanup();

	if (d->preparedQueriesEnabled && !d->preparedStmtId.empty())
		qDeallocatePreparedStmt(d);

	if (d)
		delete d;
	d = NULL;

	if (m_blobData)
		delete []m_blobData;
	m_blobData = NULL;
}
void CAdoSQLResult::cleanup()
{
	if (d->result)
		d->result->Close();
	d->currentSize = -1;
	setActive(false);
}

bool CAdoSQLResult::fetch(int i)
{
	if (!isActive())
		return false;
	if (i < 0)
		return false;
	if (i >= d->currentSize)
		return false;
	// 	if (at() == i)
	// 		return true;
	//setAt(i);
	return true;
}

bool CAdoSQLResult::fetchFirst()
{
	return fetch(0);
}

bool CAdoSQLResult::fetchLast()
{
	return fetch(SelectRowCount() - 1);
}
bool CAdoSQLResult::IsFieldNull(int nIndex)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	vtFld = d->result->Fields->GetItem(vtIndex)->Value;
	return vtFld.vt == VT_NULL;
}

bool CAdoSQLResult::reset (const char* query, short execFormate)
{
	cleanup();
	if (!driver())
		return false;
	if (!(driver()->IsOpen()))
		return false;
	try
	{
		d->result = d->driver->exec(query);
	}
	catch (_com_error e)
	{
		setLastError(e.ErrorMessage());
	}

	return d->processResults();
}
int CAdoSQLResult::size()
{
	return d->currentSize;
}
int CAdoSQLResult::SelectRowCount()
{
	ASSERT(d->result!=NULL);

	int nRows = d->result->GetRecordCount();
	if(nRows == -1)
	{
		nRows = 0;
		if(d->result->adoEOF != VARIANT_TRUE)
			d->result->MoveFirst();

		while(d->result->adoEOF != VARIANT_TRUE)
		{
			nRows++;
			d->result->MoveNext();
		}
		if(nRows > 0)
			d->result->MoveFirst();
	}

	return nRows;
}
// 暂时未实现
long CAdoSQLResult::lastInsertId() const
{
	if (isActive()) {

	}
	return -1;
}

bool CAdoSQLResult::prepare(const string& query)
{
	if (!d->preparedQueriesEnabled)
		return CSqlRecordset::prepare(query);

	cleanup();

	return true;
}
bool CAdoSQLResult::exec()
{
	if (!d->preparedQueriesEnabled)
		return CSqlRecordset::exec();

	cleanup();
	return d->processResults();
}

// 字段名 
string CAdoSQLResult::GetFieldName(long lIndex)
{
	ASSERT(d->result != NULL);
	string strFieldName;

	try
	{
		strFieldName = d->result->Fields->GetItem(_variant_t(lIndex))->GetName();
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());		strFieldName.empty();
		return strFieldName;
	}
	return strFieldName;
}

//暂时没有实现。。。。。。。
int CAdoSQLResult::GetIndex(const char* strFieldName)
{
	ASSERT(d->result != NULL);
	int  nIndex = -1;
	_variant_t vt;

	try
	{
		vt = d->result->Fields->GetItem(_variant_t(strFieldName));
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
int CAdoSQLResult::GetFieldType(int lIndex)
{
	ASSERT(d->result != NULL);
	try 
	{
		return d->result->Fields->GetItem(_variant_t((long)lIndex))->GetType();
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return adEmpty;
	}
}

int CAdoSQLResult::GetFieldType(const char* lpszFieldName)
{
	ASSERT(d->result != NULL);
	try 
	{
		return d->result->Fields->GetItem(_variant_t(lpszFieldName))->GetType();
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return adEmpty;
	}	
}

// 字段定义长度
int  CAdoSQLResult::GetFieldDefineSize(int lIndex)
{	
	ASSERT(d->result != NULL);
	try
	{
		return d->result->Fields->GetItem(_variant_t((long)lIndex))->DefinedSize;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return -1;
	}
}

int  CAdoSQLResult::GetFieldDefineSize(const char* lpszFieldName)
{
	ASSERT(d->result != NULL);
	try
	{
		return d->result->Fields->GetItem(_variant_t(lpszFieldName))->DefinedSize;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return -1;
	}
}

// 字段判断是否为NULL
// bool CAdoSQLResult::IsFieldNull(char* lpFieldName)
// {
// 	_variant_t vtFld;
// 	vtFld = d->result->Fields->GetItem(lpFieldName)->Value;
// 	return vtFld.vt == VT_NULL;
// }
// 
// bool CAdoSQLResult::IsFieldNull(int nIndex)
// {
// 	_variant_t vtFld;
// 	_variant_t vtIndex;
// 
// 	vtIndex.vt = VT_I2;
// 	vtIndex.iVal = nIndex;
// 
// 	vtFld = d->result->Fields->GetItem(vtIndex)->Value;
// 	return vtFld.vt == VT_NULL;
// }


// bool CAdoSQLResult::IsEOF()
// {
// 	return d->result->adoEOF == VARIANT_TRUE;	
// }
// 
// bool CAdoSQLResult::IsBOF()
// {
// 	return d->result->BOF == VARIANT_TRUE;
// }

void CAdoSQLResult::MoveFirst() 
{
	ASSERT(d->result!=NULL);
	d->result->MoveFirst();
}

void CAdoSQLResult::MoveNext()
{
	ASSERT(d->result!=NULL);
	d->result->MoveNext();
}

void CAdoSQLResult::MovePrevious() 
{
	ASSERT(d->result!=NULL);
	d->result->MovePrevious();
}

void CAdoSQLResult::MoveLast() 
{
	ASSERT(d->result!=NULL);
	d->result->MoveLast();
}
bool CAdoSQLResult::IsEOF()
{
	if (d->result == NULL)
		return true;

	return d->result->adoEOF == VARIANT_TRUE;	
}

bool CAdoSQLResult::IsBOF()
{
	if (d->result == NULL)
		return true;

	return d->result->BOF == VARIANT_TRUE;	
}

bool CAdoSQLResult::IsNull()
{
	return (d->result == NULL);
}

bool CAdoSQLResult::IsEmpty()
{
	if (d->result!=NULL && SelectRowCount()<1)
		return true;

	return false;
}

int CAdoSQLResult::GetFieldNum()
{
	ASSERT(d->result!=NULL);
	return d->result->Fields->GetCount();
}

int CAdoSQLResult::GetValueSize(long lIndex)
{
	ASSERT(d->result != NULL);
	try
	{
		return d->result->Fields->GetItem(_variant_t(lIndex))->ActualSize;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return -1;
	}
}
int CAdoSQLResult::GetValueSize(char* lpszFieldName)
{
	ASSERT(d->result != NULL);
	try
	{
		return d->result->Fields->GetItem(_variant_t(lpszFieldName))->ActualSize;
	}
	catch (_com_error &e)
	{
		TRACE("%s", e.ErrorMessage());
		return -1;
	}
}

bool CAdoSQLResult::GetValue(int nColumns, _variant_t& vt, bool &nIsNull )
{
	_variant_t vtIndex;
	vtIndex.vt = VT_I2;
	vtIndex.iVal = nColumns;
	nIsNull=false;

	try
	{
		vt = d->result->GetCollect(vtIndex);
	}
	catch (_com_error &e)
	{		
		TRACE("GetValue error %s", e.ErrorMessage());
		return false;
	}
	return true;
}
bool CAdoSQLResult::GetValue(char* strFieldName, _variant_t& vt, bool &nIsNull )
{
	nIsNull=false;
	try
	{
		vt = d->result-> GetCollect(_variant_t(strFieldName));
	}
	catch (_com_error &e)
	{		
		TRACE("GetValue error %s", e.ErrorMessage());
		return false;
	}
	return true;
}

bool CAdoSQLResult::GetValue(int nColumns, bool& bValue, bool &nIsNull )
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
bool CAdoSQLResult::GetValue(char* strFieldName, bool& bValue, bool &nIsNull )
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

bool CAdoSQLResult::GetValue(int nColumns, byte& bValue, bool &nIsNull )
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

bool CAdoSQLResult::GetValue(char* strFieldName, byte& bValue, bool &nIsNull )
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

bool CAdoSQLResult::GetValue(int nColumns, short& nValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(char* strFieldName, short& nValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(int nColumns, int& nValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(char* strFieldName, int& nValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(int nColumns, long& lValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(char* strFieldName, long& lValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(int nColumns, __int64& lValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(char* strFieldName, __int64& lValue, bool &nIsNull)
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
bool CAdoSQLResult::GetValue(int nColumns, float& fValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(char* strFieldName, float& fValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(int nColumns, double& dValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(char* strFieldName, double& dValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(int nColumns, string& strValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(char* strFieldName, string& strValue, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(int nColumns, char* strValue, short len,bool &nIsNull)
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
bool CAdoSQLResult::GetValue(char* strFldName, char* strValue, short len, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(int nColumns, tm& time, bool &nIsNull)
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

bool CAdoSQLResult::GetValue(char* strFieldName, tm& time, bool &nIsNull)
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


bool CAdoSQLResult::GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull)
{
	try
	{
		if(d->result->adoEOF||d->result->BOF) 
			return false;

		_variant_t vtIndex;
		vtIndex.vt = VT_I2;
		vtIndex.iVal = nColumns;
		_variant_t vt;

		//得到数据的长度
		long lDataSize = d->result->Fields->GetItem(vtIndex)->ActualSize;
		vt = d->result->GetFields()->GetItem(vtIndex)->GetChunk(lDataSize);
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

bool CAdoSQLResult::GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull)
{
	try
	{
		_variant_t vt;
		//得到数据的长度
		long lDataSize = d->result->Fields->GetItem(_variant_t(strFieldName))->ActualSize;
		vt = d->result->GetFields()->GetItem(_variant_t(strFieldName))->GetChunk(lDataSize);
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



















////////////////////////////////AdoDriver//////////////////////////////////////////

CAdoSQLDriver::CAdoSQLDriver(void)
{
	init();
// 	::CoInitialize(NULL);
// 	//HRESULT hr=ado_d->m_pConn.CreateInstance(__uuidof(Connection));
// 
// 	if(FAILED(hr))
// 		ado_d->m_pConn=NULL;
}
CAdoSQLDriver::CAdoSQLDriver(_ConnectionPtr conn)
{
	init();
	ado_d->m_pConn = conn;
	if (conn) {
		setOpen(true);
	}
}
CAdoSQLDriver::~CAdoSQLDriver(void)
{
	if (ado_d)
		delete ado_d;
	ado_d = NULL;
// 	if(ado_d->m_pConn)
// 	{
// 		ado_d->m_pConn.Release();
// 		ado_d->m_pConn = NULL;
// 	}
// 
// 	::CoUninitialize();
}

bool CAdoSQLDriver::open(const char* db, const char* user, const char* password,
		  const char* host, int port, const char* connOpts)
{
	char szConn[100] = "";

	if(strlen(db) < 1)
		sprintf_s(szConn,"Provider=MSDASQL.1; Data Source=%s; User ID=%s; PWD=%s",host,user,password);
	else
		sprintf_s(szConn,"driver={SQL Server};Server=%s;UID=%s;PWD=%s;DATABASE=%s", host, user, password, db);

	HRESULT hr;
	try
	{
		if(IsOpen())
			close();

		_bstr_t bstrConn(szConn);
		hr= ado_d->m_pConn->Open(bstrConn,"","",adModeUnknown);			
		if(SUCCEEDED(hr))
			return TRUE;
		else
			return FALSE;
	}
	catch(_com_error e)///捕捉异常
	{
		m_strError = "打开数据库失败!\r\n错误信息:" + string(e.ErrorMessage());
		TRACE(m_strError.c_str());///显示错误信息
		return FALSE;
	} 
	return FALSE;
}

bool CAdoSQLDriver::IsOpen() const
{
	if (ado_d->m_pConn != NULL)
		return ado_d->m_pConn->GetState() != adStateClosed;

	return FALSE;
}
void CAdoSQLDriver::close()
{
	if (IsOpen())
	{
		ado_d->m_pConn->Close();
	}
	ado_d->m_pConn.Release();
	ado_d->m_pConn=NULL;
}

bool CAdoSQLDriver::BeginTransaction()
{
	if (!IsOpen())
		return false;

	m_strError = "";
	if (ado_d->m_pConn->BeginTrans() > 0)
	{
		m_strError = "Unable to begin ADO begin transaction";
		TRACE(m_strError.c_str());
		return false;
	}

	return true;
}
bool CAdoSQLDriver::CommitTransaction()
{
	if (!IsOpen())
		return false;

	m_strError = "";
	if (ado_d->m_pConn->CommitTrans() > 0)
	{
		m_strError = "Unable to begin ADO commit transaction";
		TRACE(m_strError.c_str());
		return false;
	}

	return true;
}
bool CAdoSQLDriver::RollbackTransaction()
{
	if (!IsOpen())
		return false;

	m_strError = "";
	if (ado_d->m_pConn->RollbackTrans() > 0)
	{
		m_strError = "Unable to begin ADO rollback transaction";
		TRACE(m_strError.c_str());
		return false;
	}

	return true;
}

int  CAdoSQLDriver::ExecuteSql(const char* strSql)
{
	if (!IsOpen())
		return 0;

	_CommandPtr pCommand;
	m_strError = "";

	try
	{
		if (FAILED(pCommand.CreateInstance(__uuidof(Command))))
			return FALSE;

		pCommand->PutRefActiveConnection(ado_d->m_pConn);
		pCommand->PutCommandText(strSql);
		pCommand->Execute(NULL, NULL, adCmdText | adExecuteNoRecords);
	}
	catch (_com_error &e)
	{	
		m_strError = "ado error message:%s" + string(e.ErrorMessage()) ;
		TRACE(m_strError.c_str());
		return 0;
	}

	return 1;
}

int CAdoSQLDriver::ExecBinary(const char* strSql, byte** pParas,int *len, short nParamsNum)
{
	// 插入二进制需要根据数据集
	return 0;
}

bool CAdoSQLDriver::PutBlob(const char *szWhereSql, const char *szTableName, const char* szFldName, byte*szVal, long nSize)
{
	// ADO 要先查询
	CString strSql= "";
	strSql.Format("SELECT %s FROM %s", szFldName, szTableName);
	if (szWhereSql && strlen(szWhereSql)>0)
		strSql += " WHERE " + CString(szWhereSql);

	_RecordsetPtr pRecordset;
	m_strError = "";
	try
	{
		if (FAILED(pRecordset.CreateInstance(__uuidof(Recordset))))
			return FALSE;

		if (FAILED(pRecordset->Open(_variant_t(strSql), _variant_t((IDispatch*)pRecordset), adOpenStatic, adLockOptimistic, adCmdText)))
			return FALSE;

		if (pRecordset == NULL || lstrlen(szFldName) < 1)
			return FALSE;

		VARIANT   varBlob; 
		SAFEARRAY  * psa; 
		SAFEARRAYBOUND rgsabound[1]; 

		rgsabound[0].lLbound = 0; 
		rgsabound[0].cElements = nSize; 
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound); 
		for (long i = 0; i < (long)nSize; ++i) 
			SafeArrayPutElement (psa, &i, szVal++); 
		varBlob.vt = VT_ARRAY | VT_UI1; 
		varBlob.parray = psa; 

		pRecordset->GetFields()->GetItem(_variant_t(szTableName))->AppendChunk(varBlob);
		pRecordset->Update(); 

		pRecordset->Close();
		pRecordset.Release();
	}
	catch (_com_error &e)
	{	
		m_strError = string(e.ErrorMessage());
		OutputDebugString(e.ErrorMessage());
		return FALSE;
	}

	return true;
}

CSqlRecordset* CAdoSQLDriver::createResult() const
{
	return new CAdoSQLResult(this, ado_d);
}

void CAdoSQLDriver::init()
{
	ado_d = new CAdoSQLDriverPrivate();
}
