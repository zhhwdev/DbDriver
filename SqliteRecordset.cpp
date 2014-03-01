#include "StdAfx.h"
#include "SqliteRecordset.h"
#include "CodingConv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSqliteRecordset::CSqliteRecordset(void)
{
	//m_Recordset = NULL;
	m_lRows = 0;
	m_lCols = 0;
	m_lCur = 0;
    m_stmt = NULL;
}
CSqliteRecordset::CSqliteRecordset(sqlite3_stmt *stmt)
{
	m_lCur = 0;
    m_stmt =stmt;
	m_lCols = sqlite3_column_count(stmt);
	InitRowNum();
}

CSqliteRecordset::~CSqliteRecordset(void)
{
	clear();
}

 //字段名 
string CSqliteRecordset::GetFieldName(long lIndex)
{
	ASSERT(m_stmt != NULL);

	return sqlite3_column_name(m_stmt,lIndex);
}

//字段序号
int CSqliteRecordset::GetIndex(char* strFieldName)
{
	ASSERT(m_stmt != NULL);

	char *szFieldName = new char[50];
	CCodingConv::GB2312_2_UTF8(szFieldName,50,strFieldName,(int)strlen(strFieldName));//将GB2312编码转为UTF8编码

	for (long i=0; i<m_lCols ;i++)
	{
		string name = sqlite3_column_name(m_stmt,i);
		if(name == szFieldName)
			return i;
	}

	return -1;
}
//将数据类型从字符串转为int
int CSqliteRecordset::AnalyType(CString strType)
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

// 字段数据类型 
int CSqliteRecordset::GetFieldType(long lIndex)
{
    ASSERT(m_stmt != NULL);
	char* szType= const_cast<char*>(sqlite3_column_decltype(m_stmt,lIndex)); 

	return AnalyType(szType);
}
int CSqliteRecordset::GetFieldType(char* lpszFieldName)
{ 
	ASSERT(m_stmt != NULL);

	long lIndex=GetIndex(lpszFieldName);
	char* szType= const_cast<char*>(sqlite3_column_decltype(m_stmt,lIndex)); 
	
	return AnalyType(szType);
}

// 字段定义长度
int  CSqliteRecordset::GetFieldDefineSize(long lIndex)
{
	ASSERT(m_stmt != NULL);
	CString strType= (CString)const_cast<char*>(sqlite3_column_decltype(m_stmt,lIndex));
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

int  CSqliteRecordset::GetFieldDefineSize(char* strFieldName)
{
	ASSERT(m_stmt != NULL);

	long lIndex = GetIndex(strFieldName);
	CString strType= (CString)const_cast<char*>(sqlite3_column_decltype(m_stmt,lIndex));
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

//字段判断是否为NULL
bool CSqliteRecordset::IsFieldNull(char* lpFieldName) 
{
    int type = GetFieldType(lpFieldName); //
	return type == SQLITE_NULL;
}

bool CSqliteRecordset::IsFieldNull(int nIndex) 
{
    int type = GetFieldType(nIndex);
	return type == SQLITE_NULL;
}

//清空数据集
void CSqliteRecordset::clear()
{
	if (m_stmt != NULL)
		sqlite3_finalize(m_stmt);  
}

// 判断查询结果集是否为NULL
bool CSqliteRecordset::IsNull()
{
	if (m_stmt == NULL)
		return true;

	return false;
}

//结果集是否为空
bool CSqliteRecordset::IsEmpty()
{
	if (m_stmt != NULL && sqlite3_column_count(m_stmt) <= 0)
		return true;

	return false;
}
bool CSqliteRecordset::IsNullEmpty()
{
	if (m_stmt == NULL || sqlite3_column_count(m_stmt) <= 0)
		return true;

	return false;
}

//判断执行是否成功
bool CSqliteRecordset::IsSuccess()
{
	if (m_stmt != NULL)
		return true;

	return false;
}

//是否为最后一条记录
bool CSqliteRecordset::IsEOF()
{
	return m_lCur == m_lRows;
}

//是否为第一条记录
bool CSqliteRecordset::IsBOF()
{
	return m_lCols == 1;
}

//查询结果集光标移动
//移到第一条记录
void CSqliteRecordset::MoveFirst()
{
	ASSERT(m_stmt != NULL);
	
	if(m_lCur != 1)
       sqlite3_reset(m_stmt);

	int ret = sqlite3_step(m_stmt);
	if (ret != SQLITE_DONE && ret != SQLITE_ROW)
	{
		sqlite3_finalize(m_stmt);
		return;
	}

    m_lCur = 1;
}
//下一条记录
void CSqliteRecordset::MoveNext()
{
	ASSERT(m_stmt != NULL);
	int ret = sqlite3_step(m_stmt);
	if (ret != SQLITE_DONE && ret != SQLITE_ROW)
	{
		sqlite3_finalize(m_stmt);
		return;
	}
	m_lCur ++;
}
//void CSqliteRecordset::MovePrevious()
//移到最后一条记录
void CSqliteRecordset::MoveLast()
{
	ASSERT(m_stmt != NULL);

	int ret;
	while (m_lCur < m_lRows)
	{
		 ret = sqlite3_step(m_stmt);
		if (ret != SQLITE_DONE && ret != SQLITE_ROW)
		{
			sqlite3_finalize(m_stmt);
			return;
		}
		m_lCur++;
	}
	
}

//取光标位置
long CSqliteRecordset::GetCursorPos()
{
	return m_lCur;
}
//设置光标位置
long CSqliteRecordset::SetCursorPos(long i)
{
	ASSERT(m_stmt != NULL);
	
	if (i<1)
		return -1;

	if(i == m_lCur)//设置的光标位置为当前位置
		return 1;

	int ret;
	m_lCur = 0;
	sqlite3_reset(m_stmt);//重置SQL语句

	do 
	{
		ret = sqlite3_step(m_stmt);
		if (ret != SQLITE_DONE && ret != SQLITE_ROW)
		{
			sqlite3_finalize(m_stmt);
			return -1;
		}
		m_lCur++;
	} while (m_lCur < i);

	return 1;
}

//字段数
int CSqliteRecordset::GetFieldNum() 
{
	ASSERT(m_stmt != NULL);
	return m_lCols;
}

//行数
int CSqliteRecordset::GetRowNum()
{
	ASSERT(m_stmt != NULL);
	return m_lRows; 
}

//取数据
int CSqliteRecordset::GetValueSize(long lIndex)
{
	return 0;
}

int CSqliteRecordset::GetValueSize(char* lpszFieldName)
{
	return 0;
}

bool CSqliteRecordset::GetValue(int nColumns, bool& bValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	if(nColumns < 0)
		return false;

	int nValue = sqlite3_column_int(m_stmt,nColumns);
	if (nValue == 1)
		bValue = true;
	else
		bValue = false;

	return true;
}
bool CSqliteRecordset::GetValue(char* strFieldName, bool& bValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	int nValue = sqlite3_column_int(m_stmt,nIndex);
	if (nValue == 1)
		bValue = true;
	else
		bValue = false;

	return true;
}
bool CSqliteRecordset::GetValue(int nColumns, byte& nValue, bool &nIsNull )//int1
{
	ASSERT(m_stmt != NULL);
	if(nColumns < 0)
		return false;

	int values = sqlite3_column_int(m_stmt,nColumns);
	nValue = (byte)values;
	return true;
}
bool CSqliteRecordset::GetValue(char* strFieldName, byte& nValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	int values = sqlite3_column_int(m_stmt,nIndex);
	nValue = (byte)values;
	return true;
}

bool CSqliteRecordset::GetValue(int nColumns, float& fValue, bool &nIsNull ) 
{
	ASSERT(m_stmt != NULL);
	if(nColumns < 0)
		return false;

	fValue = (float)sqlite3_column_double(m_stmt,nColumns);
	return true;
}

bool CSqliteRecordset::GetValue(char* strFieldName, float& fValue, bool &nIsNull)
{
     ASSERT(m_stmt != NULL);
	 int nIndex = GetIndex(strFieldName);
	 if (nIndex < 0)
	 return false;

	 fValue = (float)sqlite3_column_double(m_stmt,nIndex);
	 return true;
}

bool CSqliteRecordset::GetValue(int nColumns, long& lValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	if(nColumns < 0)
		return false;

	lValue = (long)sqlite3_column_int(m_stmt,nColumns);
	return true;
}

 bool CSqliteRecordset::GetValue(char* strFieldName, long& lValue, bool &nIsNull )
 {
	 ASSERT(m_stmt != NULL);
	 int nIndex = GetIndex(strFieldName);
	 if (nIndex < 0)
		 return false;

	 lValue = (long)sqlite3_column_int(m_stmt,nIndex);
	 return true;
 }

 bool CSqliteRecordset::GetValue(int nColumns, string& strValue, bool &nIsNull) 
 {
	 ASSERT(m_stmt != NULL);
	 if(nColumns < 0)
		 return false;

	 strValue = (string)(char*)sqlite3_column_text(m_stmt,nColumns);
	 if (strValue.length() < 1)
		 return false;

	 char* szGBKValue = new char[strValue.length()];
	 CCodingConv::UTF8_2_GB2312(szGBKValue,(int)strValue.length()+1,strValue.c_str(),(int)strValue.length());//将UTF8编码字符转为GBK
     strValue = szGBKValue;

	 return true;
 }
bool CSqliteRecordset::GetValue(char* strFieldName, string& strValue, bool &nIsNull)
{
	ASSERT(m_stmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	strValue = (string)(char*)sqlite3_column_text(m_stmt,nIndex);
	if (strValue.length() < 1)
		return false;

	char* szGBKValue = new char[strValue.length()];
	CCodingConv::UTF8_2_GB2312(szGBKValue,(int)strValue.length()+1,strValue.c_str(),(int)strValue.length());//将UTF8编码字符转为GBK
	strValue = szGBKValue;
	return true;  
}

bool CSqliteRecordset::GetValue(int nColumns, char* strValue, short len,bool &nIsNull)
{
	ASSERT(m_stmt != NULL);
 	if(nColumns < 0)
		return false;

	char * szValue = (char*)sqlite3_column_text(m_stmt,nColumns);
	if (szValue == NULL)
		return false;

     CCodingConv::UTF8_2_GB2312(strValue,len,szValue,(int)strlen(szValue));

	//strcpy_s(strValue,len,szValue);
	return true;
}
bool CSqliteRecordset::GetValue(char* strFldName, char* strValue, short len, bool &nIsNull)
{
	ASSERT(m_stmt != NULL);
	int nIndex = GetIndex(strFldName);
	if (nIndex < 0)
		return false;

	char *szValue = (char*)sqlite3_column_text(m_stmt,nIndex);
	if (szValue == NULL)
		return false;

	 CCodingConv::UTF8_2_GB2312(strValue,len,szValue,(int)strlen(szValue));
	//strcpy_s(strValue,len,szValue);
	return true;
}

bool CSqliteRecordset::GetValue(int nColumns, int& nValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	if(nColumns < 0)
		return false;

	nValue = sqlite3_column_int(m_stmt,nColumns);
	return true;
}
bool CSqliteRecordset::GetValue(char* strFieldName, int& nValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	nValue = sqlite3_column_int(m_stmt,nIndex);
	return true;  
}
bool CSqliteRecordset::GetValue(int nColumns, __int64& lValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	if(nColumns < 0)
		return false;

	lValue = sqlite3_column_int64(m_stmt,nColumns);
   return true;
}
bool CSqliteRecordset::GetValue(char* strFieldName, __int64& lValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	lValue = sqlite3_column_int64(m_stmt,nIndex);
   return true;
}
bool CSqliteRecordset::GetValue(int nColumns, short& nValue, bool &nIsNull )
 {
	 ASSERT(m_stmt != NULL);
	 if(nColumns < 0)
		 return false;

	 nValue = (short)sqlite3_column_int(m_stmt,nColumns);
	 return true;
 }
bool CSqliteRecordset::GetValue(char* strFieldName, short& nValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	nValue = (short)sqlite3_column_int(m_stmt,nIndex);
	return true;  
}

bool CSqliteRecordset::GetValue(int nColumns, double& lValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	if(nColumns < 0)
		return false;

	lValue = sqlite3_column_double(m_stmt,nColumns);
	return true;
}
bool CSqliteRecordset::GetValue(char* strFieldName, double& lValue, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	lValue = sqlite3_column_double(m_stmt,nIndex);
	return true;  
}
bool CSqliteRecordset::GetValue(int nColumns, tm& time, bool &nIsNull )
{
	if (nColumns <0)
		return false;

	short len(25);
	char *szDataTime = new char[len];
	GetValue(nColumns,szDataTime,len,nIsNull);
	if(szDataTime == NULL)
		return false;

	time = AnalyDateTime(szDataTime);
	return true;  
}
bool CSqliteRecordset::GetValue(char* szFieldName, tm& time, bool &nIsNull )
{
	if (szFieldName == NULL)
		return false;

	short len(25);
	char * szDateTime = new char[len];
	GetValue(szFieldName,szDateTime,len,nIsNull);
	if(szDateTime == NULL)
		return false;

	time = AnalyDateTime(szDateTime);
	return true;  
}

bool CSqliteRecordset::GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);
	
	const void* pcvData = sqlite3_column_blob(m_stmt,nColumns);  //const void*转byte**
	*len = sqlite3_column_bytes(m_stmt,nColumns);

	*bDat = new byte[*len];
	memcpy(*bDat,pcvData,*len);
	return true;  
}
bool CSqliteRecordset::GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull )
{
	ASSERT(m_stmt != NULL);

	int nIndex = GetIndex(strFieldName);
	if (nIndex < 0)
		return false;

	const void* pcvData = sqlite3_column_blob(m_stmt,nIndex);  //const void*转byte**
	*len = sqlite3_column_bytes(m_stmt,nIndex);

	*bDat = new byte[*len];
	memcpy(*bDat,pcvData,*len);
	return true;  
}


void CSqliteRecordset::InitRowNum()
{
	ASSERT(m_stmt != NULL);

	int  rc;
	m_lRows = 0;
	sqlite3_reset(m_stmt);
	
	do{   
		rc = sqlite3_step(m_stmt);  
		if(rc == SQLITE_ROW /*|| rc== SQLITE_DONE*/)  
		{  
			m_lRows++;
		}  

		else 
			break;  

	}while(1);  

	sqlite3_reset(m_stmt);
}



//strDateTime:YYYY-MM-DD HH:MM:SS
//当strDateTime: YYYY-MM-DD HH:MM (秒为0)
tm& CSqliteRecordset::AnalyDateTime(char* szDateTime)
{
	tm *dateTime = new tm; 
	string str="";
	string strDateTime = (string)szDateTime;
   
	int nPos = (int)strDateTime.find_first_of(' ');
	//1.根据空格分解时间为两部分
	string strValue1 = strDateTime.substr(0, nPos);
	string strValue2 = strDateTime.substr(nPos+1, (int)strlen(szDateTime) - nPos - 1);

	//2.解析日期
	int nFPos = (int)strValue1.find_first_of('-');
	int nLPos = (int)strValue1.find_last_of('-');
	dateTime->tm_year = atoi(strValue1.substr(0, nFPos).c_str());
	dateTime->tm_mon = atoi(strValue1.substr(nFPos + 1, nLPos - nFPos - 1).c_str());
	dateTime->tm_mday = atoi(strValue1.substr(nLPos + 1, (int)strlen(strValue1.c_str())-nLPos - 1).c_str());

	//3.解析时间
	nFPos = (int)strValue2.find_first_of(':');
	nLPos = (int)strValue2.find_last_of(':');
	dateTime->tm_hour = atoi(strValue2.substr(0, nFPos).c_str());
	dateTime->tm_min = atoi(strValue2.substr(nFPos + 1, nLPos - nFPos - 1).c_str());

	if(nFPos == nLPos)
		dateTime->tm_sec =0;
	else
	{
		string strValue3 = strValue2.substr(nLPos + 1, (int)strlen(strValue2.c_str())-nLPos - 1);
	    dateTime->tm_sec = atoi(strValue3.substr(0, nFPos).c_str());
	}
  
	return *dateTime;
}