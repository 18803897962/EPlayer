#pragma once
#include"Public.h"
#include"DataBaseHelper.h"
#include<mysql/mysql.h>

class CMysqlClient :public CDataBaseClient {
public:
	CMysqlClient() {
		bzero(&m_db, sizeof(m_db));
		m_bInit = false;
	}
	virtual ~CMysqlClient() {
		Close();
	}
public:
	CMysqlClient(const CMysqlClient&) = delete;  //不允许复制构造
	CMysqlClient& operator= (const CMysqlClient&) = delete;
public:
	virtual int Connect(const KeyValue& args); //连接
	virtual int Exec(const Buffer& sql);//sql为传入的sql语句 类型为Buffer  //执行
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table);  //查询语句  结果 以及由结果构成的表
	virtual int StartTransaction();  //开启事务
	virtual int CommitTransaction();//提交事务
	virtual int RollbackTransaction();//回滚事务
	virtual int Close();  //关闭连接
	virtual bool IsConnected();  //是否已连接
private:
	MYSQL m_db;
	bool m_bInit;//默认是false 表示未初始化 初始化之后则为true 表示已经连接
private:
	class ExecParam {
	public:
		ExecParam(CMysqlClient* obj, Result& result, const _Table_& table) :
			obj(obj), result(result), table(table) {}
		CMysqlClient* obj;
		Result& result;
		const _Table_& table;
	};
};


class _mysql_table_ :public _Table_ {
public:
	_mysql_table_() :_Table_() {}
	virtual ~_mysql_table_() {}
public:
	_mysql_table_(const _mysql_table_& table);
public:
	virtual Buffer Create();//返回创建的sql语句
	virtual Buffer Drop();
public:
	//增删改查
	virtual Buffer Insert(const _Table_& values);//增
	virtual Buffer Delete(const _Table_& values);//删
	virtual Buffer Modify(const _Table_& values);//改
	virtual Buffer Query(const Buffer& condition = ""); //查
	//创建一个基于表的对象
	virtual PTable Copy() const;//复制
	virtual void ClearFieldUsed();//清理全部的使用状态
public:
	//获取表的全名
	virtual operator const Buffer() const;
};



class _mysql_field_ :public _Field_ {
public:
	_mysql_field_();
	_mysql_field_(int ntype, const Buffer& name, unsigned attr,
		const Buffer& type, const Buffer& size,
		const Buffer& default_, const Buffer& check);
	_mysql_field_(const _mysql_field_& field);
	virtual ~_mysql_field_();
	virtual Buffer Create(); //本列的sql语句
	virtual void LoadFromStr(const Buffer& str);
	//where语句使用
	virtual Buffer toEqualExp() const;//得到等号表达式
	virtual Buffer toSqlString() const;//转成字符串
	virtual operator const Buffer() const;//拿到列的全名
private:
	Buffer StrToHex(const Buffer& data) const;
private:
	
};


#define DECLARE_TABLE_CLASS(name,base) class name:public base{ \
public:\
virtual PTable Copy() const {return PTable(new name(*this));} \
name():base(){Name=#name;

#define DECLARE__MYSQL_FIELD(ntype,name,attr,type,size,default_,check) \
{PField field(new _mysql_field_(ntype,#name,attr,type,size,default_,check)); \
FieldDefine.push_back(field); Fields[#name] = field; }

#define DECLARE_TABLE_CLASS_END() }};
