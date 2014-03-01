#ifndef SQL_DEF_H
#define SQL_DEF_H

//数据库类型
typedef enum 
{
	DBUnknown		= 0,
	DB_SQLITE		= 1,
	DB_POSTGRESQL	= 2,
	DB_ACCESS		= 3,
	DB_ORACLE		= 4,
	DB_MSSQL		= 5,
	DB_EXCEL        = 6,
}DB_TYPE;


//字段类型
typedef enum
{
	FLD_UNKNOWN		=	0,	//未知
	FLD_BOOL			,	//布尔型
	FLD_BLOB			,	//二进制
	FLD_CHAR			,	//字符型
	FLD_INT2			,	
	FLD_INT4			,
	FLD_INT8			,
	FLD_FLOAT			,	//float4
	FLD_DOUBLE			,	//float8
	FLD_VARCHAR			,	//varchar
	FLD_DATE			,
	FLD_TIME			,
	FLD_DATETIME		,
	FLD_NUMERIC			,
}FIELD_TYPE;



#endif