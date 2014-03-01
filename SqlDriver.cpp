#include "StdAfx.h"
#include "SqlDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////////////////////
CSqlDriver::CSqlDriver(void)
{
	m_poolflag=0;
	m_dbType = DBUnknown;

	m_bIsOpen = false;
	m_bIsOpenError =false;
	m_strError = "";
}

CSqlDriver::~CSqlDriver(void)
{
	m_bIsOpen = false;
	m_bIsOpenError =false;
	m_strError = "";
}

bool CSqlDriver::IsOpen() const
{
	return m_bIsOpen;
}

bool CSqlDriver::isOpenError() const
{
	return m_bIsOpenError;
}

void CSqlDriver::setOpen(bool o)
{
	m_bIsOpen = o;
}

void CSqlDriver::setOpenError(bool e)
{
	m_bIsOpenError = e;
}

string CSqlDriver::GetLastError()
{
	return m_strError;
}

void CSqlDriver::SetLastError(string &szError)
{
	m_strError = szError;
}

short CSqlDriver::GetPoolFlag()
{
	return m_poolflag;
}

short CSqlDriver::SetPoolFlag(short poolflag)
{
	short rtn=m_poolflag;
	m_poolflag=poolflag;
	return rtn;
}

