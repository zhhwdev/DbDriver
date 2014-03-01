#pragma once

#include "SqlDriver.h"
#include "SqlRecordset.h"

/*
 *This driver is only for oracle
 *Add by zhhw at 2013-06-03
 *It does not completed
 */

class COciSQLDriverPrivate;
class COciSQLResultPrivate;
class COciSQLDriver;

class COciSQLResult: public CSqlRecordset
{

};

class COciSQLDriver : public CSqlDriver
{

};