#include "StdAfx.h"
#include "SqlDataBase.h"

#include "SqlRecordset.h"
#include "SqlDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CSqlQueryPrivate
{
public:
	CSqlQueryPrivate(CSqlRecordset* result);
	~CSqlQueryPrivate();
	//QAtomicInt ref;
	CSqlRecordset* pSqlResult;
	size_t refCount;

};

CSqlQueryPrivate::CSqlQueryPrivate(CSqlRecordset* result)
: refCount(1), pSqlResult(result)
{
	if (!pSqlResult)
		pSqlResult = NULL;
}

CSqlQueryPrivate::~CSqlQueryPrivate()
{
	if (pSqlResult)
		delete pSqlResult;
	pSqlResult = NULL;
}


//////////////////////////////////////////////////////////////////////////
CSqlQuery::CSqlQuery(CSqlRecordset *result)
{
    d = new CSqlQueryPrivate(result);
}

CSqlQuery::~CSqlQuery()
{
	if (!d)
		return;

	d->refCount--;
    if (!d->refCount)
	{
		if (d)
	       delete d;
		d= NULL;
	}
}

CSqlQuery::CSqlQuery(const CSqlQuery& other)
{   
    d = other.d;
	d->refCount++;
}

bool CSqlQuery::Close()
{
	if (d->refCount == 1)
	{
		delete d;
		d = NULL;
		return true;
	}

	return false;
}
void qInit(CSqlQuery *q, const char* query, CSqlDataBase *db)
{
	if (db->isValid()) 
		*q = CSqlQuery(((CSqlDriver*)(db->driver()))->createResult());

	if (query && strlen(query)>0)
		q->Query(query);
}

//////////////////////////////////CDataBase////////////////////////////////////////
inline void CSqlQuery::qAtomicAssign(CSqlQueryPrivate *&d, CSqlQueryPrivate *x)
{
	if (d == x)
		return;
	x->refCount++;
	if (d != NULL)
	{
		if (d->refCount > 0)						// 有可能有问题
			delete d;
	}

	d = x;
}

CSqlQuery::CSqlQuery(const char* query, CSqlDataBase *db)
{
	d = NULL;
	qInit(this, query, db);
}

CSqlQuery::CSqlQuery(CSqlDataBase *pDb)
{
	d = NULL;
	qInit(this, "", pDb);
}

CSqlQuery& CSqlQuery::operator=(const CSqlQuery& other)
{
	qAtomicAssign(d, other.d);
	return *this;
}

bool CSqlQuery::isFieldNull(int field) const
{
	if (d->pSqlResult->isActive())
		return d->pSqlResult->IsFieldNull(field);
	return true;
}

bool CSqlQuery::Query(const char* query)
{
	if (d->refCount != 1) 
		*this = CSqlQuery(driver()->createResult());
	else 
		d->pSqlResult->setActive(false);

	d->pSqlResult->setQuery(query);
	if (!driver()->IsOpen() || driver()->isOpenError()) 
	{
		TRACE("QSqlQuery::exec: database not open");
		return false;
	}
	if (!query || strlen(query)<1) 
	{
		TRACE("QSqlQuery::exec: empty query");
		return false;
	}

	return d->pSqlResult->reset(query);
}

bool CSqlQuery::QueryBinary(const char* query)
{
	if (d->refCount != 1) 
		*this = CSqlQuery(driver()->createResult());
	else 
		d->pSqlResult->setActive(false);

	d->pSqlResult->setQuery(query);
	if (!driver()->IsOpen() || driver()->isOpenError()) 
	{
		TRACE("CSqlQuery::exec: database not open");
		return false;
	}
	if (!query || strlen(query)<1) 
	{
		TRACE("CSqlQuery::exec: empty query");
		return false;
	}

	return d->pSqlResult->reset(query, 1);
}


string CSqlQuery::lastQuery() const
{
    return d->pSqlResult->lastQuery();
}

const CSqlDriver *CSqlQuery::driver() const
{
    return (d->pSqlResult->driver());
}

int CSqlQuery::GetRowNum() const
{
	
	if (!isValid())	
		return -1;
	
	return d->pSqlResult->SelectRowCount();
}
bool CSqlQuery::isValid() const
{
	return d->pSqlResult != NULL;
}

bool CSqlQuery::isActive() const
{
	return d->pSqlResult->isActive();
}

bool CSqlQuery::isSelect() const
{
	return d->pSqlResult->isSelect();
}


void CSqlQuery::clear()
{
	*this = CSqlQuery(driver()->createResult());
}

bool CSqlQuery::Query()
{
// 	d->sqlResult->resetBindCount();
// 
// 	if (d->sqlResult->lastError().isValid())
// 		d->sqlResult->setLastError(QSqlError());

	return d->pSqlResult->exec();
}


bool CSqlQuery::IsEmpty()
{
	if (!d || !d->pSqlResult)
		return true;

	return d->pSqlResult->IsEmpty();
}

bool CSqlQuery::IsNull()
{
	if (!d || !d->pSqlResult)
		return true;

	return d->pSqlResult->IsNull();
}

int CSqlQuery::GetFieldDefineSize(const char*szFldName)
{
	if(!d->pSqlResult)
		return -1;

	return d->pSqlResult->GetFieldDefineSize(szFldName);
}
int CSqlQuery::GetFieldDefineSize(int nFldIndex)
{	
	if (d->pSqlResult == NULL)
		return -1;

	return d->pSqlResult->GetFieldDefineSize(nFldIndex);
}

int CSqlQuery::GetFieldNum()
{
	if (d->pSqlResult == NULL)
		return -1;

	return d->pSqlResult->GetFieldNum();
}

string CSqlQuery::GetFieldName(int i)
{
	if (d->pSqlResult == NULL)
		return "";

	return d->pSqlResult->GetFieldName(i);
}

int CSqlQuery::GetFieldType(const char*szFldName)
{
	if(!d->pSqlResult)
		return -1;

	return d->pSqlResult->GetFieldType(szFldName);

}
int CSqlQuery::GetFieldType(int nFldIndex)
{
	if (d->pSqlResult == NULL)
		return -1;

	return d->pSqlResult->GetFieldType(nFldIndex);
}

// 移动行光标
void CSqlQuery::MoveFirst()
{
	d->pSqlResult->MoveFirst();	
}						
void CSqlQuery::MoveNext() 
{
	d->pSqlResult->MoveNext();
}			
void CSqlQuery::MovePrevious()
{
	d->pSqlResult->MovePrevious();
}					
void CSqlQuery::MoveLast()
{
	d->pSqlResult->MoveLast();
}
bool CSqlQuery::IsEOF()
{
	if (d->pSqlResult == NULL)
		return false;	
	return  d->pSqlResult->IsEOF();
}
bool CSqlQuery::IsBOF()
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->IsBOF();
}
bool CSqlQuery::GetValue(int nColumns, bool& bValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, bValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, bool& bValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, bValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, byte& nValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, nValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, byte& nValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, nValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, short& nValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, nValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, short& nValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, nValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, int& nValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, nValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, int& nValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, nValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, long& lValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, lValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, long& lValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, lValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, __int64& lValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, lValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, __int64& lValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, lValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, float& fValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, fValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, float& fValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, fValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, double& lValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, lValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, double& lValue, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, lValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, tm& time, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, time, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, tm& time, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, time, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, char* strValue, short len,bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, strValue, len, nIsNull);
}
bool CSqlQuery::GetValue(char* strFldName, char* strValue, short len, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFldName, strValue, len, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, string& strValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, strValue, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, string& strValue, bool &nIsNull)
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, strValue, nIsNull);
}

bool CSqlQuery::GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(nColumns, bDat, len, nIsNull);
}
bool CSqlQuery::GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull )
{
	if (d->pSqlResult == NULL)
		return false;
	return d->pSqlResult->GetValue(strFieldName, bDat, len, nIsNull);
}