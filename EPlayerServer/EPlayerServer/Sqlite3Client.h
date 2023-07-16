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
	CSqlite3Client(const CSqlite3Client&) = delete;  //�������ƹ���
	CSqlite3Client& operator= (const CSqlite3Client&) = delete;
public:
	virtual int Connect(const KeyValue& args); //����
	virtual int Exec(const Buffer& sql);//sqlΪ�����sql��� ����ΪBuffer  //ִ��
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table);  //��ѯ���  ��� �Լ��ɽ�����ɵı�
	virtual int StartTransaction();  //��������
	virtual int CommitTransaction();//�ύ����
	virtual int RollbackTransaction();//�ع�����
	virtual int Close();  //�ر�����
	virtual bool IsConnected();  //�Ƿ�������
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
	virtual Buffer Create();//���ش�����sql���
	virtual Buffer Drop();
public:
	//��ɾ�Ĳ�
	virtual Buffer Insert(const _Table_& values);//��
	virtual Buffer Delete(const _Table_& values);//ɾ
	virtual Buffer Modify(const _Table_& values);//��
	virtual Buffer Query(const Buffer& condition = ""); //��
	//����һ�����ڱ�Ķ���
	virtual PTable Copy() const;//����
	virtual void ClearFieldUsed();//����ȫ����ʹ��״̬
public:
	//��ȡ���ȫ��
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
	virtual Buffer Create(); //���е�sql���
	virtual void LoadFromStr(const Buffer& str);
	//where���ʹ��
	virtual Buffer toEqualExp() const;//�õ��Ⱥű��ʽ
	virtual Buffer toSqlString() const;//ת���ַ���
	virtual operator const Buffer() const;//�õ��е�ȫ��
private:
	Buffer StrToHex(const Buffer& data) const;
private:
	union {
		bool Bool;
		int Integer;
		double Double;
		Buffer* String;
	}Value;
	int nType;//����
};

//#name��ʾ��Сдnameת�����ַ���
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