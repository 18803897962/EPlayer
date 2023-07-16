#pragma once
//�ӿ���
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
	CDataBaseClient(const CDataBaseClient&) = delete;  //�������ƹ���
	CDataBaseClient& operator= (const CDataBaseClient&) = delete;
public:
	virtual int Connect(const KeyValue& args)=0; //����
	virtual int Exec(const Buffer& sql) = 0;//sqlΪ�����sql��� ����ΪBuffer  //ִ��
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table) = 0;  //��ѯ���  ��� �Լ��ɽ�����ɵı�
	virtual int StartTransaction() = 0;  //��������
	virtual int CommitTransaction() = 0;//�ύ����
	virtual int RollbackTransaction() = 0;//�ع�����
	virtual int Close() = 0;  //�ر�����
	virtual bool IsConnected() = 0;  //�Ƿ�������
};

//����еĻ����ʵ��

class _Field_;
using PField = std::shared_ptr<_Field_>;
using FieldArray = std::vector<PField>;
using FieldMap = std::map<Buffer, PField>;   //Name �� ptr �ļ�ֵ��



class _Table_ {//��
public:
	_Table_() {}
	virtual ~_Table_() {}
public:
	virtual Buffer Create() = 0;//���ش�����sql���
	virtual Buffer Drop() = 0;
public:
	//��ɾ�Ĳ�
	virtual Buffer Insert(const _Table_& values) = 0;//��
	virtual Buffer Delete(const _Table_& values) = 0;//ɾ
	virtual Buffer Modify(const _Table_& values) = 0;//��
	//virtual Buffer Query() = 0; //��
	virtual Buffer Query(const Buffer& condition="") = 0;
	//����һ�����ڱ�Ķ���
	virtual PTable Copy() const = 0;//����
	virtual void ClearFieldUsed() = 0;//����ȫ����ʹ��״̬
public:	
	//��ȡ���ȫ��
	virtual operator const Buffer() const = 0;
public:
	Buffer Database;//��������DB����
	Buffer Name;
	FieldArray FieldDefine;//�� �洢��ѯ���
	FieldMap Fields;//�еĶ���ӳ���
};

enum {
	SQL_INSERT=1,//�������
	SQL_MODIFY=2,//�޸ĵ���
	SQL_CONSITION=4//��ѯ������
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

class _Field_ {//��
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
	virtual Buffer Create() = 0; //���е�sql���
	virtual void LoadFromStr(const Buffer& str) = 0;
	//where���ʹ��
	virtual Buffer toEqualExp() const = 0;//�õ��Ⱥű��ʽ
	virtual Buffer toSqlString() const = 0;//ת���ַ���
	virtual operator const Buffer() const = 0;//�õ��е�ȫ��
public:
	Buffer Name;
	Buffer Type;
	Buffer Size;
	unsigned Attr;//����
	Buffer Default;//Ĭ��ֵ
	Buffer Check;//Լ������
	//��������
public:
	unsigned Condition; 
	union {
		bool Bool;
		int Integer;
		double Double;
		Buffer* String;
	}Value;
	int nType;//����
};
