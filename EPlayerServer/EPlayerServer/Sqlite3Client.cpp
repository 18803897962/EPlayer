#include "Sqlite3Client.h"
#include"Logger.h"

int CSqlite3Client::Connect(const KeyValue& args){
	auto it = args.find("host");  //���ݿ���������or·��
	if (it == args.end()) return -1;
	if (m_db != NULL) return -2;  //˵���Ѿ����� ���ܷ�������
	int ret= sqlite3_open(it->second, &m_db);//
	if (ret != 0) {
		TRACEE("connect failed ret=%d msg[%s]", ret, sqlite3_errmsg(m_db));
		return -3;
	}

	return 0;
}

int CSqlite3Client::Exec(const Buffer& sql){
	if (m_db == NULL) return -1;//δ��ʼ��
	int ret = sqlite3_exec(m_db,sql,NULL,this,NULL);  //��m_db��sql��䴫��
	if (ret != SQLITE_OK) {
		printf("(%s):%d[%s] sql={%s}\n", __FILE__,__LINE__,__FUNCTION__,(char*)sql);
		printf("(%s):%d[%s]  exec failed ret=%d msg[%s]\n", __FILE__, __LINE__, __FUNCTION__, ret, sqlite3_errmsg(m_db));
		TRACEE("sql={%s}", sql);
		TRACEE("exec failed ret=%d msg[%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::Exec(const Buffer& sql, Result& result, const _Table_& table){
	if (m_db == NULL) return -1;
	ExecParam param(this, result, table);
	char* errmsg = NULL;
	int ret = sqlite3_exec(m_db, sql,
		&CSqlite3Client::ExecCallback ,(void*)&param , &errmsg);
	if (ret != SQLITE_OK) {
		TRACEE("sql={%s}", sql);
		TRACEE("exec failed ret=%d msg[%s]", ret, errmsg);
		if (errmsg) sqlite3_free(errmsg);
		return -2;
	}
	if (errmsg) sqlite3_free(errmsg);
	return 0;
}

int CSqlite3Client::StartTransaction(){//��������
	if (m_db == NULL) return -1;
	int ret = sqlite3_exec(m_db, "BEGIN TRANSACTION", 0, 0, NULL); //ִ�п�������
	if (ret != SQLITE_OK) {
		TRACEE("BEGIN TRANSACTION");
		TRACEE("exec failed ret=%d msg[%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::CommitTransaction(){//�ύ����
	if (m_db == NULL) return -1;
	int ret = sqlite3_exec(m_db, "COMMIT TRANSACTION", 0, 0, NULL); //ִ���ύ����
	if (ret != SQLITE_OK) {
		TRACEE("BEGIN TRANSACTION");
		TRACEE("exec failed ret=%d msg[%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::RollbackTransaction(){
	if (m_db == NULL) return -1;
	int ret = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", 0, 0, NULL); 
	if (ret != SQLITE_OK) {
		TRACEE("ROLLBACK TRANSACTION");
		TRACEE("exec failed ret=%d msg[%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::Close(){
	if (m_db == NULL) return -1;
	int ret = sqlite3_close(m_db);
	if (ret != SQLITE_OK) {
		TRACEE("close failed ret = % d msg[% s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	m_db = NULL;
	return 0;
}

bool CSqlite3Client::IsConnected(){
	return m_db!=NULL;
}

int CSqlite3Client::ExecCallback(void* arg, int count, char** value, char** name){
	ExecParam* param = (ExecParam*)arg;
	return param->obj->ExecCallback(param->result, param->table, count, name, value);
}

int CSqlite3Client::ExecCallback(Result& result, const _Table_& table, int count, char** names, char** values){
	PTable pTable = table.Copy();
	if (pTable == nullptr) {
		TRACEE("table %s is error", (const char*)(Buffer)table);
		return -1;
	}
	for (int i = 0; i < count; i++) {
		Buffer name = names[i];
		auto it = pTable->Fields.find(name);
		if (it == pTable->Fields.end()) {
			TRACEE("table %s is error", (const char*)(Buffer)table);
			return -2;
		}
		if (values[i] != NULL)
			it->second->LoadFromStr(values[i]);
	}
	result.push_back(pTable);
	return 0;
}

_sqlite3_table_::_sqlite3_table_(const _sqlite3_table_& table){
	Database = table.Database;
	Name = table.Name;
	for (size_t i = 0; i < table.Fields.size(); i++) {
		PField field = PField(new _sqlite3_field_(*(_sqlite3_field_*)table.FieldDefine[i].get()));
		//�ӻ���ָ��ת��Ϊ����ָ�� Ȼ��*������ת��Ϊ����ʵ��
		FieldDefine.push_back(field);
		Fields.insert({ field->Name, field });
	}
}

Buffer _sqlite3_table_::Create(){
	//CREATE TABLE IF NOT EXISTS ������ �����ݿ�.������ (�ж��� , ......);  �����ھʹ���
	//��ȫ�� = ���ݿ�.����
	Buffer sql = "CREATE TABLE IF NOT EXISTS "+(Buffer)(*this)+"(\r\n";
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (i > 0) {//��֮���ö��ŷָ�
			sql += ",";
		}
		sql += FieldDefine[i]->Create();  //ÿ���е�sql���
	}
	sql += ");";
	TRACEI("sql=%s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Drop(){
	Buffer sql="DROP TABLE "+(Buffer)(*this)+";";
	TRACEI("sql=%s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Insert(const _Table_& values){
	// INSERT INTO ��ȫ�� (��1����2....��n)
	//VALUE (ֵ1��ֵ2...ֵn)
	Buffer sql="INSERT INTO "+(Buffer)*this+" (";
	bool isFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++){
		if (values.FieldDefine[i]->Condition & SQL_INSERT) {//ֻ�����õ����� û�õ����в���Ҫ����
			if (!isFirst) sql += ",";
			else isFirst = false;
			sql += (Buffer)(*values.FieldDefine[i]);//��ȫ��
		}
	}
	sql += ") VALUES ( ";
	isFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_INSERT) {
			if (!isFirst) sql += ",";
			else isFirst = false;
			sql += values.FieldDefine[i]->toSqlString();
		}
	}
	sql += ");";
	TRACEI("sql=%s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Delete(const _Table_& values) {
	//DELETE FROM ��ȫ�� WHERE ����   (Buffer)*this��ʾ��ȫ��
	Buffer sql="DELETE FROM "+(Buffer)*this;
	Buffer Where = "";
	bool isFirst = true;
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (FieldDefine[i]->Condition & SQL_CONSITION) {
			if (!isFirst) Where += " AND ";
			else isFirst = false;
			Where += (Buffer)*FieldDefine[i]+"=" + FieldDefine[i]->toSqlString();
		}
	}
	if (Where.size()>0) {
		sql += " WHERE" + Where+" ";
	}
	sql += ";";
	TRACEI("sql=%s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Modify(const _Table_& values){
	// UPDATE ��ȫ�� SET ��1=ֵ1��......��n=ֵn; [WHERE ����]
	Buffer sql = "UPDATE " + (Buffer)*this + " SET ";
	bool isFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_MODIFY) {//ֻ�����õ����� û�õ����в���Ҫ����
			if (!isFirst) sql += ",";
			else isFirst = false;
			sql += (Buffer)(*values.FieldDefine[i])+"="+ values.FieldDefine[i]->toSqlString();//��ȫ��
		}
	}
	Buffer Where = "";
	isFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_CONSITION) {
			if (!isFirst) Where += " AND ";
			else isFirst = false;
			Where += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlString();
		}
	}
	if (Where.size() > 0) {
		sql += " WHERE" + Where+" ";
	}
	sql += ";";
	TRACEI("sql=%s", (char*)sql);
	return sql;
}


Buffer _sqlite3_table_::Query(const Buffer& condition){
	//  SELECT ����1������2��...������n FROM ��ȫ����
	Buffer sql = "SELECT ";
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (i > 0) {
			sql += ',';
		}
		sql += '"' + FieldDefine[i]->Name + "\" ";
	}
	sql += " FROM " + (Buffer)*this + " ";  //from ��ȫ��
	if (condition.size() > 0) {
		sql += "WHERE" + condition;
	}
	sql += ";";
	TRACEI("sql=%s", (char*)sql);
	return sql;
}

PTable _sqlite3_table_::Copy() const{
	return PTable(new _sqlite3_table_(*this));
}

void _sqlite3_table_::ClearFieldUsed(){
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		FieldDefine[i]->Condition = 0;
	}
}

_sqlite3_table_::operator const Buffer() const{
	Buffer Head;
	if (Database.size()) {//��ָ�����ݿ�
		Head = '"' + Database + "\".";  //  \��ʾת���ַ�
	}
	return Head + '"' + Name + '"';
}

/*	TYPE_VARCHAR = 16,
	TYPE_TEXT = 32,
	TYPE_BLOB = 64
*/


_sqlite3_field_::_sqlite3_field_(int ntype,const Buffer& name, unsigned attr,
	const Buffer& type, const Buffer& size, const Buffer& default_, 
	const Buffer& check) {
	nType = ntype;
	switch (nType) {
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		Value.String = new Buffer();
		break;
	default:
		break;
	}
	Name = name;
	Attr = attr;
	Type = type;
	Size = size;
	Default = default_;
	Check = check;

}

_sqlite3_field_::_sqlite3_field_(const _sqlite3_field_& field){
	nType = field.nType;
	switch (nType) {
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		Value.String = new Buffer();
		*Value.String = *field.Value.String;
		break;
	default:
		break;
	}
	Name = field.Name;
	Attr = field.Attr;
	Type = field.Type;
	Size = field.Size;
	Default = field.Default;
	Check = field.Check;
}

_sqlite3_field_::~_sqlite3_field_(){
	switch (nType) {
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		if (Value.String) {
			Buffer* ptr = Value.String;
			Value.String = NULL;
			delete ptr;
		}
		break;
	default:
		break;
	}
}


Buffer _sqlite3_field_::Create(){
	//�����ơ� ���� ����
	Buffer sql = '"'+Name+"\" "+Type+" ";
	if (Attr & NOT_NULL) {
		sql += " NOT NULL ";
	}
	if (Attr & DEFAULT) {
		sql += " DEFAULT "+Default+" ";
	}
	if (Attr & UNIQUE) {
		sql += " UNIQUE ";
	}
	if (Attr & PRIMARY_KEY) {
		sql += " PRIMARY KEY ";
	}
	if (Attr & CHECK) {
		sql += " CHECK( "+Check+") ";
	}
	if (Attr & AUTOINCREMENT) {
		sql += " AUTOINCREMENT ";
	}
	return sql;
}

void _sqlite3_field_::LoadFromStr(const Buffer& str){
	switch (nType)
	{
	case TYPE_NULL:
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		Value.Integer = atoi(str);
		break;
	case TYPE_REAL:
		Value.Double = atof(str);
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
		*Value.String = str;
		break;
	case TYPE_BLOB:
		*Value.String = StrToHex(str);
		break;
	default:
		TRACEW("type=%d", nType);
		break;
	}
}

Buffer _sqlite3_field_::toEqualExp() const{
	Buffer sql=(Buffer)*this+"=";
	std::stringstream ss;
	switch (nType)
	{
	case TYPE_NULL:
		sql += " NULL ";
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		ss << Value.Integer;
		sql += ss.str() + " ";
		break;
	case TYPE_REAL:
		ss << Value.Double;
		sql += ss.str() + " ";
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		sql += '"' + *Value.String + "\" ";
		break;
	default:
		TRACEW("type=%d", nType);
		break;
	}
	return sql;
}

Buffer _sqlite3_field_::toSqlString() const{
	Buffer sql="";
	std::stringstream ss;
	switch (nType)
	{
	case TYPE_NULL:
		sql += " NULL ";
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		ss << Value.Integer;
		sql += ss.str() + " ";
		break;
	case TYPE_REAL:
		ss << Value.Double;
		sql += ss.str() + " ";
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		sql += '"' + *Value.String + "\" ";
		break;
	default:
		TRACEW("type=%d", nType);
		break;
	}
	return sql;
}

_sqlite3_field_::operator const Buffer() const{
	return '"' + Name + '"';
}

Buffer _sqlite3_field_::StrToHex(const Buffer& data) const  //ʮ�����ַ���ת����ʮ�������ַ���
{
	const char* hex = "0123456789ABCDEF";
	std::stringstream ss;
	for (char ch : data) {//0��askiiֵΪ48 ��0x30  3Ϊ����4λ���� 0Ϊ&0xF����
		ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xF];
	}
	return ss.str();
}

