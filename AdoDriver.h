#pragma once

#include "SqlDriver.h"
#include "SqlRecordset.h"

class CAdoSQLDriverPrivate;
class CAdoSQLResultPrivate;
class CAdoSQLDriver;

class CAdoSQLResult: public CSqlRecordset
{
	friend class CAdoSQLResultPrivate;
public:
	CAdoSQLResult(const CAdoSQLDriver* db, const CAdoSQLDriverPrivate* p);
	~CAdoSQLResult();

	//void virtual_hook(int id, void *data);

protected:
	void cleanup();
	bool fetch(int i);
	bool fetchFirst();
	bool fetchLast();

	bool IsFieldNull(int field);
	bool reset(const char* query, short execFormate=0);
	int size();
	int SelectRowCount();
	long lastInsertId() const;
	bool prepare(const string& query);
	bool exec();

	//取字段个数
	int GetFieldNum();
	//取字段名/字段编号
	string GetFieldName(long lIndex);		
	int GetIndex(const char* strFieldName);		

	int GetFieldType(const char* lpszFieldName);
	int GetFieldType(int lIndex);

	void MoveFirst();						
	void MoveNext();		
	void MovePrevious();	
	void MoveLast();

	bool IsEOF();
	bool IsBOF();

	bool IsNull();
	bool IsEmpty();

	//获取数据实际长度
	int GetValueSize(long lIndex);				
	int GetValueSize(char* lpszFieldName);	
	//取字段定义长度
	int GetFieldDefineSize(int lIndex);
	int GetFieldDefineSize(const char* lpszFieldName);

	bool GetValue(int nColumns, _variant_t& vt, bool &nIsNull );
	bool GetValue(char* strFieldName, _variant_t& vt, bool &nIsNull);

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
	byte *m_blobData;
	CAdoSQLResultPrivate *d;
};

class CAdoSQLDriver : public CSqlDriver
{
public:
	CAdoSQLDriver(void);
	CAdoSQLDriver(_ConnectionPtr conn);
	~CAdoSQLDriver(void);

public:
	bool open(const char* db, const char* user, const char* password,
		const char* host, int port, const char* connOpts);

	bool IsOpen() const;
	void close();

	virtual bool hasFeature(DriverFeature f) const
	{
		return true;
	}

	bool BeginTransaction();
	bool CommitTransaction();
	bool RollbackTransaction();

	int ExecuteSql(const char *szSql);
	int ExecBinary(const char* strSql, byte** pParas,int *len, short nParamsNum);
	bool PutBlob(const char *strWhereSql, const char *szTableName, const char* szFldName, byte*szVal, long len);
	//Protocol protocol() const;
	CSqlRecordset *createResult() const;

private:
	void init();
	CAdoSQLDriverPrivate *ado_d;
};