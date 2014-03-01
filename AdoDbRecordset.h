#pragma once

#include "SqlDriver.h"



class COMMON_DBLIB CAdoRecordset:public CDbRecordSet
{
public:
	CAdoRecordset(_RecordsetPtr pRecordset);
	~CAdoRecordset(void);
public:
	//取字段个数	
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
	bool IsFieldNull(int nIndex);
public:
	bool IsNull();
	bool IsEmpty();
	bool IsNullEmpty();
	void clear();

	//判断执行是否成功
	virtual bool IsSuccess();
public:
	bool IsEOF();
	bool IsBOF();

	void MoveFirst() ;
	void MoveNext();
	void MovePrevious() ;
	void MoveLast() ;

	//设置光标位置
	long GetCursorPos();
	long SetCursorPos(long i);

	//取结果集行数
	int GetRowNum();

	//获取数据实际长度
 	int GetValueSize(long lIndex);
 	int GetValueSize(char* lpszFieldName);

	bool GetValue(int nColumns, _variant_t& vt, bool &nIsNull );
	bool GetValue(char* strFieldName, _variant_t& vt, bool &nIsNull );
	
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
	_RecordsetPtr m_pRecordset;
	byte* m_blobData;
};
