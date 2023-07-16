#include "Sqlite3Client.h"
#include"Logger.h"

int CSqlite3Client::Connect(const KeyValue& args){
	auto it = args.find("host");  //数据库所在域名or路径
	if (it == args.end()) return -1;
	if (m_db != NULL) return -2;  //说明已经连接 不能反复连接
	int ret= sqlite3_open(it->second, &m_db);//
	if (ret != 0) {
		TRACEE("connect failed ret=%d msg[%s]", ret, sqlite3_errmsg(m_db));
		return -3;
	}

	return 0;
}

int CSqlite3Client::Exec(const Buffer& sql){
	if (m_db == NULL) return -1;//未初始化
	int ret = sqlite3_exec(m_db,sql,NULL,this,NULL);  //将m_db和sql语句传入
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

int CSqlite3Client::StartTransaction(){//开启事务
	if (m_db == NULL) return -1;
	int ret = sqlite3_exec(m_db, "BEGIN TRANSACTION", 0, 0, NULL); //执行开启事务
	if (ret != SQLITE_OK) {
		TRACEE("BEGIN TRANSACTION");
		TRACEE("exec failed ret=%d msg[%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

int CSqlite3Client::CommitTransaction(){//提交事务
	if (m_db == NULL) return -1;
	int ret = sqlite3_exec(m_db, "COMMIT TRANSACTION", 0, 0, NULL); //执行提交事务
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
		//从基类指针转换为子类指针 然后*解引用转换为子类实例
		FieldDefine.push_back(field);
		Fields.insert({ field->Name, field });
	}
}

Buffer _sqlite3_table_::Create(){
	//CREATE TABLE IF NOT EXISTS 表名称 【数据库.表名】 (列定义 , ......);  不存在就创建
	//表全名 = 数据库.表名
	Buffer sql = "CREATE TABLE IF NOT EXISTS "+(Buffer)(*this)+"(\r\n";
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (i > 0) {//列之间用逗号分割
			sql += ",";
		}
		sql += FieldDefine[i]->Create();  //每个列的sql语句
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
	// INSERT INTO 表全名 (列1，列2....列n)
	//VALUE (值1，值2...值n)
	Buffer sql="INSERT INTO "+(Buffer)*this+" (";
	bool isFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++){
		if (values.FieldDefine[i]->Condition & SQL_INSERT) {//只考虑用到的列 没用到的列不需要负责
			if (!isFirst) sql += ",";
			else isFirst = false;
			sql += (Buffer)(*values.FieldDefine[i]);//列全名
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
	//DELETE FROM 表全名 WHERE 条件   (Buffer)*this表示表全名
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
	// UPDATE 表全名 SET 列1=值1，......列n=值n; [WHERE 条件]
	Buffer sql = "UPDATE " + (Buffer)*this + " SET ";
	bool isFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_MODIFY) {//只考虑用到的列 没用到的列不需要负责
			if (!isFirst) sql += ",";
			else isFirst = false;
			sql += (Buffer)(*values.FieldDefine[i])+"="+ values.FieldDefine[i]->toSqlString();//列全名
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
	//  SELECT 列名1，列名2，...，列名n FROM 表全名；
	Buffer sql = "SELECT ";
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (i > 0) {
			sql += ',';
		}
		sql += '"' + FieldDefine[i]->Name + "\" ";
	}
	sql += " FROM " + (Buffer)*this + " ";  //from 表全名
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
	if (Database.size()) {//已指定数据库
		Head = '"' + Database + "\".";  //  \表示转义字符
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
	//“名称” 类型 属性
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

Buffer _sqlite3_field_::StrToHex(const Buffer& data) const  //十进制字符串转换成十六进制字符串
{
	const char* hex = "0123456789ABCDEF";
	std::stringstream ss;
	for (char ch : data) {//0的askii值为48 即0x30  3为右移4位所得 0为&0xF所得
		ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xF];
	}
	return ss.str();
}

