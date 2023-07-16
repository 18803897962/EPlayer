#include "MysqlClient.h"
#include<sstream>
#include"Logger.h"
int CMysqlClient::Connect(const KeyValue& args){	
	if (m_bInit == true) return -1;//已经初始化
	MYSQL* ret =  mysql_init(&m_db);//初始化
	if (ret == NULL) return -2;
	ret = mysql_real_connect(&m_db,args.at("host"), 
		args.at("user"), args.at("password"), args.at("db"), atoi(args.at("port")), NULL, 0
	);
	if ((ret == NULL) && (mysql_errno(&m_db) != 0)) {  //???为什么不是 || ？？？
		printf("%s(%d):[%s]  errno=%d  msg:[%s]\n", __FILE__,__LINE__,__FUNCTION__,mysql_errno(&m_db),mysql_error(&m_db));
		TRACEE("errno=%d  msg=[%s]", mysql_errno(&m_db), mysql_error(&m_db));
		mysql_close(&m_db);
		bzero(&m_db, sizeof(m_db));
		return -3;
	}
	m_bInit = true;
	return 0;
}

int CMysqlClient::Exec(const Buffer& sql){
	if (m_bInit == false) return -1;//未初始化
	int ret = mysql_real_query(&m_db, sql, sql.size());
	if (ret != 0) {
		printf("%s(%d):[%s]  errno=%d  msg:[%s]\n", __FILE__, __LINE__, __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("errno=%d  msg=[%s]", mysql_errno(&m_db), mysql_error(&m_db));

		return -2;
	}
	return 0;
}

int CMysqlClient::Exec(const Buffer& sql, Result& result, const _Table_& table){
	if (m_bInit == false) return -1;//未初始化
	int ret = mysql_real_query(&m_db, sql, sql.size());
	if (ret != 0) {
		printf("%s(%d):[%s]  errno=%d  msg:[%s]\n", __FILE__, __LINE__, __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("errno=%d  msg=[%s]", mysql_errno(&m_db), mysql_error(&m_db));

		return -2;
	}
	MYSQL_RES* res = mysql_store_result(&m_db);//取结果
	MYSQL_ROW row;
	unsigned num_fields = mysql_num_fields(res);//获取列数
	while ((row = mysql_fetch_row(res)) != NULL) {
		PTable pt = table.Copy();
		for (unsigned i; i < num_fields; i++) {
			if (row[i] != NULL) {
				pt->FieldDefine[i]->LoadFromStr(row[i]);
			}
		}
		result.push_back(pt);
	}
	return 0;
}

int CMysqlClient::StartTransaction(){
	if (m_bInit == false) return -1;//未初始化
	int ret = mysql_real_query(&m_db, "BEGIN", 6);
	if (ret != 0) {
		printf("%s(%d):[%s]  errno=%d  msg:[%s]\n", __FILE__, __LINE__, __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("errno=%d  msg=[%s]", mysql_errno(&m_db), mysql_error(&m_db));

		return -2;
	}
	return 0;
}

int CMysqlClient::CommitTransaction(){
	if (m_bInit == false) return -1;//未初始化
	int ret = mysql_real_query(&m_db, "COMMIT", 7);
	if (ret != 0) {
		printf("%s(%d):[%s]  errno=%d  msg:[%s]\n", __FILE__, __LINE__, __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("errno=%d  msg=[%s]", mysql_errno(&m_db), mysql_error(&m_db));

		return -2;
	}
	return 0;
}

int CMysqlClient::RollbackTransaction(){
	if (m_bInit == false) return -1;//未初始化
	int ret = mysql_real_query(&m_db, "ROLLBACK", 9);
	if (ret != 0) {
		printf("%s(%d):[%s]  errno=%d  msg:[%s]\n", __FILE__, __LINE__, __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("errno=%d  msg=[%s]", mysql_errno(&m_db), mysql_error(&m_db));

		return -2;
	}
	return 0;
}

int CMysqlClient::Close(){
	if (m_bInit) {
		mysql_close(&m_db);
		bzero(&m_db, sizeof(m_db));
		m_bInit = false;
	}
	return 0;
}

bool CMysqlClient::IsConnected(){
	return m_bInit;
}

_mysql_table_::_mysql_table_(const _mysql_table_& table){
	Database = table.Database;
	Name = table.Name;
	for (size_t i = 0; i < table.Fields.size(); i++) {
		PField field = PField(new _mysql_field_(*(_mysql_field_*)table.FieldDefine[i].get()));
		//从基类指针转换为子类指针 然后*解引用转换为子类实例
		FieldDefine.push_back(field);
		Fields.insert({ field->Name, field });
	}
}

Buffer _mysql_table_::Create(){
	//CREATE TABLE IF NOT EXISTS 表全名 (列定义，...)  
	//PRIMARY KEY `主键列名` ，UNIQUE INDEX 列名 UNIQUE 列名 VISIBLE )

	Buffer sql = "CREATE TABLE IF NOT EXISTS " + (Buffer)*this + " (\r\n";
	for (unsigned i = 0; i < FieldDefine.size(); i++) {
		if (i > 0) sql += ",\r\n";
		sql += FieldDefine[i]->Create();
		if (FieldDefine[i]->Attr & PRIMARY_KEY) {//有主键睡醒
			sql += ",\r\nPRIMARY KEY (`" + FieldDefine[i]->Name + "`) ";
		}
		if (FieldDefine[i]->Attr & UNIQUE) {
			sql += ",UNIQUE INDEX `" + FieldDefine[i]->Name + "_UNIQUE` (";
			sql += (Buffer)*FieldDefine[i]+" ASC) VISIBLE ";
		}
	}
	sql += ");";
	return sql;
}

Buffer _mysql_table_::Drop(){
	return "DROP TABLE " + (Buffer)*this+";";
}

Buffer _mysql_table_::Insert(const _Table_& values){
	//INSERT INTO 表名 (列名...)VALUES(值...)
	Buffer sql = "INSERT INTO " + (Buffer)*this + " (";
	bool isFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
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
	printf("sql=%s\r\n", (char*)sql);
	return sql;
}

Buffer _mysql_table_::Delete(const _Table_& values){
	Buffer sql = "DELETE FROM " + (Buffer)*this;
	Buffer Where = "";
	bool isFirst = true;
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (FieldDefine[i]->Condition & SQL_CONSITION) {
			if (!isFirst) Where += " AND ";
			else isFirst = false;
			Where += (Buffer)*FieldDefine[i] + "=" + FieldDefine[i]->toSqlString();
		}
	}
	if (Where.size() > 0) {
		sql += " WHERE" + Where + " ";
	}
	sql += ";";
	printf("sql=%s\r\n", (char*)sql);
	return sql;
}

Buffer _mysql_table_::Modify(const _Table_& values){
	Buffer sql = "UPDATE " + (Buffer)*this + " SET ";
	bool isFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_MODIFY) {//只考虑用到的列 没用到的列不需要负责
			if (!isFirst) sql += ",";
			else isFirst = false;
			sql += (Buffer)(*values.FieldDefine[i]) + "=" + values.FieldDefine[i]->toSqlString();//列全名
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
		sql += " WHERE" + Where + " ";
	}
	sql += ";";
	printf("sql=%s\r\n", (char*)sql);
	return sql;
}

Buffer _mysql_table_::Query(const Buffer& condition){
	Buffer sql = "SELECT ";
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (i > 0) {
			sql += ',';
		}
		sql += '`' + FieldDefine[i]->Name + "` ";
	}
	sql += " FROM " + (Buffer)*this + " ";  //from 表全名
	if (condition.size() > 0) {
		sql += "WHERE" + condition;
	}
	sql += ";";
	printf("sql=%s\r\n", (char*)sql);
	return sql;
}

PTable _mysql_table_::Copy() const{
	return PTable(new _mysql_table_(*this));
}

void _mysql_table_::ClearFieldUsed(){
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		FieldDefine[i]->Condition = 0;
	}
}

_mysql_table_::operator const Buffer() const{
	Buffer Head;
	if (Database.size()) {//已指定数据库
		Head = '`' + Database + "`.";  //  \表示转义字符
	}
	return Head + '`' + Name + '`';

}

_mysql_field_::_mysql_field_() :_Field_() {
	nType = TYPE_NULL;
	//Value.Integer = 0;
	Value.Double = 0.0;
}

_mysql_field_::_mysql_field_(int ntype, const Buffer& name, unsigned attr, const Buffer& type,
	const Buffer& size, const Buffer& default_, const Buffer& check){
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

_mysql_field_::_mysql_field_(const _mysql_field_& field){
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

_mysql_field_::~_mysql_field_(){
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

Buffer _mysql_field_::Create(){
	Buffer sql="`"+Name+"` "+Type+Size+" ";
	if (Attr & NOT_NULL) {
		sql += "NOT NULL";
	}
	else {
		sql += "NULL";
	}
	//BLOB TEXT GEOMETRY JSON类型不能有默认值
	if ((Attr & DEFAULT) && (Default.size() > 0)&&
		Type != "BLOB"&& Type != "TEXT"&&
		Type != "GEOMETRY"&& Type != "json") {//不能有默认值
		sql += " DEFAULT \"" + Default + "\" "; 
	}//默认值添加
	//UNIQUE PRIMARY由外面处理 CHECK约束条件不支持
	if (Attr & AUTOINCREMENT) {
		sql += " AUTO_INCREMENT ";
	}
	return sql;
}

void _mysql_field_::LoadFromStr(const Buffer& str){
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
		printf("type=%d\n", nType);
		break;
	}
}

Buffer _mysql_field_::toEqualExp() const{
	Buffer sql = (Buffer)*this + "=";
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
		printf("type=%d\n", nType);
		break;
	}
	return sql;
}

Buffer _mysql_field_::toSqlString() const{
	Buffer sql = "";
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
		printf("type=%d\n", nType);
		break;
	}
	return sql;
}

_mysql_field_::operator const Buffer() const{
	return '`' + Name + '`';
}

Buffer _mysql_field_::StrToHex(const Buffer& data) const{
	const char* hex = "0123456789ABCDEF";
	std::stringstream ss;
	for (char ch : data) {//0的askii值为48 即0x30  3为右移4位所得 0为&0xF所得
		ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xF];
	}
	return ss.str();
}
