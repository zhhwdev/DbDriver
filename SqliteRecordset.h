#pragma once
#include "SqlDriver.h"

//
//#include "spatialite.h"
//#include "spatialite/gaiageo.h"
//#pragma  comment(lib,"freexl_i.lib")
//#pragma  comment(lib,"gdal_i.lib")
//#pragma  comment(lib,"geos_c_i.lib")
//#pragma  comment(lib,"spatialite_i.lib")
//#pragma  comment(lib,"freexl_i.lib")
//#pragma  comment(lib,"geotiff_i.lib")
//#pragma  comment(lib,"iconv.lib")
//#pragma  comment(lib,"proj_i.lib")


#include "sqlite3.h"
#pragma comment(lib,"sqlite3.lib")
//}
class COMMON_DBLIB CSqliteRecordset : public CDbRecordSet
{
public:
	CSqliteRecordset(void);
	CSqliteRecordset(sqlite3_stmt *stmt);
	~CSqliteRecordset(void);
public:

	//字段数
	int GetFieldNum();

	// 字段名 
	string GetFieldName(long lIndex);	
	int GetIndex(char* strFieldName);

	// 字段数据类型 
	int GetFieldType(long lIndex);
	int GetFieldType(char* lpszFieldName);
		
	// 字段定义长度
	int  GetFieldDefineSize(long lIndex);
	int  GetFieldDefineSize(char* lpszFieldName);
	
	//字段判断是否为NULL
	bool IsFieldNull(char* lpFieldName);
	bool IsFieldNull(int nIndex) ;

public:
	void clear();
	// 判断查询结果集是否为NULL
	bool IsNull();
	// 判断结果集是否为empty(行数为0)
	bool IsEmpty();
	bool IsNullEmpty();

	//判断执行是否成功
	bool IsSuccess();
public:
	bool IsEOF();
	bool IsBOF();

	//查询结果集光标移动
	void MoveFirst();
	void MoveNext();
	void MovePrevious(){};
	void MoveLast();

	//设置光标位置(从1起算)
	virtual long GetCursorPos();
	virtual long SetCursorPos(long i);
    
	//取行数
	int GetRowNum();

	//获取数据实际长度
	int GetValueSize(long lIndex);
	int GetValueSize(char* lpszFieldName);

	//获取数据
	bool GetValue(int nColumns, bool& bValue, bool &nIsNull );
	bool GetValue(char* strFieldName, bool& bValue, bool &nIsNull );

	bool GetValue(int nColumns, byte& nValue, bool &nIsNull );
	bool GetValue(char* strFieldName, byte& nValue, bool &nIsNull );

	bool GetValue(int nColumns, short& nValue, bool &nIsNull );
	bool GetValue(char* strFieldName, short& fValue, bool &nIsNull );

	bool GetValue(int nColumns, int& nValue, bool &nIsNull );
	bool GetValue(char* strFieldName, int& nValue, bool &nIsNull );

	bool GetValue(int nColumns, long& lValue, bool &nIsNull );
	bool GetValue(char* strFieldName, long& lValue, bool &nIsNull );

	bool GetValue(int nColumns, __int64& lValue, bool &nIsNull );
	bool GetValue(char* strFieldName, __int64& lValue, bool &nIsNull );

	bool GetValue(int nColumns, float& fValue, bool &nIsNull );
	bool GetValue(char* strFieldName, float& fValue, bool &nIsNull );

	bool GetValue(int nColumns, double& dValue, bool &nIsNull );
	bool GetValue(char* strFieldName, double& dValue, bool &nIsNull );

	bool GetValue(int nColumns, string& strValue, bool &nIsNull );
	bool GetValue(char* strFieldName, string& strValue, bool &nIsNull );

	bool GetValue(int nColumns, char* strValue, short len,bool &nIsNull);
	bool GetValue(char* strFldName, char* strValue, short len, bool &nIsNull);

	bool GetValue(int nColumns, tm& time, bool &nIsNull );
	bool GetValue(char* strFieldName, tm& time, bool &nIsNull );

	bool GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull );
	bool GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull );
 
private:
	void InitRowNum();
	int AnalyType(CString strType);
	tm& AnalyDateTime(char* strDateTime);
	
private:
	/*char **m_Recordset;*/
	sqlite3_stmt *m_stmt;
	long m_lRows;   //行数
	long m_lCols;  //列数
	long m_lCur;  //光标位置(从1起算)
};
