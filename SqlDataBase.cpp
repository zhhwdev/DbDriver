#include "StdAfx.h"
#include "SqlDataBase.h"
#include "SqlDriver.h"
#include "PgDriver.h"
#include "AdoDriver.h"
#include "SqliteDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CSqlDatabasePrivate
{
public:
	CSqlDatabasePrivate(CSqlDriver *dr = NULL):
	  driver(dr),
	  m_nPort(-1),
	  refCount(1)
	  {
		  m_szDbName[0] = '\0';
		  m_szUserName[0] = '\0';
		  m_szPwd[0] = '\0';
		  m_szHost[0] = '\0';
	  }
	  CSqlDatabasePrivate(const CSqlDatabasePrivate &other);
	  ~CSqlDatabasePrivate();
	  void init(short type);
	  void copy(const CSqlDatabasePrivate *other);
	  void disable();

	  CSqlDriver* driver;

	  char m_szDbName[32];
	  char m_szUserName[32];
	  char m_szPwd[32];
	  char m_szHost[32];
	  int m_nPort;
	  char connOptions[32];

	  size_t refCount;		// 引用计数

	  static CSqlDatabasePrivate *shared_null();
};

CSqlDatabasePrivate::CSqlDatabasePrivate(const CSqlDatabasePrivate &other)
{
	lstrcpy(m_szDbName, other.m_szDbName);
	lstrcpy(m_szUserName, other.m_szUserName);
	lstrcpy(m_szPwd, other.m_szPwd);
	lstrcpy(m_szHost, other.m_szHost);
	m_nPort = other.m_nPort;
	driver = other.driver;
}

CSqlDatabasePrivate::~CSqlDatabasePrivate()
{
	if (driver)
		delete driver;
	driver = NULL;
}
void CSqlDatabasePrivate::init(short type)
{
	switch (type)
	{
	case DB_POSTGRESQL:
		driver = new CPgSQLDriver();
		break;
	case DB_SQLITE:
		driver = new CSqliteSQLDriver();
		break;
	case DB_MSSQL:
		driver = new CAdoSQLDriver();
		break;
	case DB_ORACLE:
		driver = NULL;
		break;
	default:
		break;
	}

}
void CSqlDatabasePrivate::copy(const CSqlDatabasePrivate *other)
{

}
void CSqlDatabasePrivate::disable()
{

}

//////////////////////////////////CDataBase////////////////////////////////////////
inline void CSqlDataBase::qAtomicAssign(CSqlDatabasePrivate *&d, CSqlDatabasePrivate *x)
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

CSqlDataBase::CSqlDataBase()
{
	d = NULL;
}
CSqlDataBase::CSqlDataBase(const CSqlDataBase &other)
{
	m_dbType = other.m_dbType;
	m_poolflag = other.m_poolflag;
	d = other.d;
	d->refCount++;
}

CSqlDataBase::~CSqlDataBase()
{
	d->refCount--;
	if (d->refCount == 0) {
		close();
		delete d;
		d = NULL;
	}
}

CSqlDataBase &CSqlDataBase::operator=(const CSqlDataBase &other)
{
	m_dbType = m_dbType;
	m_poolflag = other.m_poolflag;
	qAtomicAssign(d, other.d);
	return *this;
}

bool CSqlDataBase::open()
{
	return d->driver->open(d->m_szDbName, d->m_szUserName, d->m_szPwd, d->m_szHost,
		d->m_nPort, d->connOptions);
}

bool CSqlDataBase::open(const char* user, const char* password)
{
	setUserName(user);
	return d->driver->open(d->m_szDbName, user, password, d->m_szHost,
		d->m_nPort, d->connOptions);
}

void CSqlDataBase::close()
{
	if (!d || !d->driver)
		return;

 	d->driver->close();
}

bool CSqlDataBase::IsOpen() const
{
	if (!d || !d->driver)
		return false;

	return d->driver->IsOpen();
}

bool CSqlDataBase::IsOpenError() const
{
	return false;
}

bool CSqlDataBase::isValid() const
{
	return d->driver != NULL;/*&& d->driver != d->shared_null()->driver->IsOpen();*/
}

void CSqlDataBase::setConnctInfo(const char*  dbname, const char* host, const char* user_name, 				   const char*  password, int p)
{
	if (isValid())
	{
		if (host)
			lstrcpy(d->m_szHost, host);
		if (dbname)
			lstrcpy(d->m_szDbName, dbname);
		if (user_name)		
			lstrcpy(d->m_szUserName, user_name);
		if (password)
			lstrcpy(d->m_szPwd, password);
		d->m_nPort = p;
	}
}

void CSqlDataBase::setDatabaseName(const char* name)
{
	if (isValid() && name)
		lstrcpy(d->m_szDbName, name);
}

void CSqlDataBase::setUserName(const char* name)
{
	if (isValid() && name)
		lstrcpy(d->m_szUserName, name);
}

void CSqlDataBase::setPassword(const char*  password)
{
	if (isValid() && password)
		lstrcpy(d->m_szPwd, password);
}
void CSqlDataBase::setHostName(const char*  host)
{
	if (isValid() && host)
		lstrcpy(d->m_szHost, host);
}
void CSqlDataBase::setPort(int p)
{
	if (isValid())
		d->m_nPort = p;
}
void CSqlDataBase::databaseName(char *szDb) const
{
	lstrcpy(szDb, d->m_szDbName);
}
void CSqlDataBase::userName(char *user) const
{
	lstrcpy(user, d->m_szUserName);
}
void CSqlDataBase::password(char *psw) const
{
	lstrcpy(psw, d->m_szPwd);
}
void CSqlDataBase::hostName(char *host) const
{
	lstrcpy(host, d->m_szHost);
}
int CSqlDataBase::port() const
{
	return d->m_nPort;
}

bool CSqlDataBase::BeginTransaction()
{
	if (d->driver==NULL || !d->driver->IsOpen())
		return false;

	return d->driver->BeginTransaction();
}

bool CSqlDataBase::CommitTransaction()
{
	if (d->driver==NULL || !d->driver->IsOpen())
		return false;

	return d->driver->CommitTransaction();
}

bool CSqlDataBase::RollbackTransaction()
{
	if (d->driver==NULL || !d->driver->IsOpen())
		return false;

	return d->driver->RollbackTransaction();
}

void* CSqlDataBase::driver() const
{
	return (void*)(d->driver);
}

int CSqlDataBase::ExecuteSql(const char *szSql)
{
	if (d->driver==NULL || !d->driver->IsOpen())
		return -1;

	return d->driver->ExecuteSql(szSql);
}

int CSqlDataBase::ExecuteSql(char** &strSql, long len)
{
	if (d->driver==NULL || !d->driver->IsOpen())
		return -1;

	if (!BeginTransaction())
		return 0;

	for (int i=0; i<len; i++)
	{
		if (ExecuteSql(strSql[i]) < 1)
		{
			RollbackTransaction();
			return 0;
		}
	}

	if (!CommitTransaction())
	{
		RollbackTransaction();
		return 0;
	}

	return 1;
}

int CSqlDataBase::ExecBinary(const char* strSql, char** pParas,int *len, int nParamsNum)
{
	if (!d->driver || !d->driver->IsOpen())
		return -1;

	d->driver->ExecBinary(strSql, pParas, len, nParamsNum);
	return 1;
}

CSqlDataBase* CSqlDataBase::addDatabase(short type)
{
	CSqlDataBase *db = new CSqlDataBase(type);
	return db;
}

CSqlDataBase CSqlDataBase::addDatabase(void* driver, const char* connectionName)
{
	CSqlDataBase db(driver);
	return db;
}

void CSqlDataBase::freeDatabase(CSqlDataBase *&pDb)
{
	if (pDb)
		delete pDb;
	pDb = NULL;
}

CSqlDataBase::CSqlDataBase(short type)
{
	m_poolflag = 0;
	d = new CSqlDatabasePrivate();
	d->init(type);
	m_dbType = type;
}
CSqlDataBase::CSqlDataBase(void* driver)
{
    d = new CSqlDatabasePrivate((CSqlDriver*)driver);
}
void CSqlDataBase::GetDbName(char *szDbName, short len)
{
	if(strlen(d->m_szDbName) > 0)
		strcpy_s(szDbName, min(32, len), d->m_szDbName);
}
short CSqlDataBase::GetDbType()
{
	return m_dbType;
}
short CSqlDataBase::GetPoolFlag()
{
	return m_poolflag;
}
short CSqlDataBase::SetPoolFlag(short poolflag)
{
	short rtn=m_poolflag;
	m_poolflag=poolflag;
	return rtn;
}
