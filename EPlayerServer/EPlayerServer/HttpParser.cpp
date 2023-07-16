#include "HttpParser.h"
CHttpParser::CHttpParser(){
	m_complete = false;
	memset(&m_parser, 0, sizeof(m_parser));
	m_parser.data = this;  
	//��data�ᱻ�����ص������� ��ʱ��ʹ��thisָ����г�Ա��������
	http_parser_init(&m_parser,HTTP_REQUEST); 
	//�����ֻ���յ����Կͻ��˵����� 
	//���ǿͻ��ˣ���ֻ���յ�����˵�Ӧ�� ΪRESOPNSE
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
	m_parser.data = this;  //��ǰ����ָ��
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
		m_parser.data = this;  //��ǰ����ָ��
		memcpy(&m_settings, &http.m_settings, sizeof(m_settings));
		m_status = http.m_status;
		m_url = http.m_url;
		m_body = http.m_body;
		m_complete = http.m_complete;
		m_lastField = http.m_lastField;
	}
	return *this;
}

size_t CHttpParser::Parser(const Buffer& data){//����
	m_complete = false;
	size_t ret = http_parser_execute(
		&m_parser,&m_settings,data,data.size());
	if (m_complete == false) {//û�н�����
		m_parser.http_errno = 0x7F;
		return 0;  //����0��ʾû�����꣬�����������ó�127������
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
//TODO: ��Ҫ�����Ĳ����У�Э�顢�����Ͷ˿ڡ�URI
	const char* pos = m_url;//������¼ָ��
	const char* target=strstr(pos, "://");
	if (target == NULL) return -1;//û�ҵ�
	m_protocol = Buffer(pos, target);  //Э�����
	pos = target + 3;
	target = strchr(pos, '/');//������
	if (target == NULL) {
		if ((m_protocol.size() + 3) >= m_url.size())
			return -2;
		m_host = pos;  //????
	}
	Buffer value = Buffer(pos, target);//�����������Ͷ˿ڵ�buffer
	if (value.size() == 0) return -3;
	target = strchr(pos, ':');//�Ҷ˿�
	if (target != NULL) {//�ж˿�
		m_host = Buffer(pos, target);
		m_port = atoi(Buffer(target + 1, pos + value.size()));
	}
	else {//�����ڶ˿�
		m_host = value;
	}
	//����URI
	pos = strchr(pos,'/');//Ϊʲô����1������
	target = strchr(pos, '?');
	if (target == NULL) {
		m_uri = pos+1;
		return 0;
	}
	else {
		m_uri = Buffer(pos+1, target);
		//�еĻ��ſ�ʼ����key value
		pos = target + 1;
		do {
			target = strchr(pos, '&');
			const char* t=NULL;
			if (target == NULL) {
				t = strchr(pos, '=');
				if (t == NULL) return -4;
				m_values[Buffer(pos, t)] = Buffer(t + 1);//t+1��ȫ����value
				break;  //���һ����ֵ��
			}
			else {
				Buffer kv = Buffer(pos, target);
				t = strchr(pos, '=');
				if (t == NULL) {//������key value��ֵ�� ��ʾ�������
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
	if (it==m_values.end()) {//δ�ҵ�
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
	//��������url֮����Ҫ���»�ȡ��Ӧ����
	m_url = url;
	m_protocol = "";
	m_host = "";
	m_port = 80;
	m_values.clear();
	return 0;
}
