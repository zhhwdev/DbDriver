#pragma once

#include <string>
using namespace std;

class CSqlDriver;
class CSqlRecordsetPrivate;
class CSqlResult;


class CSqlRecordset
{
	friend class CSqlQuery;
	friend class CDbRecordsetPrivate;
public:
	explicit CSqlRecordset(const CSqlDriver * db);
	virtual ~CSqlRecordset();


	string lastQuery() const;
	bool isActive() const;
	bool isSelect() const;
	bool isForwardOnly() const;
	CSqlDriver* driver();
	virtual void setActive(bool a);
	virtual void setLastError(string strError);
	virtual void setQuery(const char* query);
	virtual void setSelect(bool s);

	// prepared query support
	virtual bool exec();
	virtual bool prepare(const string& query);

	virtual bool IsFieldNull(int i) = 0;
	virtual bool reset(const char* sqlquery, short execFormate=0) = 0;
	virtual bool fetch(int i) = 0;

	virtual bool fetchFirst() = 0;
	virtual bool fetchLast() = 0;
	virtual int size() = 0;


	//取字段个数
	virtual int GetFieldNum()							= 0;
	//取字段名/字段编号
	virtual string GetFieldName(long lIndex)			= 0;	

	virtual int GetIndex(const char* strFieldName)		= 0;
	virtual int SelectRowCount()						= 0;

	virtual int GetFieldType(const char*szFldName)		= 0;
	virtual int GetFieldType(int nFldIndex)				= 0;

	virtual void MoveFirst()							= 0;
	virtual void MoveNext()								= 0;
	virtual void MovePrevious()							= 0;
	virtual void MoveLast()								= 0;

	virtual bool IsEOF()								= 0;
	virtual bool IsBOF()								= 0;

	virtual bool IsNull()								= 0;
	virtual bool IsEmpty()								= 0;
	
	//获取数据实际长度
	virtual int GetValueSize(char* lpszFieldName)		= 0;
	virtual int GetValueSize(long lIndex)				= 0;
	//取字段定义长度
	virtual int GetFieldDefineSize(int lIndex)			= 0;
	virtual int GetFieldDefineSize(const char* lpszFieldName)	= 0;

	//获取数据

	virtual bool GetValue(int nColumns, bool& bValue, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, bool& bValue, bool &nIsNull ) = 0;

	virtual bool GetValue(int nColumns, byte& nValue, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, byte& nValue, bool &nIsNull ) = 0;

	virtual bool GetValue(int nColumns, short& nValue, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, short& nValue, bool &nIsNull ) = 0;

	virtual bool GetValue(int nColumns, int& nValue, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, int& nValue, bool &nIsNull ) = 0;

	virtual bool GetValue(int nColumns, long& lValue, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, long& lValue, bool &nIsNull ) = 0;

	virtual bool GetValue(int nColumns, __int64& lValue, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, __int64& lValue, bool &nIsNull ) = 0;

	virtual bool GetValue(int nColumns, float& fValue, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, float& fValue, bool &nIsNull) = 0;

	virtual bool GetValue(int nColumns, double& lValue, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, double& lValue, bool &nIsNull ) = 0;

	virtual bool GetValue(int nColumns, tm& time, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, tm& time, bool &nIsNull ) = 0;

	virtual bool GetValue(int nColumns, char* strValue, short len,bool &nIsNull) = 0;
	virtual bool GetValue(char* strFldName, char* strValue, short len, bool &nIsNull) = 0;

	virtual bool GetValue(int nColumns, string& strValue, bool &nIsNull) = 0;
	virtual bool GetValue(char* strFieldName, string& strValue, bool &nIsNull) = 0;

	virtual bool GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull ) = 0;
	virtual bool GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull ) = 0;

private:
	CSqlRecordsetPrivate* d;
};
