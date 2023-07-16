#pragma once
#include<unistd.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string>
#include<fcntl.h>
#include"Public.h"

enum SockAttr {
	SOCK_ISSERVER = 1,  //是否是服务器 1表示是 0表示不是
	SOCK_ISNONBLOCK = 2,//是否阻塞 1表示非阻塞 0表示阻塞
	SOCK_ISUDP = 4,//是否是UDP  1表示是UDP 0表示是TCP
	SOCK_ISIP=8,//是否是IP协议 1表示IP协议 0表示本地套接字
	SOCK_ISREUSE=16//是否重用地址
};
class CSockParam {
public:
	CSockParam() {
		bzero(&addr_in, sizeof(addr_in));
		bzero(&addr_un, sizeof(addr_un));
		port = -1;
		attr = 0;//默认客户端 阻塞 TCP
	};
	CSockParam(const Buffer& ip, short port, int attr) {//网络套接字
		this->IP = ip;
		this->port = port;//host to net short 主机字节序转为网络字节序
		this->attr = attr;
		addr_in.sin_family = AF_INET;  //PF_INET 协议族 用于套接字初始化时  AF_INET地址族 用于addr赋值
		addr_in.sin_port = htons(port);
		addr_in.sin_addr.s_addr = inet_addr(ip);
	}
	CSockParam(const Buffer& path, int attr) {
		IP = path;//使用IP来承接路径
		addr_un.sun_family = AF_UNIX;
		memcpy(addr_un.sun_path, (const char*)path, sizeof(addr_un.sun_path));
		this->attr = attr;
	}
	CSockParam(const sockaddr_in* addrin, int attr) {//网络套接字
		this->IP = IP;
		this->port = port;
		this->attr = attr;  //???????
		memcpy(&addr_in,addrin,sizeof(sockaddr_in));
	}
	~CSockParam() {};
	CSockParam(const CSockParam& param) {
		IP = param.IP;
		port = param.port;
		attr = param.attr;
		memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
		memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
	}
public:
	CSockParam& operator=(const CSockParam& param) {
		if (this != &param) {
			IP = param.IP;
			port = param.port;
			attr = param.attr;
			memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
			memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
		}
		return *this;
	}
	sockaddr* addrin() {
		return (sockaddr*)&addr_in;
	}
	sockaddr* addrun() {
		return (sockaddr*)&addr_un;
	}
public:
	//地址
	sockaddr_in addr_in;//网络套接字
	sockaddr_un addr_un;//本地套接字
	Buffer IP;//IP
	short port;//端口
	int attr;//属性 参考SockAttr

};

class CSocketBase {
public:
	CSocketBase():m_socket(-1),m_status(0) {}
	virtual ~CSocketBase() {
		Close();
	}//用来传递析构操作
public:
	//服务器：套接字创建  bind listen   客户端 套接字创建
	//初始化  
	virtual int Init(const CSockParam& param) = 0;
	//服务器 accept 客户端 connect  udp可以忽略
	//连接
	virtual int Link(CSocketBase** pclient = NULL) = 0;
	//发送数据
	virtual int Send(const Buffer& data) = 0;
	//接收数据
	virtual int Recv(Buffer& data) = 0;
	//关闭连接
	virtual int Close() {
		m_status = 3;
		if (m_socket != -1) {
			if ((m_param.attr & SOCK_ISSERVER)//服务器 
				&& ((m_param.attr & SOCK_ISIP) == 0))//非IP
				unlink(m_param.IP);  //本地套接字close需要解除链接 网络套接字无需解除
			int fd = m_socket;
			m_socket = -1;
			close(fd);
		}
		return 0;
	}
	virtual operator int() {
		return m_socket;
	}
	virtual operator int() const  {
		return m_socket;
	}
	virtual operator sockaddr_in*() {
		return (sockaddr_in*)&m_param.addr_in;
	}
	virtual operator sockaddr_in* () const {
		return (sockaddr_in*)&m_param.addr_in;
	}
protected:
	//套接字描述符 默认-1
	int m_socket;
	//状态  0未初始化  1已初始化 2已连接  3已关闭
	int m_status;
	//初始化参数
	CSockParam m_param;
};

class CSocket:public CSocketBase {
public:
	CSocket() {

	}
	CSocket(int sock):CSocketBase() {
		m_socket = sock;
	}
	virtual ~CSocket() {
		Close();
	}
public:
	//服务器：套接字创建  bind listen   客户端 套接字创建
	//初始化  
	virtual int Init(const CSockParam& param) {
		if (m_status != 0) return -1;
		m_param = param;
		int type = (param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;
		if (m_socket == -1) {
			if (m_param.attr & SOCK_ISIP) {  //网络套接字
				m_socket = socket(PF_INET, type, 0);
			}
			else {
				m_socket = socket(PF_LOCAL, type, 0);//本地套接字
			}
		}
		else
			m_status = 2;  //accept来的套接字 已经处于连接状态
		if (m_socket == -1) {
			return -2;
		}
		int ret = 0;
		if (m_param.attr & SOCK_ISREUSE) {  //需要地址重用 就删除之前的套接字文件 更新当前的套接字文件
			int option = 1;
			ret = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
			if (ret == -1) return -3;
		}
		if (m_param.attr & SOCK_ISSERVER) {//是否是服务器 服务器需要bind 和 listen
			if (m_param.attr & SOCK_ISIP) {  //网络套接字
				ret = bind(m_socket, m_param.addrin(), sizeof(sockaddr_in));
				printf("%s(%d)%s,m_sock=%d  ret=%d", __FILE__, __LINE__, __FUNCTION__, m_socket,ret);

			}
			else {
				ret = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
				printf("%s(%d)%s,m_sock=%d  ret=%d", __FILE__, __LINE__, __FUNCTION__, m_socket, ret);
			}
			if (ret == -1) return -4;
			ret = listen(m_socket, 32);
			if (ret == -1) return -5;
		}
		//客户端只需要创建套接字  
		if (m_param.attr & SOCK_ISNONBLOCK) {//非阻塞状态
			int option = fcntl(m_socket, F_GETFL);  //取当前状态
			if (option == -1) return -6;
			option |= O_NONBLOCK;//添加非阻塞状态
			ret = fcntl(m_socket, F_SETFL, option);//设置非阻塞状态
			if (ret == -1) return -7;
		}
		
		if(m_status==0)
			m_status = 1;
		return 0;

	}
	//服务器 accept 客户端 connect  udp可以忽略
	//连接
	virtual int Link(CSocketBase** pclient = NULL) {
		if (m_status <= 0 || m_socket == -1) return -1; 
		int ret = 0;
		if (m_param.attr & SOCK_ISSERVER) {//服务器 accept
			if (pclient == NULL) return -2;//接收到的描述符要对Client进行写入 因此客户端的Client不能为空 
			CSockParam param;
			int fd = -1;
			socklen_t len = 0; if (m_param.attr & SOCK_ISIP) {
				param.attr |= SOCK_ISIP;
				len = sizeof(sockaddr_in);
				fd = accept(m_socket, (sockaddr*)param.addrin(), &len);
			}
			else {
				len = sizeof(sockaddr_un);
				fd = accept(m_socket, (sockaddr*)param.addrun(), &len);
			}
			if (fd == -1) return -3;
			*pclient = new CSocket(fd);
			if (*pclient == NULL) return -4;
			ret = (*pclient)->Init(param);
			if (ret != 0) {
				delete (*pclient);
				*pclient = NULL;
				return -5;
			}
		}
		else {
			if (m_param.attr & SOCK_ISIP) {
				ret = connect(m_socket, m_param.addrin(), sizeof(sockaddr_in));
				printf("%s(%d):%s  m_socket:%d\n", __FILE__,__LINE__,__FUNCTION__,m_socket);
			}
			else {
				ret = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
				printf("%s(%d):%s  m_socket:%d\n", __FILE__, __LINE__, __FUNCTION__, m_socket);

			}
			if (ret != 0 ) return -6;
		}
		m_status = 2;
		return 0;
	}
	//发送数据
	virtual int Send(const Buffer& data) {
		if (m_status < 2 || m_socket == -1) return -1;
		size_t index = 0;
		while (index < data.size()) {
			ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
			printf("%s(%d):%s  len=%d errno=%d  str=%s\n", __FILE__, __LINE__, __FUNCTION__, len, errno, strerror(errno));
			if (len == 0) return -2;//说明关闭了
			if (len < 0) return -3;//说明发生错误
			index += len;
		}
		return 0;
	}
	//接收数据 返回值大于0表示接收到的长度  小于0表示接受失败 等于0表示未收到数据但没有发生错误
	virtual int Recv(Buffer& data) {
		if (m_status < 2 || m_socket == -1) return -1;
		data.resize(1024 * 1024);
		ssize_t len = read(m_socket, data, data.size());
		printf("%s(%d):%s m_status=%d m_socket=%d  read =%d tid=  %ld\n", __FILE__, __LINE__, __FUNCTION__, m_status, m_socket,len,pthread_self());
		if (len > 0) {
			data.resize(len);
			return (int)len;//收到数据
		}
		data.clear();
		if (len < 0) {
			if (errno == EINTR || errno == EAGAIN) {//未接收到数据
				data.clear();
				return 0;//未收到数据
			}
			return -2;//发生错误
		}
		return -3;//ret==0 表示套接字被关闭
	}
	//关闭连接
	virtual int Close(){
		return CSocketBase::Close();
	}
};

