#pragma once
#include"Server.h"
#include<map>
#include"Logger.h"
#include"HttpParser.h"
#include"Crypto.h"
#include"MysqlClient.h"
#include"json/json.h"
//数据库操作
DECLARE_TABLE_CLASS(myLogin_user_mysql, _mysql_table_)
DECLARE__MYSQL_FIELD(TYPE_INT,user_id, NOT_NULL | PRIMARY_KEY |AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE__MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")
DECLARE__MYSQL_FIELD(TYPE_VARCHAR, user_phone, DEFAULT, "VARCHAR","(11)", "'18888888888'", "") 
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_name, NOT_NULL, "TEXT", "", "", "") 
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_nick, NOT_NULL, "TEXT", "","", "") 
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_wechat, DEFAULT, "TEXT","", "NULL", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_wechat_id, DEFAULT, "TEXT","", "NULL", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_address, DEFAULT, "TEXT","", "", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_province, DEFAULT, "TEXT","", "", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_country, DEFAULT, "TEXT","", "", "")
DECLARE__MYSQL_FIELD(TYPE_INT, user_age, DEFAULT | CHECK, "INTEGER", "", "18", "")
DECLARE__MYSQL_FIELD(TYPE_INT, user_male, DEFAULT, "BOOL", "", "1", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_flags, DEFAULT, "TEXT", "","0", "")
DECLARE__MYSQL_FIELD(TYPE_REAL,user_experience, DEFAULT,"REAL", "", "0.0", "")
DECLARE__MYSQL_FIELD(TYPE_INT, user_level, DEFAULT | CHECK, "INTEGER", "", "0", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_class_priority, DEFAULT, "TEXT", "", "", "")
DECLARE__MYSQL_FIELD(TYPE_REAL, user_time_per_viewer, DEFAULT, "REAL", "", "", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_career, NONE, "TEXT", "","", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_password, NOT_NULL, "TEXT","", "", "")
DECLARE__MYSQL_FIELD(TYPE_INT, user_birthday, NONE, "DATETIME","", "", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_describe, NONE, "TEXT", "","", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_education, NONE, "TEXT","", "", "")
DECLARE__MYSQL_FIELD(TYPE_INT, user_register_time, DEFAULT,"DATETIME", "", "LOCALTIME()", "")
DECLARE_TABLE_CLASS_END()



//1、客户端地址问题 需要recv 和 send
//2、连接回调参数问题
//3、接收回调参数问题
#define ERR_RETURN(ret,err) if(ret!=0){TRACEE("ret = %d errno = %d mst = [%s]", ret, errno, strerror(errno));return err;}
#define WARN_CONTINUE(ret) if(ret!=0){TRACEE("ret = %d errno = %d mst = [%s]", ret, errno, strerror(errno));continue;}

class CPlayerServer :public CBusiness{
public:
	CPlayerServer(unsigned count) :CBusiness(),m_count(count) {
	}
	~CPlayerServer() {
		if (m_db != NULL) {
			CDataBaseClient* tmp = m_db;
			m_db = NULL;
			delete tmp;
		}
		m_epoll.Close();
		m_pool.Close();
		for (auto it : m_mapClients) {
			if (it.second != NULL) {
				delete it.second;
			}
		}
		m_mapClients.clear();
	}
	virtual int CBusinessProcess(CProcess* proc) {
		using namespace std::placeholders;//占位符  表示此时该位置的值尚未设置  但要预留该位置 
		m_db = new CMysqlClient();
		if (m_db == NULL) {
			TRACEE("no more memory");
			return -1;
		}
		int ret = 0;
		KeyValue args;
		args["host"] = "10.164.241.100";
		args["user"] = "test";
		args["password"] = "123456";
		args["port"] = "3306";
		args["db"] = "test";

		ret = m_db->Connect(args);//连接数据库
		if (ret != 0) {
			ERR_RETURN(ret, -2);
		}
		myLogin_user_mysql user;
		ret = m_db->Exec(user.Create());//不存在就创建 存在就不创建
		ERR_RETURN(ret, -3);
		ret = SetConnectCallback(&CPlayerServer::Connected, this, _1);
		ERR_RETURN(ret, -4);
		ret = SetRecvCallback(&CPlayerServer::Received, this, _1, _2);
		ERR_RETURN(ret, -5);
		ret = m_epoll.Create(m_count);
		ERR_RETURN(ret, -6);
		ret = m_pool.Start(m_count);
		ERR_RETURN(ret, -7);
		for (unsigned i = 0; i < m_count; i++) {
			ret = m_pool.AddTask(&CPlayerServer::ThreadFunc,this);  //this_call类型的函数，需要将this指针传入
			ERR_RETURN(ret, -8);
		}
		int sock = 0;
		sockaddr_in addrin;
		while (m_epoll != -1) {
			ret = proc->RecvSocket(sock,&addrin);  //接收
			TRACEI("Recv sock ret=%d", ret);
			if ((ret < 0)||(sock==0)) {
				break;
			}
			CSocketBase* pClient = new CSocket(sock);
			if (pClient == NULL) continue;
			ret = pClient->Init(CSockParam(&addrin,SOCK_ISIP));
			WARN_CONTINUE(ret);
			ret = m_epoll.Add(sock,EpollData((void*)pClient));
			if (m_connectedcallback) {//
				(*m_connectedcallback)(pClient);
			}
			WARN_CONTINUE(ret);
			m_mapClients[sock] = pClient;
		}
		return 0;
	}
	Buffer MakeResonse(int ret) {
		Json::Value root;
		root["status"] = ret;
		if (ret != 0) {
			root["message"] = "登录失败，可能是用户名或密码错误!";
		}
		else {
			root["message"] = "Success!";
		}
		Buffer json = root.toStyledString();
		Buffer result="HTTP/1.1 200 OK\r\n";
		time_t t;
		time(&t);
		tm* ptm = localtime(&t);
		char tmp[64] = "";
		strftime(tmp,sizeof(tmp),"%a, %d %b %G %T GMT\r\n",ptm);//格式化字符串
		Buffer Date = Buffer("Date: ") + tmp;
		Buffer Server = "Server: Edoyun/1.0\r\nContent-Type: text/html; charset=utf-8\r\nX-Frame-Options:DENY\r\n";
		snprintf(tmp, sizeof(tmp), "%d",json.size());
		Buffer Length = Buffer("Content-Length: ") + tmp + "\r\n";
		Buffer Stub = "X-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n\r\n";
		result += Date + Server + Length + Stub + json;
		TRACEI("resopnse: %s", (char*)result);
		return result;
	}
private:
	int ThreadFunc() {
		int ret = 0;
		EPEvents events;
		while (m_epoll != -1) {  //epoll还没关
			ssize_t size = m_epoll.WaitEvents(events);
			if (size < 0) break;
			if (size > 0) {
				for (ssize_t i = 0; i < size; i++) {
					if (events[i].events & EPOLLERR) {
						break;
					}
					else if(events[i].events&EPOLLIN){  //只能是客户端
						CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;  //events[i].data.ptr是void*类型
						if (pClient) {
							Buffer data;
							ret = pClient->Recv(data);
							TRACEI("Recv data size %d", ret);
							if (ret <= 0) { 
								TRACEE("ret = %d errno = %d mst = [%s]", ret, errno, strerror(errno)); 
								m_epoll.Delete(*pClient);
								continue; 
							}
							(*m_SetRecvcallback)(pClient,data);
						}
					}
				}
			}
		}
		return 0;
	}
private:
	int Connected(CSocketBase* pClient) {
		//TODO:客户端连接处理  做简单打印即可
		struct sockaddr_in* pAddr = *pClient;
		TRACEI("%s(%d):[%s],Client connected port:%d  addr:%s\n",__FILE__,__LINE__,__FUNCTION__, pAddr->sin_port, inet_ntoa(pAddr->sin_addr));
		return 0;
	}
	int Received(CSocketBase* pClient, const Buffer& data) {
		//TODO:1、客户端主要业务在此处理  2、HTTP 解析 3、数据库 查询  4、登录请求的验证  5、验证结果的反馈
		//HTTP解析
		TRACEI("接收到数据");
		int ret = 0;
		Buffer response = "";
		ret = HttpParser(data);
		TRACEI("Http Parser ret=%d",ret);
		if (ret != 0) {//验证失败
			TRACEE("%s(%d):[%s],HttpParser Failed ret=%d", __FILE__, __LINE__, __FUNCTION__, ret);
		} 
		response = MakeResonse(ret);//成功or失败的应答
		ret = pClient->Send(response);
		if (ret != 0) {
			TRACEE("%s(%d):[%s],Send Resopnse Failed ret=%d response:%s", __FILE__, __LINE__, __FUNCTION__, ret,(char*)response);
		}
		else {
			TRACEI("%s(%d):[%s],Send Resopnse Succeed", __FILE__, __LINE__, __FUNCTION__);
		}
		return 0;
	}
	int HttpParser(const Buffer& data) {//Http解析模块
		CHttpParser parser;
		size_t size = parser.Parser(data);
		if (size == 0 || (parser.Errno() != 0)) {
			TRACEE("%s(%d):[%s],HttpParser failed size=%ld,errno=%d", __FILE__, __LINE__, __FUNCTION__, size, parser.Errno());
		}
		if (parser.Method() == HTTP_GET) {
			//GET处理
			CUrlParser url("https://10.164.241.100"+parser.URL());
			int ret = url.Parser();
			if (ret != 0) {
				TRACEE("%s(%d):[%s],URL parser failed ret=%d url:[%s]", __FILE__, __LINE__, __FUNCTION__, ret, (char*)"https://10.164.241.100" + parser.URL());
				return -2;
			}
			Buffer uri = url.Uri();
			//URI，统一资源标志符(Uniform Resource Identifier)，表示的是web上每一种可用的资源，如 HTML文档、图像、视频片段、程序等都由一个URI进行唯一标识的。
			if (uri == "login") {//登录
				Buffer time = url["time"];
				Buffer salt = url["salt"];
				Buffer user = url["user"];
				Buffer sign = url["sign"];
				TRACEI("%s(%d):[%s]  url parser result: time=%s salt=%s user=%s sign=%s", (char*)time, (char*)salt, (char*)user, (char*)sign);
				//TODO:数据库查询 
				myLogin_user_mysql db_user;
				Result result;
				Buffer sql = db_user.Query("user_name=\"" + user + "\"");
				ret = m_db->Exec(sql,result,db_user);
				if (ret != 0) {
					TRACEE("exec failed sql=[%s] ret=%d", (char*)sql, ret);
					return -3;
				}
				if (result.size() == 0) {//无查询结果
					TRACEE("no result sql=[%s] ret=%d", (char*)sql, ret);
					return -4;
				}
				if (result.size() != 1) {//查询到多个结果
					TRACEE("more than one result sql=[%s] ret=%d", (char*)sql, ret);
					return -5;
				}
				//正常查询到结果
				auto user1 = result.front();
				Buffer pwd = *user1->Fields["user_password"]->Value.String;
				TRACEI("user password=%s", (char*)pwd);
				//验证登录
				const char* MD5_KEY = "*&^%$#@b.v+h-b*g/h@n!h#n$d^ssx,.kl<kl";
				Buffer md5str = time + MD5_KEY + pwd + salt;
				Buffer md5=Crypto::MD5(md5str);
				TRACEI("md5 = % s", (char*)md5);
				if (md5 != sign) {
					return -6;
				}
				return 0;//验证成功
			}
		}
		else if (parser.Method() == HTTP_POST) {
			//POST处理
		}
		return -7;
	}
private:
	CEpoll m_epoll;
	ThreadPool m_pool;
	std::map<int, CSocketBase*> m_mapClients;
	unsigned m_count;
	CDataBaseClient* m_db;
};

