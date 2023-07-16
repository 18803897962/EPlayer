#pragma once
#include "http_parser.h"
#include"Public.h"
#include<map>
class CHttpParser {
public:
	CHttpParser();
	~CHttpParser();
public:
	CHttpParser(const CHttpParser& http);
	CHttpParser& operator=(const CHttpParser& http);
public:
	size_t Parser(const Buffer& data);
	//GET POST ... 参考http_parser.h的HTTP_METHOD_MAP宏
	unsigned Method()const { return m_parser.method; }
	const std::map<Buffer, Buffer>& Headers(){ return m_headValues; }
	const Buffer& Status() const { return m_status; }
	const Buffer& URL() const { return m_url; }
	const Buffer& Body() const { return m_body; }
	unsigned Errno() const { return m_parser.http_errno; }
protected:
	static int OnMessageBegin(http_parser* parser);
	static int OnUrl(http_parser* parser, const char* at, size_t length);
	static int OnStatus(http_parser* parser, const char* at, size_t length);
	static int OnHeaderField(http_parser* parser, const char* at, size_t length);
	static int OnHeaderValue(http_parser* parser, const char* at, size_t length);
	static int OnHeaderComplete(http_parser* parser);
	static int OnBody(http_parser* parser, const char* at, size_t length);
	static int OnMessageComplete(http_parser* parser);

	int OnMessageBegin();
	int OnUrl(const char* at, size_t length);
	int OnStatus(const char* at, size_t length);
	int OnHeaderField(const char* at, size_t length);
	int OnHeaderValue(const char* at, size_t length);
	int OnHeaderComplete();
	int OnBody(const char* at, size_t length);
	int OnMessageComplete();

private:
	http_parser m_parser;  //http内容
	http_parser_settings m_settings;//回调函数
	std::map<Buffer, Buffer> m_headValues;//键值对   //拷贝构造函数不用拷贝吗？？？
	Buffer m_status;
	Buffer m_url;
	Buffer m_body;
	bool m_complete;
	Buffer m_lastField;
};


class CUrlParser {
public:
	CUrlParser(const Buffer& url);
	~CUrlParser();
public:
	int Parser();
	Buffer operator[](const Buffer& name) const;//键值对 通过key获取其对应的value
	Buffer Protocol() const;
	Buffer Host() const;
	int Port() const;//默认返回80  HTTP是80端口
	int SetUrl(const Buffer& data);
	Buffer const Uri()const { return m_uri; }
private:
	Buffer m_url;
	Buffer m_protocol;
	Buffer m_host;
	Buffer m_uri;
	int m_port;
	std::map<Buffer, Buffer> m_values;
};