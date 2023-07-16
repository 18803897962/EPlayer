#include "Server.h"
#include"Logger.h"
CServer::CServer() {
	m_server = NULL;
	m_business = NULL;
}

CServer::~CServer(){
	Close();
}

int CServer::Init(CBusiness* pBusiness, const Buffer& ip, short port){
	int ret = 0;
	if (pBusiness == NULL) return -1;//û��ҵ���ֱ�ӷ���
	m_business = pBusiness;
	ret = m_process.SetEctryFunc(&CBusiness::CBusinessProcess, m_business,&m_process);
	if (ret != 0) return -2;
	ret = m_process.CreateSubPro();//�����ӽ���
	if (ret != 0) return -3;
	ret = m_pool.Start(2);//�̳߳�����
	if (ret != 0) return -4;
	ret = m_epoll.Create(2);
	if (ret != 0) return -5;
	m_server = new CSocket();
	if (m_server == NULL) return -6;
	ret = m_server->Init(CSockParam(ip,port,SOCK_ISSERVER|SOCK_ISIP|SOCK_ISREUSE));  //����������
	if (ret != 0) return -7;
	ret = m_epoll.Add(*m_server,EpollData((void*)m_server));
	if (ret != 0) return -8;
	for (size_t i = 0; i < m_pool.Size(); i++) {
		ret = m_pool.AddTask(&CServer::ThreadFunc, this);
		printf("%s(%d):[%s]", __FILE__, __LINE__, __FUNCTION__);
		if (ret != 0) return -9;
	}
	return 0;
}

int CServer::Run(){
	while (m_server != NULL) {
		usleep(10);
	}
	return 0;
}

int CServer::Close(){
	if (m_server != NULL) {
		CSocketBase* sock = m_server;
		m_server = NULL;
		m_epoll.Delete(*sock);
		delete sock;
	}
	m_epoll.Close();
	m_process.SendFD(-1);//����-1��ʾ����
	m_pool.Close();
	return 0;
}

int CServer::ThreadFunc(){
	TRACEI("epoll:%d  server:%p", (int)m_epoll, m_server);
	int ret = 0;
	EPEvents events;
	while ((m_epoll != -1) && (m_server != NULL)) {
		//printf("%s(%d):[%s]", __FILE__, __LINE__, __FUNCTION__);
		size_t size = m_epoll.WaitEvents(events, 10);
		//printf("%s(%d):[%s]  size=%d", __FILE__, __LINE__, __FUNCTION__,size);

		if (size < 0) {//���ִ���
			break;
		}
		if (size > 0) {
			for (size_t i = 0; i < size; i++) {
				TRACEI("%s(%d):[%s] size=%d  event:%08X", __FILE__, __LINE__, __FUNCTION__, size, events[0].events);
				if (events[i].events & EPOLLERR) { //���ִ���
					break;//�˴�������ͻ���������շ�
					//�������������Close�ڹرգ��˴����ù�
				}
				else if (events[i].events & EPOLLIN) {//�յ�����
					if (m_server) {
						CSocketBase* pClient;
						ret = m_server->Link(&pClient); //���ӿͻ���
						if (ret != 0) continue;
						ret = m_process.SendSocket(*pClient,*pClient);//���ͻ�������������
						TRACEI("send socket ret=%d", ret);
						int s = *pClient;
						delete pClient;
						if (ret != 0) {
							TRACEE("send client fd failed! fd:%d",s);
							continue;
						}
					}
				}
			}
		}
	}
	TRACEI("��������ֹ");
	printf("%s(%d):[%s]", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}
