#pragma once

#include "SqlDriver.h"
#include "SqlRecordset.h"

class CPgSQLDriverPrivate;
class CPgSQLResultPrivate;
class CPgSQLDriver;


class CPgSQLResult : public CSqlRecordset
{
	friend class CPgSQLResultPrivate;
public:
	CPgSQLResult(const CPgSQLDriver* db, const CPgSQLDriverPrivate* p);
	~CPgSQLResult();

	//void virtual_hook(int id, void *data);

protected:
	void cleanup();
	bool fetch(int i);
	bool fetchFirst();
	bool fetchLast();

	bool IsFieldNull(int field);
	bool reset(const char* query, short execFormate = 0);
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

	int GetFieldType(int lIndex);
	int GetFieldType(const char *szFldName);

	void MoveFirst() {m_lCurrRow = 0;}							
	void MoveNext() {m_lCurrRow++;}			
	void MovePrevious(){m_lCurrRow--;}					
	void MoveLast(){}	

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
	long m_lCurrRow;
	CPgSQLResultPrivate *d;
};

class  CPgSQLDriver:public CSqlDriver
{
public:
	enum Protocol {
		VersionUnknown = -1,
		Version8 = 1,
		Version81 = 2,
		Version82 = 3,
		Version83 = 4,
		Version84 = 5,
		Version9 = 6,
	};

public:
	CPgSQLDriver(void);
	CPgSQLDriver(PGconn *conn);
	~CPgSQLDriver(void);
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

	// 执行带二进制数据操作
	int ExecBinary(const char *szSql, byte **szBinVal, int *lLen, short nNum);

	Protocol protocol() const;
	CSqlRecordset *createResult() const;

	void SetError(char *szError);
private:
	void init();
	CPgSQLDriverPrivate *pg_d;
};
