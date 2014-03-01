#pragma once

#include <vector>
#include "SqlDef.h"
using namespace std;

#ifdef _AFX_COMMON_DBLIB_
	#define COMMON_DBLIB __declspec(dllexport)
#else
	#define COMMON_DBLIB __declspec(dllimport)
	#ifdef _DEBUG
		#pragma comment(lib, "DbDriverD.lib")
	#else
		#pragma comment(lib, "DbDriver.lib")
	#endif
#endif


class CSqlDriver;
class CSqlDatabasePrivate;
class CSqlQueryPrivate;
class CSqlRecordset;

class COMMON_DBLIB CSqlDataBase
{
public:
	CSqlDataBase();
	CSqlDataBase(const CSqlDataBase &other);
	~CSqlDataBase();

	CSqlDataBase &operator=(const CSqlDataBase &other);

	// 连接数据库
	bool open();
	bool open(const char* user, const char* password);
	
	void close();
	bool IsOpen() const;
	bool IsOpenError() const;
	
	// 判断driver是否为空
	bool isValid() const;

	// 设置连接信息(一起设置所有的信息)
	void setConnctInfo(const char*  dbname, const char* host = NULL, const char* user_name = NULL, 
		const char*  password = NULL, int p = -1);

	// 单独设置所有连接信息
	void setDatabaseName(const char* name);
	void setUserName(const char* name);
	void setPassword(const char*  password);
	void setHostName(const char*  host);
	void setPort(int p);

	// 获取连接信息
	void databaseName(char *szDb) const;
	void userName(char *user) const;
	void password(char *psw) const;
	void hostName(char *host) const;
	int  port() const;

	// 事务
	bool BeginTransaction();
	bool CommitTransaction();
	bool RollbackTransaction();

	// 获取driver指针(内部使用，一般不会使用)
	void* driver() const;

	// 执行非查询的SQL语句
	// 返回值: -1-连接有问题,0-执行失败,1-执行成功
	int ExecuteSql(const char *szSql);

	// 批量执行sql语句(使用事物)
	int ExecuteSql(char**& strSql,long len);

	// 二进制参数绑定的方式执行Sql语句执行函数
	// nParamsNum:参数个数; pParas参数值
	// 
	int ExecBinary(const char* strSql, byte** pParas,int *len, int nParamsNum);

	// 主要是为了插入二进制数据(暂时没有实现，后期实现)
	bool PutBlob(const char *strWhereSql, const char *szTableName, const char* szFldName, byte*szVal, long len);

	// 新建一个连接,参数为数据库类型
	static CSqlDataBase* addDatabase(short type);
	static CSqlDataBase addDatabase(void* driver, const char* connectionName);
	
	// 释放连接
	static void freeDatabase(CSqlDataBase *&pDb);

	// 取错误信息
	void GetLastError(char *szError);
public:
	void GetDbName(char *szDbName, short len);
	short GetDbType();

	// 连接池使用
	virtual short GetPoolFlag();
	virtual short SetPoolFlag(short poolflag);

protected:
	explicit CSqlDataBase(short type);
	explicit CSqlDataBase(void* driver);
	inline void qAtomicAssign(CSqlDatabasePrivate *&d, CSqlDatabasePrivate *x);

private:
	friend class CSqlDatabasePrivate;
	CSqlDatabasePrivate *d;

	short m_poolflag;	//连接池使用， 0-未 1-放入Pool
	short m_dbType;
};


class COMMON_DBLIB CSqlQuery
{
public:
	CSqlQuery(){d = NULL;}
	explicit CSqlQuery(const char* query, CSqlDataBase *db);
	explicit CSqlQuery(CSqlDataBase *db);
	CSqlQuery(const CSqlQuery& other);
	CSqlQuery& operator=(const CSqlQuery& other);
	~CSqlQuery();

	// 关闭查询对象中的数据(一般不用调用，直接等待对象调用析构函数,主要是针对Sqlite不能共享必须关闭的问题)
	bool Close();

	// 判断结果集是否为空,不为空-true
	bool isValid() const;
	// 查询是否成功,成功-true
	bool isActive() const;
	// 是否查询成功
	bool isSelect() const;

	// 获取查询的SQL语句
	string lastQuery() const;

	// 获取查询结果集的行数
	int GetRowNum() const;

	// 查询函数
	bool Query(const char* query);
	bool QueryBinary(const char* query);
	bool Query();
		
	//　清空结果集
	void clear();

	// 判断某个字段数据是否为空
	bool isFieldNull(int field) const;

	// 获取字段定义长度
	int GetFieldDefineSize(const char*szFldName);
	int GetFieldDefineSize(int nFldIndex);

	// 获取字段数目
	int GetFieldNum();

	// 根据字段index获取name
	string GetFieldName(int i);

	// 获取字段类型
	int GetFieldType(const char*szFldName);
	int GetFieldType(int nFldIndex);

	// 移动行光标
	void MoveFirst();					
	void MoveNext() ;			
	void MovePrevious();
	void MoveLast();

	// 行光标是否在末尾/开始
	bool IsEOF();
	bool IsBOF();

	// 查询结果集为空/NULL
	bool IsEmpty();
	bool IsNull();

	// 取数据
	bool GetValue(int nColumns, bool& bValue, bool &nIsNull );
	bool GetValue(char* strFieldName, bool& bValue, bool &nIsNull );

	bool GetValue(int nColumns, byte& nValue, bool &nIsNull );
	bool GetValue(char* strFieldName, byte& nValue, bool &nIsNull );

	bool GetValue(int nColumns, short& nValue, bool &nIsNull );
	bool GetValue(char* strFieldName, short& nValue, bool &nIsNull );

	bool GetValue(int nColumns, int& nValue, bool &nIsNull );
	bool GetValue(char* strFieldName, int& nValue, bool &nIsNull );

	bool GetValue(int nColumns, long& lValue, bool &nIsNull );
	bool GetValue(char* strFieldName, long& lValue, bool &nIsNull );

	bool GetValue(int nColumns, __int64& lValue, bool &nIsNull );
	bool GetValue(char* strFieldName, __int64& lValue, bool &nIsNull );

	bool GetValue(int nColumns, float& fValue, bool &nIsNull );
	bool GetValue(char* strFieldName, float& fValue, bool &nIsNull);

	bool GetValue(int nColumns, double& lValue, bool &nIsNull );
	bool GetValue(char* strFieldName, double& lValue, bool &nIsNull );

	bool GetValue(int nColumns, tm& time, bool &nIsNull );
	bool GetValue(char* strFieldName, tm& time, bool &nIsNull );

	bool GetValue(int nColumns, char* strValue, short len,bool &nIsNull);
	bool GetValue(char* strFldName, char* strValue, short len, bool &nIsNull);

	bool GetValue(int nColumns, string& strValue, bool &nIsNull);
	bool GetValue(char* strFieldName, string& strValue, bool &nIsNull);

	bool GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull );
	bool GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull );

private:
	friend void qInit(CSqlQuery *q, const char* query, CSqlDataBase *db);
	CSqlQuery(CSqlRecordset *r);
	const CSqlDriver* driver() const;


	inline void qAtomicAssign(CSqlQueryPrivate *&d, CSqlQueryPrivate *x);
	CSqlQueryPrivate* d;
};
