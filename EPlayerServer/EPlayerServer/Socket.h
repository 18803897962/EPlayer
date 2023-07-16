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
	SOCK_ISSERVER = 1,  //�Ƿ��Ƿ����� 1��ʾ�� 0��ʾ����
	SOCK_ISNONBLOCK = 2,//�Ƿ����� 1��ʾ������ 0��ʾ����
	SOCK_ISUDP = 4,//�Ƿ���UDP  1��ʾ��UDP 0��ʾ��TCP
	SOCK_ISIP=8,//�Ƿ���IPЭ�� 1��ʾIPЭ�� 0��ʾ�����׽���
	SOCK_ISREUSE=16//�Ƿ����õ�ַ
};
class CSockParam {
public:
	CSockParam() {
		bzero(&addr_in, sizeof(addr_in));
		bzero(&addr_un, sizeof(addr_un));
		port = -1;
		attr = 0;//Ĭ�Ͽͻ��� ���� TCP
	};
	CSockParam(const Buffer& ip, short port, int attr) {//�����׽���
		this->IP = ip;
		this->port = port;//host to net short �����ֽ���תΪ�����ֽ���
		this->attr = attr;
		addr_in.sin_family = AF_INET;  //PF_INET Э���� �����׽��ֳ�ʼ��ʱ  AF_INET��ַ�� ����addr��ֵ
		addr_in.sin_port = htons(port);
		addr_in.sin_addr.s_addr = inet_addr(ip);
	}
	CSockParam(const Buffer& path, int attr) {
		IP = path;//ʹ��IP���н�·��
		addr_un.sun_family = AF_UNIX;
		memcpy(addr_un.sun_path, (const char*)path, sizeof(addr_un.sun_path));
		this->attr = attr;
	}
	CSockParam(const sockaddr_in* addrin, int attr) {//�����׽���
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
	//��ַ
	sockaddr_in addr_in;//�����׽���
	sockaddr_un addr_un;//�����׽���
	Buffer IP;//IP
	short port;//�˿�
	int attr;//���� �ο�SockAttr

};

class CSocketBase {
public:
	CSocketBase():m_socket(-1),m_status(0) {}
	virtual ~CSocketBase() {
		Close();
	}//����������������
public:
	//���������׽��ִ���  bind listen   �ͻ��� �׽��ִ���
	//��ʼ��  
	virtual int Init(const CSockParam& param) = 0;
	//������ accept �ͻ��� connect  udp���Ժ���
	//����
	virtual int Link(CSocketBase** pclient = NULL) = 0;
	//��������
	virtual int Send(const Buffer& data) = 0;
	//��������
	virtual int Recv(Buffer& data) = 0;
	//�ر�����
	virtual int Close() {
		m_status = 3;
		if (m_socket != -1) {
			if ((m_param.attr & SOCK_ISSERVER)//������ 
				&& ((m_param.attr & SOCK_ISIP) == 0))//��IP
				unlink(m_param.IP);  //�����׽���close��Ҫ������� �����׽���������
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
	//�׽��������� Ĭ��-1
	int m_socket;
	//״̬  0δ��ʼ��  1�ѳ�ʼ�� 2������  3�ѹر�
	int m_status;
	//��ʼ������
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
	//���������׽��ִ���  bind listen   �ͻ��� �׽��ִ���
	//��ʼ��  
	virtual int Init(const CSockParam& param) {
		if (m_status != 0) return -1;
		m_param = param;
		int type = (param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;
		if (m_socket == -1) {
			if (m_param.attr & SOCK_ISIP) {  //�����׽���
				m_socket = socket(PF_INET, type, 0);
			}
			else {
				m_socket = socket(PF_LOCAL, type, 0);//�����׽���
			}
		}
		else
			m_status = 2;  //accept�����׽��� �Ѿ���������״̬
		if (m_socket == -1) {
			return -2;
		}
		int ret = 0;
		if (m_param.attr & SOCK_ISREUSE) {  //��Ҫ��ַ���� ��ɾ��֮ǰ���׽����ļ� ���µ�ǰ���׽����ļ�
			int option = 1;
			ret = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
			if (ret == -1) return -3;
		}
		if (m_param.attr & SOCK_ISSERVER) {//�Ƿ��Ƿ����� ��������Ҫbind �� listen
			if (m_param.attr & SOCK_ISIP) {  //�����׽���
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
		//�ͻ���ֻ��Ҫ�����׽���  
		if (m_param.attr & SOCK_ISNONBLOCK) {//������״̬
			int option = fcntl(m_socket, F_GETFL);  //ȡ��ǰ״̬
			if (option == -1) return -6;
			option |= O_NONBLOCK;//��ӷ�����״̬
			ret = fcntl(m_socket, F_SETFL, option);//���÷�����״̬
			if (ret == -1) return -7;
		}
		
		if(m_status==0)
			m_status = 1;
		return 0;

	}
	//������ accept �ͻ��� connect  udp���Ժ���
	//����
	virtual int Link(CSocketBase** pclient = NULL) {
		if (m_status <= 0 || m_socket == -1) return -1; 
		int ret = 0;
		if (m_param.attr & SOCK_ISSERVER) {//������ accept
			if (pclient == NULL) return -2;//���յ���������Ҫ��Client����д�� ��˿ͻ��˵�Client����Ϊ�� 
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
	//��������
	virtual int Send(const Buffer& data) {
		if (m_status < 2 || m_socket == -1) return -1;
		size_t index = 0;
		while (index < data.size()) {
			ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
			printf("%s(%d):%s  len=%d errno=%d  str=%s\n", __FILE__, __LINE__, __FUNCTION__, len, errno, strerror(errno));
			if (len == 0) return -2;//˵���ر���
			if (len < 0) return -3;//˵����������
			index += len;
		}
		return 0;
	}
	//�������� ����ֵ����0��ʾ���յ��ĳ���  С��0��ʾ����ʧ�� ����0��ʾδ�յ����ݵ�û�з�������
	virtual int Recv(Buffer& data) {
		if (m_status < 2 || m_socket == -1) return -1;
		data.resize(1024 * 1024);
		ssize_t len = read(m_socket, data, data.size());
		printf("%s(%d):%s m_status=%d m_socket=%d  read =%d tid=  %ld\n", __FILE__, __LINE__, __FUNCTION__, m_status, m_socket,len,pthread_self());
		if (len > 0) {
			data.resize(len);
			return (int)len;//�յ�����
		}
		data.clear();
		if (len < 0) {
			if (errno == EINTR || errno == EAGAIN) {//δ���յ�����
				data.clear();
				return 0;//δ�յ�����
			}
			return -2;//��������
		}
		return -3;//ret==0 ��ʾ�׽��ֱ��ر�
	}
	//�ر�����
	virtual int Close(){
		return CSocketBase::Close();
	}
};

