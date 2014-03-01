#include "StdAfx.h"
#include "SqlRecordset.h"
#include "SqlDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CSqlRecordsetPrivate
{
public:
	CSqlRecordsetPrivate(CSqlRecordset* d)
		: q(d), active(false), isSel(false), forwardOnly(false)
	{
		sql = "";
		executedQuery = "";
	}

	void clearValues()
	{

	}

	void resetBindCount()
	{

	}


public:
	CSqlRecordset* q;
	CSqlDriver *sqldriver;
	int idx;
	string sql;
	bool active;
	bool isSel;
	//QSqlError error;
	bool forwardOnly;

// 	int bindCount;
// 	QSqlResult::BindingSyntax binds;

	string executedQuery;

};



CSqlRecordset::CSqlRecordset(const CSqlDriver *db)
{
	d = new CSqlRecordsetPrivate(this);
	d->sqldriver = const_cast<CSqlDriver *>(db);
}

CSqlRecordset::~CSqlRecordset(void)
{
	delete d;
}

string CSqlRecordset::lastQuery() const
{
	return d->sql;
}

bool CSqlRecordset::isActive() const
{
	return d->active;
}


bool CSqlRecordset::isSelect() const
{
	return d->isSel;
}

void CSqlRecordset::setSelect(bool select)
{
	d->isSel = select;
}

CSqlDriver *CSqlRecordset::driver()
{
	return d->sqldriver;
}

void CSqlRecordset::setActive(bool active)
{
	if (active && d->executedQuery.empty())
		d->executedQuery = d->sql;

	d->active = active;
}

void CSqlRecordset::setLastError(string strError)
{
	d->sqldriver->SetLastError(strError);
	TRACE(strError.c_str());
}

void CSqlRecordset::setQuery(const char* query)
{
	d->sql = string(query);
}
bool CSqlRecordset::exec()
{
	bool ret;
	// fake preparation - just replace the placeholders..
	string query = lastQuery();
	if (query.empty())
		return false;
// 	if (d->binds == NamedBinding) {
// 		int i;
// 		QVariant val;
// 		QString holder;
// 		for (i = d->holders.count() - 1; i >= 0; --i) {
// 			holder = d->holders.at(i).holderName;
// 			val = d->values.value(d->indexes.value(holder).value(0,-1));
// 			QSqlField f(QLatin1String(""), val.type());
// 			f.setValue(val);
// 			query = query.replace(d->holders.at(i).holderPos,
// 				holder.length(), driver()->formatValue(f));
// 		}
// 	} else {
// 		QString val;
// 		int i = 0;
// 		int idx = 0;
// 		for (idx = 0; idx < d->values.count(); ++idx) {
// 			i = query.indexOf(QLatin1Char('?'), i);
// 			if (i == -1)
// 				continue;
// 			QVariant var = d->values.value(idx);
// 			QSqlField f(QLatin1String(""), var.type());
// 			if (var.isNull())
// 				f.clear();
// 			else
// 				f.setValue(var);
// 			val = driver()->formatValue(f);
// 			query = query.replace(i, 1, driver()->formatValue(f));
// 			i += val.length();
// 		}
// 	}

	// have to retain the original query with placeholders
	string orig = lastQuery();
	ret = reset(query.c_str());
	d->executedQuery = query;
	setQuery(orig.c_str());
	//d->resetBindCount();
	return ret;
}

bool CSqlRecordset::prepare(const string& query)
{
	d->sql = query;
	return true; // fake prepares should always succeed
}

