#include "HttpParser.h"
CHttpParser::CHttpParser(){
	m_complete = false;
	memset(&m_parser, 0, sizeof(m_parser));
	m_parser.data = this;  
	//此data会被传到回调函数中 届时可使用this指针进行成员函数调用
	http_parser_init(&m_parser,HTTP_REQUEST); 
	//服务端只能收到来自客户端的请求 
	//若是客户端，则只能收到服务端的应答 为RESOPNSE
	memset(&m_settings, 0, sizeof(m_settings));
	m_settings.on_message_begin = &CHttpParser::OnMessageBegin;
	m_settings.on_url = &CHttpParser::OnUrl;
	m_settings.on_status = CHttpParser::OnStatus;
	m_settings.on_header_field = &CHttpParser::OnHeaderField;
	m_settings.on_header_value = &CHttpParser::OnHeaderValue;
	m_settings.on_headers_complete = &CHttpParser::OnHeaderComplete;
	m_settings.on_body = &CHttpParser::OnBody;
	m_settings.on_message_complete = &CHttpParser::OnMessageComplete;

}

CHttpParser::~CHttpParser(){}

CHttpParser::CHttpParser(const CHttpParser& http)
{
	memcpy(&m_parser, &http.m_parser, sizeof(m_parser));
	m_parser.data = this;  //当前的类指针
	memcpy(&m_settings, &http.m_settings, sizeof(m_settings));
	m_status = http.m_status;
	m_url = http.m_url;
	m_body = http.m_body;
	m_complete = http.m_complete;
	m_lastField = http.m_lastField;
}

CHttpParser& CHttpParser::operator=(const CHttpParser& http) {
	if (this != &http) {
		memcpy(&m_parser, &http.m_parser, sizeof(m_parser));
		m_parser.data = this;  //当前的类指针
		memcpy(&m_settings, &http.m_settings, sizeof(m_settings));
		m_status = http.m_status;
		m_url = http.m_url;
		m_body = http.m_body;
		m_complete = http.m_complete;
		m_lastField = http.m_lastField;
	}
	return *this;
}

size_t CHttpParser::Parser(const Buffer& data){//解析
	m_complete = false;
	size_t ret = http_parser_execute(
		&m_parser,&m_settings,data,data.size());
	if (m_complete == false) {//没有解析完
		m_parser.http_errno = 0x7F;
		return 0;  //返回0表示没解析完，将错误码设置成127并返回
	}
	return ret;
}

int CHttpParser::OnMessageBegin(http_parser* parser){
	return ((CHttpParser*)parser->data)->OnMessageBegin();
}

int CHttpParser::OnUrl(http_parser* parser, const char* at, size_t length){
	return ((CHttpParser*)parser->data)->OnUrl(at,length);
}

int CHttpParser::OnStatus(http_parser* parser, const char* at, size_t length){
	return ((CHttpParser*)parser->data)->OnStatus(at, length);
}

int CHttpParser::OnHeaderField(http_parser* parser, const char* at, size_t length){
	return ((CHttpParser*)parser->data)->OnHeaderField(at, length);
}

int CHttpParser::OnHeaderValue(http_parser* parser, const char* at, size_t length){
	return ((CHttpParser*)parser->data)->OnHeaderValue(at, length);
}

int CHttpParser::OnHeaderComplete(http_parser* parser){
	return ((CHttpParser*)parser->data)->OnHeaderComplete();
}

int CHttpParser::OnBody(http_parser* parser, const char* at, size_t length){
	return ((CHttpParser*)parser->data)->OnBody(at, length);
}

int CHttpParser::OnMessageComplete(http_parser* parser){
	return ((CHttpParser*)parser->data)->OnMessageComplete();
}

int CHttpParser::OnMessageBegin(){
	return 0;
}

int CHttpParser::OnUrl(const char* at, size_t length){
	m_url = Buffer(at, length);
	return 0;
}

int CHttpParser::OnStatus(const char* at, size_t length){
	m_status = Buffer(at, length);
	return 0;
}

int CHttpParser::OnHeaderField(const char* at, size_t length){
	m_lastField = Buffer(at, length);
	return 0;
}

int CHttpParser::OnHeaderValue(const char* at, size_t length){
	m_headValues[m_lastField] = Buffer(at, length);
	return 0;
}

int CHttpParser::OnHeaderComplete(){
	return 0;
}

int CHttpParser::OnBody(const char* at, size_t length){
	m_body = Buffer(at, length);
	return 0;
}

int CHttpParser::OnMessageComplete(){
	m_complete = true;
	return 0;
}

CUrlParser::CUrlParser(const Buffer& url){
	m_url = url;
}

CUrlParser::~CUrlParser(){
	
}

int CUrlParser::Parser(){
//TODO: 需要解析的部分有：协议、域名和端口、URI
	const char* pos = m_url;//用作记录指针
	const char* target=strstr(pos, "://");
	if (target == NULL) return -1;//没找到
	m_protocol = Buffer(pos, target);  //协议解析
	pos = target + 3;
	target = strchr(pos, '/');//找域名
	if (target == NULL) {
		if ((m_protocol.size() + 3) >= m_url.size())
			return -2;
		m_host = pos;  //????
	}
	Buffer value = Buffer(pos, target);//包含了域名和端口的buffer
	if (value.size() == 0) return -3;
	target = strchr(pos, ':');//找端口
	if (target != NULL) {//有端口
		m_host = Buffer(pos, target);
		m_port = atoi(Buffer(target + 1, pos + value.size()));
	}
	else {//不存在端口
		m_host = value;
	}
	//解析URI
	pos = strchr(pos,'/');//为什么不加1？？？
	target = strchr(pos, '?');
	if (target == NULL) {
		m_uri = pos+1;
		return 0;
	}
	else {
		m_uri = Buffer(pos+1, target);
		//有的话才开始解析key value
		pos = target + 1;
		do {
			target = strchr(pos, '&');
			const char* t=NULL;
			if (target == NULL) {
				t = strchr(pos, '=');
				if (t == NULL) return -4;
				m_values[Buffer(pos, t)] = Buffer(t + 1);//t+1后全是其value
				break;  //最后一个键值对
			}
			else {
				Buffer kv = Buffer(pos, target);
				t = strchr(pos, '=');
				if (t == NULL) {//不符合key value键值对 表示情况不对
					return -5;
				}
				m_values[Buffer(pos, t)] = Buffer(t + 1, pos + kv.size());
				pos = target + 1;
			}
		} while (target != NULL);
	}
	return 0;
}

Buffer CUrlParser::operator[](const Buffer& name) const {
	std::map<Buffer, Buffer>::const_iterator it = m_values.find(name);
	if (it==m_values.end()) {//未找到
		return Buffer();
	}
	return it->second;
}

Buffer CUrlParser::Protocol() const{
	return m_protocol;
}

Buffer CUrlParser::Host() const
{
	return m_host;
}

int CUrlParser::Port() const
{
	return m_port;
}

int CUrlParser::SetUrl(const Buffer& url){
	//重新设置url之后需要重新获取相应数据
	m_url = url;
	m_protocol = "";
	m_host = "";
	m_port = 80;
	m_values.clear();
	return 0;
}
