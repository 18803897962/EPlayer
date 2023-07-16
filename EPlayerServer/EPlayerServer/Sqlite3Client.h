#pragma once
#include"Public.h"
#include"DataBaseHelper.h"
#include"sqlite3.h"
class CSqlite3Client:public CDataBaseClient{
public:
	CSqlite3Client() {
		m_db = NULL;
		m_stmt = NULL;
	}
	virtual ~CSqlite3Client() {
		Close();
	}
public:
	CSqlite3Client(const CSqlite3Client&) = delete;  //不允许复制构造
	CSqlite3Client& operator= (const CSqlite3Client&) = delete;
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
	static int ExecCallback(void* arg, int count, char** name, char** value);
	int ExecCallback(Result& result,const _Table_& table,int count, char** name, char** value);
private:
	sqlite3_stmt* m_stmt;
	sqlite3* m_db;
private:
	class ExecParam {
	public:
		ExecParam(CSqlite3Client* obj, Result& result, const _Table_& table) :
		obj(obj), result(result), table(table){}
		CSqlite3Client* obj;
		Result& result;
		const _Table_& table;
	};
};


class _sqlite3_table_:public _Table_ {
public:
	_sqlite3_table_():_Table_() {}
	virtual ~_sqlite3_table_() {}
public:
	_sqlite3_table_(const _sqlite3_table_& table);
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



class _sqlite3_field_ :public _Field_{
public:
	_sqlite3_field_() :_Field_(){
		nType = TYPE_NULL;
		Value.Integer = 0;
		Value.Double = 0.0;
	}
	_sqlite3_field_(int ntype,const Buffer& name, unsigned attr,
		const Buffer& type, const Buffer& size, 
		const Buffer& default_, const Buffer& check);
	_sqlite3_field_(const _sqlite3_field_& field);
	virtual ~_sqlite3_field_();
	virtual Buffer Create(); //本列的sql语句
	virtual void LoadFromStr(const Buffer& str);
	//where语句使用
	virtual Buffer toEqualExp() const;//得到等号表达式
	virtual Buffer toSqlString() const;//转成字符串
	virtual operator const Buffer() const;//拿到列的全名
private:
	Buffer StrToHex(const Buffer& data) const;
private:
	union {
		bool Bool;
		int Integer;
		double Double;
		Buffer* String;
	}Value;
	int nType;//类型
};

//#name表示将小写name转换成字符串
/*
#define DECLARE_TABLE_CLASS(name,base) class name:public base{ \
public:\
virtual PTable Copy() const {return PTable(new name(*this));} \
name():base(){Name=#name;

#define DECLARE_FIELD(ntype,name,attr,type,size,default_,check) \
{PField field(new _sqlite3_field_(ntype,#name,attr,type,size,default_,check)); \
FieldDefine.push_back(field); Fields[#name] = field; }

#define DECLARE_TABLE_CLASS_END() }};
*/