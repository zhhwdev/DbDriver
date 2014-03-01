#include "StdAfx.h"
#include "PgDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SWAP16PTR(x) \
{                                                                 \
	byte       byTemp, *_pabyDataT = (byte *) (x);              \
	\
	byTemp = _pabyDataT[0];                                       \
	_pabyDataT[0] = _pabyDataT[1];                                \
	_pabyDataT[1] = byTemp;                                       \
}                                                                    

#define SWAP32PTR(x) \
{                                                                 \
	byte       byTemp, *_pabyDataT = (byte *) (x);              \
	\
	byTemp = _pabyDataT[0];                                       \
	_pabyDataT[0] = _pabyDataT[3];                                \
	_pabyDataT[3] = byTemp;                                       \
	byTemp = _pabyDataT[1];                                       \
	_pabyDataT[1] = _pabyDataT[2];                                \
	_pabyDataT[2] = byTemp;                                       \
}                                                                    

#define SWAP64PTR(x) \
{                                                                 \
	byte    byTemp, *_pabyDataT = (byte *) (x);              \
	\
	byTemp = _pabyDataT[0];                                       \
	_pabyDataT[0] = _pabyDataT[7];                                \
	_pabyDataT[7] = byTemp;                                       \
	byTemp = _pabyDataT[1];                                       \
	_pabyDataT[1] = _pabyDataT[6];                                \
	_pabyDataT[6] = byTemp;                                       \
	byTemp = _pabyDataT[2];                                       \
	_pabyDataT[2] = _pabyDataT[5];                                \
	_pabyDataT[5] = byTemp;                                       \
	byTemp = _pabyDataT[3];                                       \
	_pabyDataT[3] = _pabyDataT[4];                                \
	_pabyDataT[4] = byTemp;                                       \
}                                                                    

#define SWAPDOUBLE(p) SWAP64PTR(p)

void  array_reverse(byte* arr,int num)
{
	for(int i=0;i<num/2;i++)
	{
		char c=arr[i];
		arr[i]=arr[num-1-i];
		arr[num-1-i]=c;		
	}
}

#define POSTGRES_EPOCH_JDATE	2451545 /* == date2j(2000, 1, 1) */

static void PGj2date(int jd, int *year, int *month, int *day)
{
	unsigned int julian;
	unsigned int quad;
	unsigned int extra;
	int			y;

	julian = jd;
	julian += 32044;
	quad = julian / 146097;
	extra = (julian - quad * 146097) * 4 + 3;
	julian += 60 + quad * 3 + extra / 146097;
	quad = julian / 1461;
	julian -= quad * 1461;
	y = julian * 4 / 1461;
	julian = ((y != 0) ? ((julian + 305) % 365) : ((julian + 306) % 366))
		+ 123;
	y += quad * 4;
	*year = y - 4800;
	quad = julian * 2141 / 65536;
	*day = julian - 7834 * quad / 256;
	*month = (quad + 10) % 12 + 1;

	return;
}	/* j2date() */


#define USECS_PER_SEC	1000000
#define USECS_PER_MIN   ((__int64) 60 * USECS_PER_SEC)
#define USECS_PER_HOUR  ((__int64) 3600 * USECS_PER_SEC)
#define USECS_PER_DAY   ((__int64) 3600 * 24 * USECS_PER_SEC)

static void PGdt2timeInt8(__int64 jd, int *hour, int *min, int *sec, double *fsec)
{
	__int64		time;

	time = jd;

	*hour = (int) (time / USECS_PER_HOUR);
	time -= (__int64) (*hour) * USECS_PER_HOUR;
	*min = (int) (time / USECS_PER_MIN);
	time -=  (__int64) (*min) * USECS_PER_MIN;
	*sec = (int)time / USECS_PER_SEC;
	*fsec = (double)(time - *sec * USECS_PER_SEC);
}	/* dt2time() */

#define TMODULO(t,q,u) \
do { \
	(q) = ((t) / (u)); \
	if ((q) != 0) (t) -= ((q) * (u)); \
} while(0)

/* Coming from timestamp2tm() in pgsql/src/backend/utils/adt/timestamp.c */

static int PGTimeStamp2DMYHMS(__int64 dt, int *year, int *month, int *day,
							  int* hour, int* min, int* sec)
{
	__int64 date;
	__int64 time;
	double fsec;

	time = dt;
	TMODULO(time, date, USECS_PER_DAY);

	if (time < 0)
	{
		time += USECS_PER_DAY;
		date -= 1;
	}

	/* add offset to go from J2000 back to standard Julian date */
	date += POSTGRES_EPOCH_JDATE;

	/* Julian day routine does not work for negative Julian days */
	if (date < 0 || date > (double) INT_MAX)
		return -1;

	PGj2date((int) date, year, month, day);
	PGdt2timeInt8(time, hour, min, sec, &fsec);

	return 0;
}

typedef short NumericDigit;

typedef struct NumericVar
{
        int			ndigits;		/* # of digits in digits[] - can be 0! */
        int			weight;			/* weight of first digit */
        int			sign;			/* NUMERIC_POS, NUMERIC_NEG, or NUMERIC_NAN */
        int			dscale;			/* display scale */
        NumericDigit *digits;		/* base-NBASE digits */
} NumericVar;

#define NUMERIC_POS			0x0000
#define NUMERIC_NEG			0x4000
#define NUMERIC_NAN			0xC000

#define DEC_DIGITS	4

static char * PGGetStrFromBinaryNumeric(NumericVar *var)
{
        char	   *str;
        char	   *cp;
        char	   *endcp;
        int			i;
        int			d;
        NumericDigit dig;
        NumericDigit d1;
        
        int dscale = var->dscale;
        i = (var->weight + 1) * DEC_DIGITS;
        if (i <= 0)
                i = 1;

        str = new char[i + dscale + DEC_DIGITS + 2];
        cp = str;

        if (var->sign == NUMERIC_NEG)
                *cp++ = '-';

        if (var->weight < 0)
        {
                d = var->weight + 1;
                *cp++ = '0';
        }
        else
        {
                for (d = 0; d <= var->weight; d++)
                {
                        dig = (d < var->ndigits) ? var->digits[d] : 0;
                        SWAP16PTR(&dig);
                        {
                                bool		putit = (d > 0);

                                d1 = dig / 1000;
                                dig -= d1 * 1000;
                                putit |= (d1 > 0);
                                if (putit)
                                        *cp++ = (char)(d1 + '0');
                                d1 = dig / 100;
                                dig -= d1 * 100;
                                putit |= (d1 > 0);
                                if (putit)
                                        *cp++ = (char)(d1 + '0');
                                d1 = dig / 10;
                                dig -= d1 * 10;
                                putit |= (d1 > 0);
                                if (putit)
                                        *cp++ = (char)(d1 + '0');
                                *cp++ = (char)(dig + '0');
                        }
                }
        }
        
        if (dscale > 0)
        {
                *cp++ = '.';
                endcp = cp + dscale;
                for (i = 0; i < dscale; d++, i += DEC_DIGITS)
                {
                        dig = (d >= 0 && d < var->ndigits) ? var->digits[d] : 0;
                        SWAP16PTR(&dig);
                        d1 = dig / 1000;
                        dig -= d1 * 1000;
                        *cp++ = (char)(d1 + '0');
                        d1 = dig / 100;
                        dig -= d1 * 100;
                        *cp++ = (char)(d1 + '0');
                        d1 = dig / 10;
                        dig -= d1 * 10;
                        *cp++ = (char)(d1 + '0');
                        *cp++ = (char)(dig + '0');
                }
                cp = endcp;
        }

        *cp = '\0';
        return str;
}

class CPgSQLDriverPrivate
{
public:
	CPgSQLDriverPrivate():connection(NULL), pro(CPgSQLDriver::Version9) {}
	PGconn *connection;
	//bool isUtf8;
	CPgSQLDriver::Protocol pro;

	//void appendTables(QStringList &tl, QSqlQuery &t, QChar type);
	PGresult* exec(const char* stmt) const;
	PGresult* execForBinary(const char* stmt) const;
	CPgSQLDriver::Protocol getPSQLVersion();
	//bool setEncodingUtf8();
	void setDatestyle();
};

PGresult* CPgSQLDriverPrivate::exec(const char * stmt) const
{
	PGresult *result = PQexec(connection, stmt);

	return result;
}

PGresult* CPgSQLDriverPrivate::execForBinary(const char* stmt) const
{
	PGresult* result = PQexecParams(connection, stmt, 0, NULL, NULL, NULL, NULL, 1);
	return result;
}

void CPgSQLDriverPrivate::setDatestyle()
{
	PGresult* result = exec("SET DATESTYLE TO 'ISO'");
	int status =  PQresultStatus(result);
	if (status != PGRES_COMMAND_OK)
		TRACE("%s", PQerrorMessage(connection));
	PQclear(result);
}



class CPgSQLResultPrivate
{
public:
	CPgSQLResultPrivate(CPgSQLResult *qq): q(qq), driver(NULL), result(NULL), currentSize(-1), preparedQueriesEnabled(false) {}

	CPgSQLResult *q;
	const CPgSQLDriverPrivate *driver;
	PGresult *result;
	int currentSize;
	bool preparedQueriesEnabled;
	string preparedStmtId;

	bool processResults();
};

bool CPgSQLResultPrivate::processResults()
{
	if (!result)
		return false;

	ExecStatusType status = PQresultStatus(result);
	if (status == PGRES_TUPLES_OK) 
	{
		q->setSelect(true);
		q->setActive(true);
		currentSize = PQntuples(result);
		return true;
	} 
	else if (status == PGRES_COMMAND_OK) 
	{
		q->setSelect(false);
		q->setActive(true);
		currentSize = -1;
		return true;
	}
	else
		q->setLastError(PQerrorMessage(driver->connection));

	return false;
}

CPgSQLDriver::Protocol CPgSQLDriverPrivate::getPSQLVersion()
{
	CPgSQLDriver::Protocol serverVersion = CPgSQLDriver::Version9;
	PGresult* result = exec("select version()");
	int status = PQresultStatus(result);
 	if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) 
	{
 		string val = PQgetvalue(result, 0, 0);
		int pos1 = (int)val.find_first_of(',');
		string val1 = val.substr(0, pos1);
		int pos2 = (int)val1.find_first_of(' ');
		string val2 = val1.substr(pos2, pos1);
		int pos3 = (int)val2.find_first_of('.');
		string Ver1 = val2.substr(0, pos3);
		int pos4 = (int)val2.find_last_of('.');
		string Ver2 = val2.substr(pos3+1, pos4-pos3-1);

		switch (atoi(Ver1.c_str()))
		{
		case 9:
			serverVersion = CPgSQLDriver::Version9;
			break;
		case 8:
			{
				switch (atoi(Ver2.c_str()))
				{
				case 0:
					serverVersion = CPgSQLDriver::Version8;
					break;
				case 1:
					serverVersion = CPgSQLDriver::Version81;
					break;
				case 2:
					serverVersion = CPgSQLDriver::Version82;
					break;
				case 3:
					serverVersion = CPgSQLDriver::Version83;
					break;
				case 4:
					serverVersion = CPgSQLDriver::Version84;
					break;
				default:
					serverVersion = CPgSQLDriver::VersionUnknown;
					break;
				}
				break;
			}
		default:
			serverVersion = CPgSQLDriver::VersionUnknown;
			break;
		}
 	}
	PQclear(result);

	//keep the old behavior unchanged
	if (serverVersion == CPgSQLDriver::VersionUnknown)
		serverVersion = CPgSQLDriver::Version9;

	if (serverVersion < CPgSQLDriver::Version8) {
		TRACE("This version of PostgreSQL is not supported and may not work.");
	}

	return serverVersion;
}

static void qDeallocatePreparedStmt(CPgSQLResultPrivate *d)
{
	string strStmt = "DEALLOCATE " + d->preparedStmtId;
	PGresult *result = d->driver->exec(strStmt.c_str());

	if (PQresultStatus(result) != PGRES_COMMAND_OK)
		TRACE("Unable to free statement: %s", PQerrorMessage(d->driver->connection));

	PQclear(result);
	d->preparedStmtId.clear();
}

CPgSQLResult::CPgSQLResult(const CPgSQLDriver* db, const CPgSQLDriverPrivate* p)
: CSqlRecordset(db)
{
	d = new CPgSQLResultPrivate(this);
	d->driver = p;
	m_lCurrRow = 0;
	//d->preparedQueriesEnabled = db->hasFeature(QSqlDriver::PreparedQueries);
}

CPgSQLResult::~CPgSQLResult()
{
	cleanup();

	if (d->preparedQueriesEnabled && !d->preparedStmtId.empty())
		qDeallocatePreparedStmt(d);

	if (d)
		delete d;
	d = NULL;
}

void CPgSQLResult::cleanup()
{
	if (d->result)
		PQclear(d->result);
	d->result = NULL;
	//setAt(QSql::BeforeFirstRow);
	d->currentSize = -1;
	setActive(false);
	m_lCurrRow = 0;
}

bool CPgSQLResult::fetch(int i)
{
	if (!isActive())
		return false;
	if (i < 0)
		return false;
	if (i >= d->currentSize)
		return false;
// 	if (at() == i)
// 		return true;
	//setAt(i);
	return true;
}

bool CPgSQLResult::fetchFirst()
{
	return fetch(0);
}

bool CPgSQLResult::fetchLast()
{
	return fetch(PQntuples(d->result) - 1);
}

bool CPgSQLResult::IsFieldNull(int field)
{
	PQgetvalue(d->result, 1/*at()*/, field);
	return PQgetisnull(d->result, 1/*at()*/, field) == 1;
}

bool CPgSQLResult::reset(const char* query, short execFormate)
{
	cleanup();
	if (!driver())
		return false;
	if (!(driver()->IsOpen()))
		return false;
	
	if (execFormate == 0)
		d->result = d->driver->exec(query);
	else
		d->result = d->driver->execForBinary(query);
	return d->processResults();
}

int CPgSQLResult::size()
{
	return d->currentSize;
}
int CPgSQLResult::SelectRowCount()
{
	return atoi(PQcmdTuples(d->result));
}

long CPgSQLResult::lastInsertId() const
{
	if (isActive()) {
		Oid id = PQoidValue(d->result);
		if (id != InvalidOid)
			return long(id);
	}
	return -1;
}

bool CPgSQLResult::prepare(const string &query)
{
	if (!d->preparedQueriesEnabled)
		return CSqlRecordset::prepare(query);

	cleanup();

// 	if (!d->preparedStmtId.empty())
// 		qDeallocatePreparedStmt(d);
// 
//  	const string stmtId = qMakePreparedStmtId();
// // 	const string stmt = fromLatin1("PREPARE %1 AS ").arg(stmtId).append(qReplacePlaceholderMarkers(query));
// 
// 	PGresult *result = d->driver->exec(stmt);
// 
// 	if (PQresultStatus(result) != PGRES_COMMAND_OK) {
// // 		setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
// // 			"Unable to prepare statement"), QSqlError::StatementError, d->driver, result));
// 		PQclear(result);
// 		d->preparedStmtId.clear();
// 		return false;
// 	}
// 
// 	PQclear(result);
// 	d->preparedStmtId = stmtId;
	return true;
}
bool CPgSQLResult::exec()
{
	if (!d->preparedQueriesEnabled)
		return CSqlRecordset::exec();

	cleanup();

// 	string stmt;
// 	const string params = qCreateParamString(boundValues(), d->q->driver());
// 	if (params.isEmpty())
// 		stmt = QString::fromLatin1("EXECUTE %1").arg(d->preparedStmtId);
// 	else
// 		stmt = QString::fromLatin1("EXECUTE %1 (%2)").arg(d->preparedStmtId).arg(params);
// 
// 	d->result = d->driver->exec(stmt);

	return d->processResults();
}

//取字段个数
int CPgSQLResult::GetFieldNum()
{
	if (d->result == NULL)
		return -1;
	return PQnfields(d->result);
}
//取字段名
string CPgSQLResult::GetFieldName(long lIndex)
{
	if (d->result == NULL)
		return "";

	return PQfname(d->result, lIndex);	
}
//取字段编号
int CPgSQLResult::GetIndex(const char* strFieldName)
{
	if (d->result == NULL)
		return -1;

	return PQfnumber(d->result, strFieldName);
}

int CPgSQLResult::GetFieldType(int lIndex)
{
	ASSERT(d->result!=NULL);

	int field_type=PQftype(d->result, lIndex);
	int rtn=FLD_UNKNOWN;
	switch(field_type)
	{
	case 16:	return FLD_BOOL;
	case 17:	return FLD_BLOB;
	case 18:	return FLD_CHAR;
	case 20:	return FLD_INT8;
	case 21:	return FLD_INT2;
	case 23:	return FLD_INT4;
	case 700:	return FLD_FLOAT;
	case 701:	return FLD_DOUBLE;
	case 1082:	return FLD_DATE;
	case 1083:	return FLD_TIME;
	case 1114:	return FLD_DATETIME;
	case 1700:	return FLD_NUMERIC;
	case 19:	//name
	case 25:
	case 1043:	return FLD_VARCHAR;
	}
	return rtn;
}

int CPgSQLResult::GetFieldType(const char *szFldName)
{
	ASSERT(d->result!=NULL);
	int nIndex = GetIndex(szFldName);

	return GetFieldType(nIndex);
}

bool CPgSQLResult::GetValue(int nColumns, bool& bValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	if(PQfformat( d->result, nColumns ) == 1)
	{
		bool* l  = (bool*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		bValue=  *l;		
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		if(strValue[0]=='t')
			bValue = true;
		else
			bValue = false;
	}
	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, bool& bValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, bValue, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, byte& nValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	if(PQfformat( d->result, nColumns ) == 1)
	{
		byte* l  = (byte*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		nValue=  *l;	
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		nValue = atoi(strValue);
	}
	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, byte& nValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, nValue, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, short& nValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	if(PQfformat( d->result, nColumns ) == 1)
	{
		//short* l  = (short*)PQgetvalue(d->result,m_currRow,nColumns);
		//nValue=  ntohs(*l);	
		 memcpy( &nValue, PQgetvalue( d->result,m_lCurrRow,nColumns ), sizeof(short) );
         SWAP16PTR(&nValue);
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		nValue = atoi(strValue);
	}
	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, short& nValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, nValue, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, long& lValue, bool &nIsNull)
{
	//需要更改，与操作系统有关
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	if(PQfformat( d->result, nColumns ) == 1)
	{
		//long* l  = (long*)PQgetvalue(d->result,m_currRow,nColumns);
		//lValue=  ntohl(*l);	
		memcpy( &lValue, PQgetvalue(d->result,m_lCurrRow,nColumns), sizeof(long));
		SWAP32PTR(&lValue);
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		lValue = atol(strValue);
	}

	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, long& lValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, lValue, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, __int64& nValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	if(PQfformat( d->result, nColumns ) == 1)
	{
		unsigned int nVal[2];
		memcpy( nVal, PQgetvalue(d->result,m_lCurrRow,nColumns), 8 );
		SWAP32PTR(&nVal[0]);
		SWAP32PTR(&nVal[1]);
		nValue = (__int64) ((((unsigned __int64)nVal[0]) << 32) | nVal[1]);
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		nValue = atoi(strValue);
	}

	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, __int64& nValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, nValue, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, string& strValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	strValue = PQgetvalue(d->result,m_lCurrRow,nColumns);
	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, string& strValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, strValue, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, int& nValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	if(PQfformat( d->result, nColumns ) == 1)
	{
		//int* l  = (int*)PQgetvalue(d->result,m_currRow,nColumns);
		//nValue=  ntohl(*l);
		memcpy( &nValue, PQgetvalue(d->result,m_lCurrRow,nColumns), sizeof(int));
		SWAP32PTR(&nValue);
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		nValue = atoi(strValue);
	}

	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, int& nValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, nValue, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, float& fValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}
	if(PQfformat( d->result, nColumns ) == 1)
	{
		memcpy( &fValue, PQgetvalue(d->result,m_lCurrRow,nColumns), sizeof(float) );
        SWAP32PTR(&fValue);	
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		fValue = (float)atof(strValue);
	}

	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, float& fValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;

	return GetValue(nIndex, fValue, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, double& dValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	if(PQfformat( d->result, nColumns ) == 1)
	{
		/*char val[9];
		memcpy(val,PQgetvalue(d->result,m_currRow,nColumns),8);
		array_reverse(val,8);
		memcpy(&dValue,val,8);*/
		int fld_type=GetFieldType(nColumns);
		if(fld_type==FLD_DOUBLE)
		{
			memcpy(&dValue, PQgetvalue(d->result,m_lCurrRow,nColumns),8);
			SWAPDOUBLE(&dValue);
		}
		else if(fld_type==FLD_NUMERIC)
		{
			unsigned short sLen, sSign, sDscale;
			short sWeight;
			char* pabyData = PQgetvalue( d->result,m_lCurrRow,nColumns );
			memcpy( &sLen, pabyData, sizeof(short));
			pabyData += sizeof(short);
			SWAP16PTR(&sLen);
			memcpy( &sWeight, pabyData, sizeof(short));
			pabyData += sizeof(short);
			SWAP16PTR(&sWeight);
			memcpy( &sSign, pabyData, sizeof(short));
			pabyData += sizeof(short);
			SWAP16PTR(&sSign);
			memcpy( &sDscale, pabyData, sizeof(short));
			pabyData += sizeof(short);
			SWAP16PTR(&sDscale);
		
			NumericVar var;
			var.ndigits = sLen;
			var.weight = sWeight;
			var.sign = sSign;
			var.dscale = sDscale;
			var.digits = (NumericDigit*)pabyData;
			char* str = PGGetStrFromBinaryNumeric(&var);
			dValue=atof(str);
			delete[] str;			
		}
	}
	else
	{
		char* strValue  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
		dValue = (double)atof(strValue);
	}
	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, double& dValue, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, dValue, nIsNull);
}

long ScanLong( const char *pszString, int nMaxLength )
{
	long    iValue;
	char    *pszValue = new char[nMaxLength + 1];

	/* -------------------------------------------------------------------- */
	/*      Compute string into local buffer, and terminate it.             */
	/* -------------------------------------------------------------------- */
	strcpy_s( pszValue, nMaxLength,pszString);
	pszValue[nMaxLength] = '\0';

	/* -------------------------------------------------------------------- */
	/*      Use atol() to fetch out the result                              */
	/* -------------------------------------------------------------------- */
	iValue = atol( pszValue );

	delete []pszValue;
	return iValue;
}

bool ParseDateFromStr( const char *pszInput, tm &t)

{
	int bGotSomething = false;

	t.tm_year = 0;
	t.tm_mon = 0;
	t.tm_mday = 0;
	t.tm_hour = 0;
	t.tm_min = 0;
	t.tm_sec= 0;
	t.tm_isdst = 0;

	/* -------------------------------------------------------------------- */
	/*      Do we have a date?                                              */
	/* -------------------------------------------------------------------- */
	while( *pszInput == ' ' )
		pszInput++;

	if( strstr(pszInput,"-") != NULL || strstr(pszInput,"/") != NULL )
	{
		t.tm_year = atoi(pszInput);
		if( t.tm_year < 100 && t.tm_year >= 30 )
			t.tm_year += 1900;
		else if( t.tm_year < 30 && t.tm_year >= 0 )
			t.tm_year += 2000;

		while( *pszInput >= '0' && *pszInput <= '9' ) 
			pszInput++;
		if( *pszInput != '-' && *pszInput != '/' )
			return false;
		else 
			pszInput++;

		t.tm_mon = atoi(pszInput);
		if( t.tm_mon > 12 )
			return false;

		while( *pszInput >= '0' && *pszInput <= '9' ) 
			pszInput++;
		if( *pszInput != '-' && *pszInput != '/' )
			return false;
		else 
			pszInput++;

		t.tm_mday = atoi(pszInput);
		if( t.tm_mday > 31 )
			return false;

		while( *pszInput >= '0' && *pszInput <= '9' )
			pszInput++;

		bGotSomething = true;
	}

	/* -------------------------------------------------------------------- */
	/*      Do we have a time?                                              */
	/* -------------------------------------------------------------------- */
	while( *pszInput == ' ' )
		pszInput++;

	if( strstr(pszInput,":") != NULL )
	{
		t.tm_hour = atoi(pszInput);
		if( t.tm_hour > 23 )
			return false;

		while( *pszInput >= '0' && *pszInput <= '9' ) 
			pszInput++;
		if( *pszInput != ':' )
			return false;
		else 
			pszInput++;

		t.tm_min = atoi(pszInput);
		if( t.tm_min > 59 )
			return false;

		while( *pszInput >= '0' && *pszInput <= '9' ) 
			pszInput++;
		if( *pszInput != ':' )
			return false;
		else 
			pszInput++;

		t.tm_sec = atoi(pszInput);
		if( t.tm_sec > 59 )
			return false;

		while( (*pszInput >= '0' && *pszInput <= '9')
			|| *pszInput == '.' )
			pszInput++;

		bGotSomething = true;
	}

	// No date or time!
	if( !bGotSomething )
		return false;

	/* -------------------------------------------------------------------- */
	/*      Do we have a timezone?                                          */
	/* -------------------------------------------------------------------- */
	while( *pszInput == ' ' )
		pszInput++;

	if( *pszInput == '-' || *pszInput == '+' )
	{
		// +HH integral offset
		if( strlen(pszInput) <= 3 )
			t.tm_isdst = (100 + atoi(pszInput) * 4);

		else if( pszInput[3] == ':'  // +HH:MM offset
			&& atoi(pszInput+4) % 15 == 0 )
		{
			t.tm_isdst = (int)(100 
				+ atoi(pszInput+1) * 4
				+ (atoi(pszInput+4) / 15));

			if( pszInput[0] == '-' )
				t.tm_isdst = -1 * (t.tm_isdst - 100) + 100;
		}
		else if( isdigit(pszInput[3]) && isdigit(pszInput[4])  // +HHMM offset
			&& atoi(pszInput+3) % 15 == 0 )
		{
			t.tm_isdst = (int)(100 
				+ static_cast<int>(ScanLong(pszInput+1,2)) * 4
				+ (atoi(pszInput+3) / 15));

			if( pszInput[0] == '-' )
				t.tm_isdst = -1 * (t.tm_isdst - 100) + 100;
		}
		else if( isdigit(pszInput[3]) && pszInput[4] == '\0'  // +HMM offset
			&& atoi(pszInput+2) % 15 == 0 )
		{
			t.tm_isdst = (int)(100 
				+ static_cast<int>(ScanLong(pszInput+1,1)) * 4
				+ (atoi(pszInput+2) / 15));

			if( pszInput[0] == '-' )
				t.tm_isdst = -1 * (t.tm_isdst - 100) + 100;
		}
		// otherwise ignore any timezone info.
	}

	return true;
}
bool CPgSQLResult::GetValue(int nColumns, tm& time, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;

	if(PQgetisnull(d->result,m_lCurrRow,nColumns))
	{
		nIsNull=true;
		return true;
	}

	memset(&time,0,sizeof(tm));
	if(PQfformat( d->result, nColumns ) == 1)
	{
		short len = (int)PQgetlength(d->result, m_lCurrRow, nColumns);
		int field_type =::PQftype(d->result,nColumns);

		int nVal, nYear, nMonth, nDay;
		int nHour, nMinute, nSecond;
		double dfsec;
		if(len==4 && field_type==1082) //日期
		{
			memcpy( &nVal, PQgetvalue( d->result, m_lCurrRow, nColumns ), sizeof(int) );
            SWAP32PTR(&nVal);
            PGj2date(nVal + POSTGRES_EPOCH_JDATE, &nYear, &nMonth, &nDay);
			time.tm_year=nYear;
			time.tm_mon=nMonth;
			time.tm_mday=nDay;
		}
		else if(len==8 && field_type==1083)//一日内时间
		{
			unsigned int nVal[2];
			__int64 llVal;
			memcpy( nVal, PQgetvalue( d->result, m_lCurrRow, nColumns  ), 8 );
			SWAP32PTR(&nVal[0]);
			SWAP32PTR(&nVal[1]);
			llVal = (__int64) ((((unsigned __int64)nVal[0]) << 32) | nVal[1]);
			PGdt2timeInt8(llVal, &nHour, &nMinute, &nSecond, &dfsec);
			time.tm_hour=nHour;
			time.tm_min=nMinute;
			time.tm_sec=nSecond;
			time.tm_isdst= (int)dfsec;
		}
		else if(len==8 && field_type==1114)//包括日期和时间
		{
			unsigned int nVal[2];
			__int64 llVal;
			memcpy( nVal, PQgetvalue( d->result, m_lCurrRow, nColumns ), 8 );
			SWAP32PTR(&nVal[0]);
			SWAP32PTR(&nVal[1]);
			llVal = (__int64) ((((unsigned __int64)nVal[0]) << 32) | nVal[1]);
			if (PGTimeStamp2DMYHMS(llVal, &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond) == 0)
			{
				time.tm_year=nYear;
				time.tm_mon=nMonth;
				time.tm_mday=nDay;
				time.tm_hour=nHour;
				time.tm_min=nMinute;
				time.tm_sec=nSecond;
				time.tm_isdst=0;
			}
		}		
	}
	else
	{
		char* strValue  = PQgetvalue(d->result,m_lCurrRow,nColumns);
		if(!strValue)
			return false;
		ParseDateFromStr(strValue, time);
	}
	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, tm& time, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	ASSERT(strFieldName != "");

	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, time, nIsNull);
}


bool CPgSQLResult::GetValue(int nColumns, byte** bDat, long *len, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;
	if (PQgetisnull(d->result, m_lCurrRow, nColumns))
		nIsNull = true;

	*bDat  = (byte*)PQgetvalue(d->result,m_lCurrRow,nColumns);
	*len = (long)PQgetlength(d->result, m_lCurrRow, nColumns);

	return true;
}

bool CPgSQLResult::GetValue(char* strFieldName, byte** bDat, long *len, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	if(strFieldName == "")
		return false;
	int nIndex =PQfnumber(d->result,strFieldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, bDat, len, nIsNull);
}

bool CPgSQLResult::GetValue(int nColumns, char* strValue, short len,bool &nIsNull)
{
	ASSERT(d->result != NULL);
	nIsNull = false;
	if (PQgetisnull(d->result, m_lCurrRow, nColumns))
	{
		nIsNull = true;
		return true;
	}
	char* szData  = (char*)PQgetvalue(d->result,m_lCurrRow,nColumns);
	short len0 = (int)PQgetlength(d->result, m_lCurrRow, nColumns)+1;

	strcpy_s(strValue, min(len,len0), szData);
	return true;
}
bool CPgSQLResult::GetValue(char* strFldName, char* strValue, short len, bool &nIsNull)
{
	ASSERT(d->result != NULL);
	if(strFldName == "")
		return false;
	int nIndex =PQfnumber(d->result,strFldName);
	if(nIndex < 0)
		return false;
	return GetValue(nIndex, strValue, len, nIsNull);
}

bool CPgSQLResult::IsEOF()
{
	if (d->result == NULL)
		return true;

	return m_lCurrRow >= PQntuples(d->result);
}

bool CPgSQLResult::IsBOF()
{
	return	m_lCurrRow == 0;
}

bool CPgSQLResult::IsNull()
{
	return d->result == NULL;
}
bool CPgSQLResult::IsEmpty()
{
	if (!d->result)
		return true;

	return PQntuples(d->result)==0;
}

//获取数据实际长度
int CPgSQLResult::GetValueSize(long lIndex)
{
	if (d->result == NULL)
		return -1;
	return PQgetlength(d->result, m_lCurrRow, lIndex);
}

int CPgSQLResult::GetValueSize(char* lpszFieldName)
{
	long lIndex = PQfnumber(d->result, lpszFieldName);
	return GetValueSize(lIndex);
}

//取字段定义长度
int CPgSQLResult::GetFieldDefineSize(int lIndex)
{
	ASSERT(d->result != NULL);	
	int field_type=PQftype(d->result, lIndex);
	switch(field_type)
	{
	case 16:			return 1;
	case 17:			return -1;
	case 18:			return 1;
	case 21:			return 2;
	case 23:			return 4;
	case 20:			return 8;
	case 700:			return 4;
	case 701:			return 8;
	case 1043:			return ::PQfmod(d->result, lIndex)-4;
	case 1082:			break;
	case 1083:			break;
	case 1114:			break;
	case 1700:			break;
	default:			break;
	}		
	int size=PQfsize(d->result, lIndex);	
	return size;
}

int CPgSQLResult::GetFieldDefineSize(const char* lpszFieldName)
{
	ASSERT(d->result != NULL);
	long lIndex = PQfnumber(d->result, lpszFieldName);
	return GetFieldDefineSize(lIndex); 
}


//////////////////////////////////////////////////////////////////////////
CPgSQLDriver::CPgSQLDriver(void)
{
	init();
}

CPgSQLDriver::CPgSQLDriver(PGconn *conn)
{
	init();
	pg_d->connection = conn;
	if (conn) {
		pg_d->pro = pg_d->getPSQLVersion();
		setOpen(true);
	}
}


CPgSQLDriver::~CPgSQLDriver(void)
{
	if (pg_d->connection)
		PQfinish(pg_d->connection);
	
	if (pg_d)
		delete pg_d;
	pg_d = NULL;
}

bool CPgSQLDriver::open(const char* db, const char* user, const char* password,
		  const char* host, int port, const char* connOpts)
{
	if (IsOpen())
		close();

	string connectString = "";
	m_strError = "";

	if (host)
		connectString += "host="  + string(host);
	if (db && strlen(db)>0)
		connectString += " dbname=" +string(db);
	if (user)
		connectString += " user=" + string(user);
	if (password)
		connectString += " password=" + string(password);
	if (port > 0)
	{
		char szPort[20] = "";
		_itoa_s(port, szPort, 20, 10);
		connectString += " port=" + string(szPort);
	}
	else
		connectString += " port=5432";

	pg_d->connection = PQconnectdb(connectString.c_str());
	if (PQstatus(pg_d->connection) == CONNECTION_BAD) 
	{
		m_strError = string(PQerrorMessage(pg_d->connection));
		TRACE("pgsql open error:%s", m_strError.c_str());

		PQfinish(pg_d->connection);
		pg_d->connection = NULL;
		return false;
	}

	pg_d->pro = pg_d->getPSQLVersion();
	//d->isUtf8 = d->setEncodingUtf8();

	//客户端编码设置为GBK
	PQsetClientEncoding(pg_d->connection, "GBK");

	setOpen(true);
	return true;
}

bool CPgSQLDriver::IsOpen() const
{
	return PQstatus(pg_d->connection) == CONNECTION_OK;
}

void CPgSQLDriver::close()
{
	if (IsOpen()) {
		if (pg_d->connection)
			PQfinish(pg_d->connection);
		pg_d->connection = NULL;
		setOpen(false);
	}
}

CPgSQLDriver::Protocol CPgSQLDriver::protocol() const
{
	return pg_d->pro;
}

CSqlRecordset *CPgSQLDriver::createResult() const
{
	return new CPgSQLResult(this, pg_d);
}

void CPgSQLDriver::SetError(char *szError)
{
	m_strError = string(szError);
}

void CPgSQLDriver::init()
{
    pg_d = new CPgSQLDriverPrivate;
}

bool CPgSQLDriver::BeginTransaction()
{
	if (!IsOpen()) 
	{
		TRACE("CPgSQLDriver::beginTransaction: Database not open");
		return false;
	}

	PGresult* res = pg_d->exec("BEGIN");
	m_strError = "";

	if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) 
	{
		m_strError = PQerrorMessage(pg_d->connection);
		TRACE("CPgSQLDriver::BeginTransaction:Begin transaction error:%s", m_strError.c_str());

		PQclear(res);
		return false;
	}

	PQclear(res);
	return true;
}
bool CPgSQLDriver::CommitTransaction()
{
	if (!IsOpen()) 
	{
		TRACE("CPgSQLDriver::commitTransaction: Database is not open");
		return false;
	}

	PGresult* res = pg_d->exec("COMMIT");
	bool transaction_failed = false;
	m_strError = "";
	transaction_failed = (lstrcmp(PQcmdStatus(res), "ROLLBACK") == 0);

	if (!res || PQresultStatus(res) != PGRES_COMMAND_OK || transaction_failed) 
	{
		m_strError = PQerrorMessage(pg_d->connection);
		TRACE("CPgSQLDriver::CommitTransaction:commit transaction error:%s", m_strError.c_str());

		PQclear(res);
		return false;
	}

	PQclear(res);
	return true;
}
bool CPgSQLDriver::RollbackTransaction()
{
	if (!IsOpen()) 
	{
		TRACE("CPgSQLDriver::rollbackTransaction: Database not open");
		return false;
	}
	PGresult* res = pg_d->exec("ROLLBACK");
	if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) 
	{
		m_strError = PQerrorMessage(pg_d->connection);
		TRACE("CPgSQLDriver::RollbackTransaction:rollback transaction error:%s", m_strError.c_str());

		PQclear(res);
		return false;
	}

	PQclear(res);
	return true;
}

int CPgSQLDriver::ExecuteSql(const char *szSql)
{
	if (szSql == NULL)
		return 0;

	PGresult *pRes = PQexec(pg_d->connection, szSql);
	if (!pRes)
		return false;

	int status = PQresultStatus(pRes);
	if (status == PGRES_COMMAND_OK) 
		return true;
	else
	{
		m_strError = PQerrorMessage(pg_d->connection);
		TRACE("CPgSQLDriver::ExecuteSql error:%s", m_strError.c_str());
	}

	return false;
}

int CPgSQLDriver::ExecBinary(const char *szSql, byte **szBinVal, int *lLen, short nNum)
{
	return 1;
}