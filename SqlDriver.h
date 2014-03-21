#ifndef _SQL_DRIVER_H
#define _SQL_DRIVER_H

#include "SqlDef.h"
#include <string>
#include <time.h>
using namespace std;

class CSqlDatabase;
class CSqlDriverPrivate;
class CSqlRecordset;


//通用数据库连接类
class CSqlDriver
{
	friend class QSqlDatabase;
public:
	enum DriverFeature { Transactions, QuerySize, BLOB, Unicode, PreparedQueries,
		NamedPlaceholders, PositionalPlaceholders, LastInsertId,
		BatchOperations, SimpleLocking, LowPrecisionNumbers,
		EventNotifications, FinishQuery, MultipleResultSets, CancelQuery };

public:
	CSqlDriver(void);
	virtual ~CSqlDriver(void);

public:
	virtual bool open(const char* db, const char* user = NULL, const char* password = NULL,
					 const char* host = NULL, int port = -1, const char* connOpts = NULL) = 0;


	virtual void close() = 0;
	virtual bool hasFeature(DriverFeature f) const = 0;

	virtual CSqlRecordset *createResult() const = 0;

	virtual bool IsOpen() const;
	bool isOpenError() const;

	virtual int ExecuteSql(const char *szSql) = 0;
	virtual int ExecBinary(const char* strSql, char** pParas,int *len, short nParamsNum) = 0;

	virtual bool BeginTransaction()				= 0;
	virtual bool CommitTransaction()			= 0;
	virtual bool RollbackTransaction()			= 0;


	virtual string GetLastError();
	virtual void SetLastError(string &szError);

	virtual short GetPoolFlag();
	virtual short SetPoolFlag(short poolflag);

protected:
	virtual void setOpen(bool o);
	virtual void setOpenError(bool e);
	//virtual void setLastError(const QSqlError& e);
	string m_strError;				// 错误信息
private:
	short m_poolflag;	//0-未 1-放入Pool
	DB_TYPE m_dbType;

	bool m_bIsOpen;
	bool m_bIsOpenError;

};

#endif