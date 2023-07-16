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
	CMysqlClient(const CMysqlClient&) = delete;  //�������ƹ���
	CMysqlClient& operator= (const CMysqlClient&) = delete;
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
	MYSQL m_db;
	bool m_bInit;//Ĭ����false ��ʾδ��ʼ�� ��ʼ��֮����Ϊtrue ��ʾ�Ѿ�����
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



class _mysql_field_ :public _Field_ {
public:
	_mysql_field_();
	_mysql_field_(int ntype, const Buffer& name, unsigned attr,
		const Buffer& type, const Buffer& size,
		const Buffer& default_, const Buffer& check);
	_mysql_field_(const _mysql_field_& field);
	virtual ~_mysql_field_();
	virtual Buffer Create(); //���е�sql���
	virtual void LoadFromStr(const Buffer& str);
	//where���ʹ��
	virtual Buffer toEqualExp() const;//�õ��Ⱥű��ʽ
	virtual Buffer toSqlString() const;//ת���ַ���
	virtual operator const Buffer() const;//�õ��е�ȫ��
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
