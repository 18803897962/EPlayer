#pragma once
//接口类
#include"Public.h"
#include<map>
#include<list>
#include <memory>
#include<vector>
class _Table_;
using PTable = std::shared_ptr<_Table_>;
using KeyValue = std::map<Buffer, Buffer>;
using Result = std::list<PTable>;
class CDataBaseClient
{
public:
	CDataBaseClient() {}
	virtual ~CDataBaseClient() {}
public:
	CDataBaseClient(const CDataBaseClient&) = delete;  //不允许复制构造
	CDataBaseClient& operator= (const CDataBaseClient&) = delete;
public:
	virtual int Connect(const KeyValue& args)=0; //连接
	virtual int Exec(const Buffer& sql) = 0;//sql为传入的sql语句 类型为Buffer  //执行
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table) = 0;  //查询语句  结果 以及由结果构成的表
	virtual int StartTransaction() = 0;  //开启事务
	virtual int CommitTransaction() = 0;//提交事务
	virtual int RollbackTransaction() = 0;//回滚事务
	virtual int Close() = 0;  //关闭连接
	virtual bool IsConnected() = 0;  //是否已连接
};

//表和列的基类的实现

class _Field_;
using PField = std::shared_ptr<_Field_>;
using FieldArray = std::vector<PField>;
using FieldMap = std::map<Buffer, PField>;   //Name 和 ptr 的键值对



class _Table_ {//表
public:
	_Table_() {}
	virtual ~_Table_() {}
public:
	virtual Buffer Create() = 0;//返回创建的sql语句
	virtual Buffer Drop() = 0;
public:
	//增删改查
	virtual Buffer Insert(const _Table_& values) = 0;//增
	virtual Buffer Delete(const _Table_& values) = 0;//删
	virtual Buffer Modify(const _Table_& values) = 0;//改
	//virtual Buffer Query() = 0; //查
	virtual Buffer Query(const Buffer& condition="") = 0;
	//创建一个基于表的对象
	virtual PTable Copy() const = 0;//复制
	virtual void ClearFieldUsed() = 0;//清理全部的使用状态
public:	
	//获取表的全名
	virtual operator const Buffer() const = 0;
public:
	Buffer Database;//表所属的DB名称
	Buffer Name;
	FieldArray FieldDefine;//列 存储查询结果
	FieldMap Fields;//列的定义映射表
};

enum {
	SQL_INSERT=1,//插入的列
	SQL_MODIFY=2,//修改的列
	SQL_CONSITION=4//查询条件列
};

enum {
	NONE=0,
	NOT_NULL=1,
	DEFAULT=2,
	UNIQUE=4,
	PRIMARY_KEY=8,
	CHECK=16,
	AUTOINCREMENT=32
};

using SqlType = enum {
	TYPE_NULL = 0,
	TYPE_BOOL = 1,
	TYPE_INT = 2,
	TYPE_DATETIME = 4,
	TYPE_REAL = 8,
	TYPE_VARCHAR = 16,
	TYPE_TEXT = 32,
	TYPE_BLOB = 64
};

class _Field_ {//列
public:
	_Field_() {}
	virtual ~_Field_() {}
public:
	_Field_(const _Field_& field) {
		Name = field.Name;
		Type = field.Type;
		Size = field.Size;
		Attr = field.Attr;
		Default = field.Default;
		Check = field.Check;
	}
	virtual _Field_& operator=(const _Field_& field) {
		if (this != &field) {
			Name = field.Name;
			Type = field.Type;
			Size = field.Size;
			Attr = field.Attr;
			Default = field.Default;
			Check = field.Check;
		}
		return *this;
	}
public:
	virtual Buffer Create() = 0; //本列的sql语句
	virtual void LoadFromStr(const Buffer& str) = 0;
	//where语句使用
	virtual Buffer toEqualExp() const = 0;//得到等号表达式
	virtual Buffer toSqlString() const = 0;//转成字符串
	virtual operator const Buffer() const = 0;//拿到列的全名
public:
	Buffer Name;
	Buffer Type;
	Buffer Size;
	unsigned Attr;//属性
	Buffer Default;//默认值
	Buffer Check;//约束条件
	//操作条件
public:
	unsigned Condition; 
	union {
		bool Bool;
		int Integer;
		double Double;
		Buffer* String;
	}Value;
	int nType;//类型
};
