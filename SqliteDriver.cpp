#include "StdAfx.h"
#include "SqliteDriver.h"
#include "CodingConv.h"
#include <Shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//将数据类型从字符串转为int
int AnalyType(CString strType)
{
	strType.MakeUpper();
	//匹配类型
	//整形数值

	if(strType == "TINYINT")
		return FLD_CHAR;

	else if(strType == "INT2")
		return FLD_INT2; 

	else if(strType == "MEDIUMINT")
		return FLD_INT4; 

	else if (strType == "SMALLINT" || strType == "INTEGER" || strType == "INT"  ||strType == "INT4" )
		return FLD_INT4;

	else if( strType == "UNSIGNED BIG INT") //猜的，未找到资料
		return FLD_INT8; 

	else if ( strType == "BIGINT" || strType == "INT8")
		return FLD_INT8;

	//字符串
	else if(strType.Find("VARYING CHARACTER") >=0 || strType.Find("NATIVE CHARACTER") >=0 ||  strType.Find("CHARACTER") >=0 ||
		strType.Find("NVARCHAR") >=0 || strType.Find("VARCHAR") >=0 || strType.Find("NCHAR") >=0 ||
		strType.Find("CHAR(") >=0 || strType== "TEXT" || strType == "CLOB")
		return FLD_VARCHAR;

	//浮点型
	else if(strType=="FLOAT" || strType=="FLOAT4" )
		return FLD_FLOAT;

	else  if(strType == "DOUBLE" || strType == "DOUBLE PRECISION" || strType == "FLOAT8" || strType == "REAL")
		return FLD_DOUBLE;

	//numinc
	else if(strType == "DATE" )//日期
		return FLD_DATE;

	else if(strType == "TIME")//时间
		return FLD_TIME;

	else if(strType == "DATETIME")//日期时间
		return FLD_DATETIME;

	else if(strType == "BOOLEAN")
		return FLD_BOOL;

	else if(strType == "NUMERIC" || strType.Find("DECIMAL(")) //用来存浮点数据的字符串
		return FLD_NUMERIC;

	//二进制
	else if(strType == "BLOB" || strType == "TINYBLOB" || strType == "MEDIUMBLOB" || strType == "LONGBLOB" || strType == "BYTEA")
		return FLD_BLOB;

	//空
	else
		return FLD_UNKNOWN;

}

//strDateTime:YYYY-MM-DD HH:MM:SS
//当strDateTime: YYYY-MM-DD HH:MM (秒为0)
void AnalyDateTime(char* szDateTime, tm &dateTime)
{
	string str="";
	string strDateTime = (string)szDateTime;

	int nPos = (int)strDateTime.find_first_of(' ');
	//1.根据空格分解时间为两部分
	string strValue1 = strDateTime.substr(0, nPos);
	string strValue2 = strDateTime.substr(nPos+1, (int)strlen(szDateTime) - nPos - 1);

	//2.解析日期
	int nFPos = (int)strValue1.find_first_of('-');
	int nLPos = (int)strValue1.find_last_of('-');
	dateTime.tm_year = atoi(strValue1.substr(0, nFPos).c_str());
	dateTime.tm_mon = atoi(strValue1.substr(nFPos + 1, nLPos - nFPos - 1).c_str());
	dateTime.tm_mday = atoi(strValue1.substr(nLPos + 1, (int)strlen(strValue1.c_str())-nLPos - 1).c_str());

	//3.解析时间
	nFPos = (int)strValue2.find_first_of(':');
	nLPos = (int)strValue2.find_last_of(':');
	dateTime.tm_hour = atoi(strValue2.substr(0, nFPos).c_str());
	dateTime.tm_min = atoi(strValue2.substr(nFPos + 1, nLPos - nFPos - 1).c_str());

	if(nFPos == nLPos)
		dateTime.tm_sec =0;
	else
	{
		string strValue3 = strValue2.substr(nLPos + 1, (int)strlen(strValue2.c_str())-nLPos - 1);
		dateTime.tm_sec = atoi(strValue3.substr(0, nFPos).c_str());
	}
}


class CSqliteDriverPrivate
{
public:
	CSqliteDriverPrivate() 
	{
		access = NULL;
	}
	sqlite3 *access;
};

class CSqliteResultPrivate
{
public:
	CSqliteResultPrivate(CSqliteSQLResult *res);
	bool isSelect();
	// initializes the recordInfo and the cache
	void init(const char **cnames, int numCols);
	void finalize();

	CSqliteSQLResult* q;
	sqlite3 *access;

	sqlite3_stmt *pStmt;
};
CSqliteResultPrivate::CSqliteResultPrivate(CSqliteSQLResult *res)
{
	q = res;
	pStmt = NULL;
	access = NULL;
}

CSqliteSQLResult::CSqliteSQLResult(const CSqliteSQLDriver* db):CSqlRecordset(db)
{
	d = new CSqliteResultPrivate(this);
	d->access = db->d->access;
	m_lCurrRow = 1;
	m_lFieldNum = 0;
	m_lAllSelectRow = 0;
	m_blobData = NULL;
}
CSqliteSQLResult::~CSqliteSQLResult()
{
	cleanup();
	delete d;
	d = NULL;

	if (m_blobData)
		delete []m_blobData;
	m_blobData = NULL;
}

void CSqliteSQLResult::cleanup()
{
	if (d->pStmt != NULL)
		sqlite3_finalize(d->pStmt);
	m_lCurrRow = 1;
}
bool CSqliteSQLResult::fetch(int i)
{
	return true;
}
bool CSqliteSQLResult::fetchFirst()
{
	return true;
}
bool CSqliteSQLResult::fetchLast()
{
	return true;
}

bool CSqliteSQLResult::IsFieldNull(int field)
{
	return true;
}
bool CSqliteSQLResult::reset (const char* query, short execFormate)
{
	cleanup();

	if (!driver())
		return false;
	if (!(driver()->IsOpen()))
		return false;

	if (!query || strlen(query)<1)
		return false;

	setSelect(false);
	int result;

	const char* szErrMsg = NULL;
	int nLen = (int)strlen(query);
	char *szSql = new char[nLen*2];
	CCodingConv::GB2312_2_UTF8(szSql, nLen*2, query, nLen);//将GB2312编码转为UTF8编码

	result = sqlite3_prepare_v2(d->access, szSql, (int)strlen(szSql), &d->pStmt, &szErrMsg);  

	if (result != SQLITE_OK)
	{
		if (strlen(szErrMsg)>0)
		{
			TRACE(szErrMsg);
			sqlite3_free(const_cast<char*> (szErrMsg));
		}
		szErrMsg = NULL;
	}

	if (szSql)
		delete []szSql;
	szSql = NULL;

	if (!d->pStmt)
	{
		setSelect(false);
		setActive(false);
		return false;
	}  

	m_lFieldNum = sqlite3_column_count(d->pStmt);
	InitRowNum();
	MoveFirst();
    setActive(true);

	return true;
}
int CSqliteSQLResult::size()
{
	return -1;
}
int CSqliteSQLResult::SelectRowCount()
{
	return m_lAllSelectRow;
}
long CSqliteSQLResult::lastInsertId() const
{
	return 0;
}
bool CSqliteSQLResult::prepare(const string& query)
{
	return true;
}
bool CSqliteSQLResult::exec()
{
	return true;
}

//取字段个数
int CSqliteSQLResult::GetFieldNum()
{
	if (d->pStmt == NULL)
		return -1;

	return m_lFieldNum;
}

//取字段名/字段编号
string CSqliteSQLResult::GetFieldName(long lIndex)
{
	if (d->pStmt == NULL)
		return "";

	return sqlite3_column_name(d->pStmt, lIndex);
}
int CSqliteSQLResult::GetIndex(const char* strFieldName)
{
	if (d->pStmt == NULL)
		return -1;

	char szFieldName[50];
	CCodingConv::GB2312_2_UTF8(szFieldName, 50,strFieldName,(int)strlen(strFieldName));//将GB2312编码转为UTF8编码

	for (long i=0; i<m_lFieldNum ;i++)
	{
		string name = sqlite3_column_name(d->pStmt, i);
		if(name == szFieldName)
			return i;
	}

	return -1;
}
int CSqliteSQLResult::GetFieldType(int lIndex)
{
	if (d->pStmt == NULL)
		return -1;

	char* szType= const_cast<char*>(sqlite3_column_decltype(d->pStmt, lIndex)); 

	return AnalyType(szType);
}

int CSqliteSQLResult::GetFieldType(const char* lpszFieldName)
{
	if (d->pStmt == NULL)
		return -1;

	long lIndex=GetIndex(lpszFieldName);

	return GetFieldType(lIndex);
}


void CSqliteSQLResult::MoveFirst()
{
	if (d->pStmt == NULL)
		return;

	//if(m_lCurrRow != 1)
	sqlite3_reset(d->pStmt);

	int nRtn = sqlite3_step(d->pStmt);
	if (nRtn!=SQLITE_DONE && nRtn!=SQLITE_ROW)
	{
		sqlite3_finalize(d->pStmt);
		return;
	}

	m_lCurrRow = 1;
}
void CSqliteSQLResult::MoveNext()
{
	if (d->pStmt == NULL)
		return;

	int nRtn = sqlite3_step(d->pStmt);
	if (nRtn!=SQLITE_DONE && nRtn!=SQLITE_ROW)
	{
		sqlite3_finalize(d->pStmt);
		return;
	}

	m_lCurrRow ++;
}
void CSqliteSQLResult::MovePrevious()
{
	// 暂时没有实现
}
void CSqliteSQLResult::MoveLast()
{
	int nRtn = 0;
	while (m_lCurrRow < m_lFieldNum)
	{
		nRtn = sqlite3_step(d->pStmt);
		if (nRtn != SQLITE_DONE && nRtn != SQLITE_ROW)
		{
			sqlite3_finalize(d->pStmt);
			return;
		}
		m_lCurrRow++;
	}
}

bool CSqliteSQLResult::IsEOF()
{
	if (d->pStmt == NULL)
		return true;

	return m_lCurrRow == m_lAllSelectRow;
}

bool CSqliteSQLResult::IsBOF()
{
	return m_lCurrRow == 1;
}

bool CSqliteSQLResult::IsNull()
{
	if (d->pStmt == NULL)
		return true;

	return false;
}
bool CSqliteSQLResult::IsEmpty()
{
	if (d->pStmt == NULL || m_lAllSelectRow < 1)
		return true;

	return false;
}

//获取数据实际长度
int CSqliteSQLResult::GetValueSize(long lIndex)
{
	return 0;
}
int CSqliteSQLResult::GetValueSize(char* lpszFieldName)
{
	return 0;
}
//取字段定义长度
int CSqliteSQLResult::GetFieldDefineSize(int lIndex)
{
	if (d->pStmt = NULL)
		return -1;

	CString strType= (CString)const_cast<char*>(sqlite3_column_decltype(d->pStmt,lIndex));
	int nPos1(-1),nPos2(-1);

	nPos1 = strType.Find('(');
	if(nPos1<0)
		return 0;

	nPos2 = strType.Find(')');
	if(nPos2<0)
		return 0;

	CString strLength = strType.Mid(nPos1+1,nPos2-nPos1-1);

	return  atoi(strLength);
}
int CSqliteSQLResult::GetFieldDefineSize(const char* lpszFieldName)
{
	if (d->pStmt = NULL)
		return -1;

	long lIndex = GetIndex(lpszFieldName);

	return  GetFieldDefineSize(lIndex);
}

bool CSqliteSQLResult::GetValue(int nColumns, bool& bValue, bool &nIsNull)
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	int nValue = sqlite3_column_int(d->pStmt,nColumns);
	if (nValue == 1)
		bValue = true;
	else
		bValue = false;

	return true;
}
bool CSqliteSQLResult::GetValue(char* strFieldName, bool& bValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, bValue, nIsNull);
}
bool CSqliteSQLResult::GetValue(int nColumns, byte& nValue, bool &nIsNull )//int1
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	int values = sqlite3_column_int(d->pStmt,nColumns);
	nValue = (byte)values;
	return true;
}
bool CSqliteSQLResult::GetValue(char* strFieldName, byte& nValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, nValue, nIsNull);
}

bool CSqliteSQLResult::GetValue(int nColumns, float& fValue, bool &nIsNull ) 
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	fValue = (float)sqlite3_column_double(d->pStmt,nColumns);
	return true;
}

bool CSqliteSQLResult::GetValue(char* strFieldName, float& fValue, bool &nIsNull)
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, fValue, nIsNull);
}

bool CSqliteSQLResult::GetValue(int nColumns, long& lValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	lValue = (long)sqlite3_column_int(d->pStmt,nColumns);
	return true;
}

bool CSqliteSQLResult::GetValue(char* strFieldName, long& lValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, lValue, nIsNull);
}

bool CSqliteSQLResult::GetValue(int nColumns, string& strValue, bool &nIsNull) 
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	char *szVal = (char*)sqlite3_column_text(d->pStmt,nColumns);
	if (!szVal)
		return false;

	int nLen = int(strlen(szVal));
	char* szGBKValue = new char[nLen + 1];
	CCodingConv::UTF8_2_GB2312(szGBKValue, nLen + 1, szVal, nLen);//将UTF8编码字符转为GBK
	strValue = string(szGBKValue);
	if (szGBKValue)
		delete []szGBKValue;
	szGBKValue = NULL;

	return true;
}
bool CSqliteSQLResult::GetValue(char* strFieldName, string& strValue, bool &nIsNull)
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

		return GetValue(nIndex, strValue, nIsNull);  
}

bool CSqliteSQLResult::GetValue(int nColumns, char* strValue, short len,bool &nIsNull)
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	char * szValue = (char*)sqlite3_column_text(d->pStmt,nColumns);
	if (szValue == NULL)
		return false;

	CCodingConv::UTF8_2_GB2312(strValue,len,szValue,(int)strlen(szValue));

	//strcpy_s(strValue,len,szValue);
	return true;
}
bool CSqliteSQLResult::GetValue(char* strFldName, char* strValue, short len, bool &nIsNull)
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, strValue, len, nIsNull);
}

bool CSqliteSQLResult::GetValue(int nColumns, int& nValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	nValue = sqlite3_column_int(d->pStmt,nColumns);
	return true;
}
bool CSqliteSQLResult::GetValue(char* strFieldName, int& nValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, nValue, nIsNull);  
}
bool CSqliteSQLResult::GetValue(int nColumns, __int64& lValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	lValue = sqlite3_column_int64(d->pStmt,nColumns);
	return true;
}
bool CSqliteSQLResult::GetValue(char* strFieldName, __int64& lValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, lValue, nIsNull);
}
bool CSqliteSQLResult::GetValue(int nColumns, short& nValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	nValue = (short)sqlite3_column_int(d->pStmt,nColumns);
	return true;
}
bool CSqliteSQLResult::GetValue(char* strFieldName, short& nValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, nValue, nIsNull);
}

bool CSqliteSQLResult::GetValue(int nColumns, double& lValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	if(nColumns < 0)
		return false;

	lValue = sqlite3_column_double(d->pStmt,nColumns);
	return true;
}
bool CSqliteSQLResult::GetValue(char* strFieldName, double& lValue, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, lValue, nIsNull);
}
bool CSqliteSQLResult::GetValue(int nColumns, tm& time, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	if (nColumns <0)
		return false;

	char szDataTime[25] = "";
	GetValue(nColumns, szDataTime, 25, nIsNull);
	if(szDataTime == NULL)
		return false;

	AnalyDateTime(szDataTime, time);
	return true;  
}
bool CSqliteSQLResult::GetValue(char* szFieldName, tm& time, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	if (szFieldName == NULL)
		return false;

	int nIndex = GetIndex(szFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, time, nIsNull);
}

bool CSqliteSQLResult::GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	
	if (nColumns < 0)
		return false;

	const void* pcvData = sqlite3_column_blob(d->pStmt,nColumns);  //const void*转byte**
	*len = sqlite3_column_bytes(d->pStmt,nColumns);
	*bDat = (byte*)pcvData;


	return true;  
}

bool CSqliteSQLResult::GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull )
{
	ASSERT(d->pStmt != NULL);
	if (!strFieldName)
		return false;

	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	return GetValue(nIndex, bDat, len, nIsNull);
}


inline void CSqliteSQLResult::InitRowNum()
{
	ASSERT(d->pStmt != NULL);
	int  rc;
	m_lAllSelectRow = 0;
	sqlite3_reset(d->pStmt);

	do{   
		rc = sqlite3_step(d->pStmt);  
		if(rc == SQLITE_ROW /*|| rc== SQLITE_DONE*/)  
		{  
			m_lAllSelectRow++;
		}  
		else 
			break;  
	}while(1);  

	sqlite3_reset(d->pStmt);
}

///////////////////////////////////CSqliteSQLDriver///////////////////////////////////////
CSqliteSQLDriver::CSqliteSQLDriver()
{
	d = new CSqliteDriverPrivate();
}

CSqliteSQLDriver::CSqliteSQLDriver(sqlite3 *connection)
{
	d = new CSqliteDriverPrivate();
	d->access = connection;
	setOpen(true);
	setOpenError(false);
}

CSqliteSQLDriver::~CSqliteSQLDriver()
{
    delete d;
}

bool CSqliteSQLDriver::open(const char* db, const char* user, const char* password,
		  const char* host, int port, const char* connOpts)
{
	if (db == NULL)
		return FALSE;

    int result = 0;
	char szDbName[128];

	//初始化sqlite调用spatialite的函数
	spatialite_init(0);	
	CCodingConv::GB2312_2_UTF8(szDbName, 128, db, (int)strlen(db));	 //将GB2312编码转为UTF8编码

	//判断是否创建内存数据库
	char* ch = strstr(szDbName,":memory:");
	if ( ch!=NULL || strlen(szDbName)<1)
		result = sqlite3_open(szDbName,&d->access);
	else
		result = sqlite3_open_v2(szDbName, &d->access,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX, NULL);//以读写模式/多线程串行模式打开数据库 
	
	setOpen(true);
	setOpenError(false);

	if (result == SQLITE_OK) 
	{		
		return true;
	}
	return false;
}

//断开连接
void CSqliteSQLDriver::close()
{
	if (IsOpen())
	{
		int nRet=sqlite3_close(d->access);
		d->access = NULL;
	}
}

//是否打开
bool CSqliteSQLDriver::IsOpen() 
{
	return d->access!=NULL;
} 

//处理数据库忙
//ms:等待的毫秒数
int CSqliteSQLDriver::BusyWait(int ms)
{
	if (d->access == NULL)
		return 0;
	return sqlite3_busy_timeout(d->access, ms);
}

//执行SQL语句（非查询）
int CSqliteSQLDriver::ExecuteSql(const char* strSql)
{
	ASSERT(d->access!=NULL);
	ASSERT (strSql  != "");

	int nRtn(-1);
	bool bCommit(false);
	char *errmsg =NULL;
	int len = (int)strlen(strSql);
	char *szSql = new char[len*2];

	CCodingConv::GB2312_2_UTF8(szSql,len*2,strSql,len);//将GB2312编码转为UTF8编码
	nRtn = sqlite3_exec(d->access, szSql, NULL, NULL, &errmsg);
	if (nRtn != SQLITE_OK)
		goto abort;

	if (szSql)
		delete []szSql;
	szSql = NULL;

	return 1;

abort:
	if (szSql)
		delete []szSql;
	szSql = NULL;

	if (errmsg)
	{
		//排除初始化空间元数据COMMIT的Bug
		if (strstr(errmsg,"no transaction is active") != NULL)
		{
			TRACE(errmsg);
			sqlite3_free(errmsg);
			return 1;
		}

		TRACE(errmsg);
		sqlite3_free(errmsg);
	}

	if ( bCommit )
	{
		//在事务开始后与结束之间出现问题则回滚事务
		sqlite3_exec( d->access, "ROLLBACK", NULL, NULL, NULL );
	}

	return 0;
}

int CSqliteSQLDriver::ExecBinary(const char* strSql, byte** pParas,int *len, short nParamsNum)
{
	ASSERT( d->access != NULL);
	ASSERT(strSql != NULL);

	if (nParamsNum <1 || len==NULL || pParas ==NULL)
		return 0;

	int nLen = (int)strlen(strSql);
	char *szSql = new char[2*nLen];
	sqlite3_stmt  *stmt = NULL; 
	const char* errmsg = NULL;  

	CCodingConv::GB2312_2_UTF8(szSql, 2*nLen,strSql, nLen);//将GB2312编码转为UTF8编码
	int result = sqlite3_prepare(d->access, szSql, (int)strlen(szSql), &stmt ,&errmsg);  

	for(int i=1; i<=nParamsNum;i++)
	{
		sqlite3_bind_blob(stmt,i,pParas[i-1],len[i-1],NULL);
		sqlite3_step(stmt);
		sqlite3_reset(stmt);
	}

	if (stmt != NULL)
		sqlite3_finalize(stmt);

	if (errmsg != NULL && strlen(errmsg)>0 )
		sqlite3_free(const_cast<char*> (errmsg));

	if (szSql)
		delete []szSql;
	szSql = NULL;
	return 1;
}
//批量执行Sql语句(非查询)
int CSqliteSQLDriver::ExecuteSql(char**& strSql,long len,long lOptions)
{
	ASSERT(d->access!=NULL );
	ASSERT (strSql != NULL);
	if (len <=0)
	return -1;

	int result;	bool bCommit(false);
	char *errmsg =NULL;
	char **szCodeSql = new char*[len];
	for(int i=0; i<len; i++)
	{
		int nLen = (int)strlen(strSql[i]);
		szCodeSql[i]= new char[nLen*2]; //转码后的长度*2 防止中文转码后长度增加 sqlCharNum[i]*2
		
		CCodingConv::GB2312_2_UTF8(szCodeSql[i], nLen*2, strSql[i], nLen);//将GB2312编码转为UTF8编码sqlCharNum[i]*2
	}

	result = sqlite3_exec(d->access, "BEGIN;", NULL,NULL, &errmsg); //申请开始事务
	if(result != SQLITE_OK)

		goto abort;

	bCommit = true;
	for (int i=0; i<len; i++) 
	{ 
		result = sqlite3_exec(d->access, szCodeSql[i], NULL,NULL, &errmsg); 
		if (result != SQLITE_OK)
			goto abort;
	} 

	result = sqlite3_exec(d->access, "COMMIT;",NULL,NULL, &errmsg); //提交事务
	if (result != SQLITE_OK)
		goto abort;

	if (szCodeSql)
	{
		for (int i=0;i<len;i++)
		{
			delete[]szCodeSql[i];
			szCodeSql[i] = NULL;
		}

		delete []szCodeSql;
		szCodeSql = NULL;
	}

	return 1;

abort:
	if (errmsg)
	{
		sqlite3_free(errmsg);
	}
	if ( bCommit )
	{
		//在事务开始后与结束之间出现问题则回滚事务
		sqlite3_exec( d->access, "ROLLBACK", NULL, NULL, NULL );
	}

	if (szCodeSql)
	{
		for (int i=0;i<len;i++)
		{
			delete[]szCodeSql[i];
			szCodeSql[i] = NULL;
		}

		delete []szCodeSql;
		szCodeSql = NULL;
	}

	return 0;
}

CSqlRecordset *CSqliteSQLDriver::createResult() const
{
	return new CSqliteSQLResult(this);
}

bool CSqliteSQLDriver::BeginTransaction()
{
	if (!IsOpen() || isOpenError())
		return false;

	char* err = NULL;
	int res = sqlite3_exec(d->access, "BEGIN", 0, this, &err);

	if (res == SQLITE_OK)
		return true;

	TRACE("Unable to begin transaction(SQLite3):%s\r\n", err);
	sqlite3_free(err);
	return false;
}

bool CSqliteSQLDriver::CommitTransaction()
{
	if (!IsOpen() || isOpenError())
		return false;

	char* err=NULL;
	int res = sqlite3_exec(d->access, "COMMIT", 0, this, &err);

	if (res == SQLITE_OK)
		return true;

	TRACE("Unable to commit transaction(SQLite3):%s\r\n", err);
	sqlite3_free(err);
	return false;
}

bool CSqliteSQLDriver::RollbackTransaction()
{
	if (!IsOpen() || isOpenError())
		return false;

	char* err=NULL;
	int res = sqlite3_exec(d->access, "ROLLBACK", 0, this, &err);

	if (res == SQLITE_OK)
		return true;

	TRACE("Unable to rollback transaction(SQLite3):%s\r\n", err);
	sqlite3_free(err);
	return false;
}