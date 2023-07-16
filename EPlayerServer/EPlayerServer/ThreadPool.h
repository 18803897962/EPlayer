#pragma once
#include"Thread.h"
#include"Epoll.h"
#include"Socket.h"
#include"Function.h"
#include"Public.h"
class ThreadPool {
public:
	ThreadPool() {
		m_server = NULL;
		timespec tp;
		clock_gettime(CLOCK_REALTIME,&tp);//��ȡϵͳʱ�� ʱ�侫ȷ��ns
		char* buf = NULL;
		asprintf(&buf, "%d.%d.sock", tp.tv_sec % 100000, tp.tv_nsec % 1000000);//��ϵͳ�Զ����ݴ�С�����ڴ�
		if (buf != NULL) {
			m_path = buf;
			free(buf);//buf��ϵͳ���䣬��Ҫ���л���
		}//TODO:���⴦��
		usleep(1);
	}
	~ThreadPool() {
		Close();
	}
public:
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
public:
	int Start(unsigned count) {//�̳߳�����  
		if (m_server != NULL) return -1;  //�Ѿ���ʼ��
		if (m_path.size() == 0) return -2; //����ʧ��
		m_server = new CSocket();
		if (m_server == NULL) return -3;
		int ret = m_server->Init(CSockParam(m_path, SOCK_ISSERVER));
		if (ret != 0) return -4;
		ret = m_epoll.Create(count);
		if (ret != 0) return -5;
		m_epoll.Add(*m_server,EpollData((void*)m_server));
		if (ret != 0) return -6;
		m_threads.resize(count);
		for (unsigned i = 0; i < count; i++) {
			m_threads[i] = new CThread(&ThreadPool::TaskDispatch,this);
			if (m_threads[i] == NULL) return -7;
			ret = m_threads[i]->Start();  //�߳�����
			if (ret != 0) return -8;
		}
		return 0;
	}
	void Close() {
		m_epoll.Close();
		if (m_server != NULL) {
			CSocketBase* tmp = m_server;
			m_server = NULL;
			//tmp->Close();
			delete tmp;
		}
		for (CThread* pthread : m_threads) {
			if (pthread != NULL) delete pthread;
		}
		m_threads.clear();
		unlink(m_path); //�����׽��ֵ������ļ�����closeʱ��Ҫ�������ļ�ɾ��
	}
	template<typename _FUNCTION_,typename... _ARGS_>
	int AddTask(_FUNCTION_ func, _ARGS_... args) {
		static thread_local CSocket client;  //C++11�У�Ϊÿ���̷߳���һ������ ͬһ���̵߳�������ͬ��  ��ͬ�̵߳��ã��õ����ǲ�ͬ��
		int ret = 0;
		if (client == -1) {  //δ��ʼ��
			ret = client.Init(CSockParam(m_path,0));
			if (ret != 0) return -1;
			client.Link();
			if (ret != 0) return -2;
		}
		CFunctionBase* base = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (base == NULL) return -3;
		Buffer data(sizeof(base));
		memcpy(data, &base, sizeof(base));//��Ҫ����ָ��base
		ret = client.Send(data);  //ͨ������������dispatch  ���͸�TaskDispatch
		if (ret != 0) {
			delete base;
			return -4;
		}
		return 0;
	}
	size_t Size() const {
		return m_threads.size();
	}
private:
	int TaskDispatch() {
		while (m_epoll != -1) {  //˵��epoll��������
			EPEvents events;
			int ret = 0;
			size_t esize = m_epoll.WaitEvents(events);
			if (esize > 0) {
				for (size_t i = 0; i < esize;i++) {
					if (events[i].events & EPOLLIN) {//������
						CSocketBase* pClient = NULL;
						if (events[i].data.ptr == m_server) {//�пͻ�������
							ret = m_server->Link(&pClient);
							if (ret != 0) continue;
							ret = m_epoll.Add(*pClient,EpollData((void*)pClient));
							if (ret != 0) {
								delete pClient;
								continue;
							}
						}
						else {//�ͻ�������
							pClient = (CSocketBase*)events[i].data.ptr;  //data���Ϸ�EpollData�����
							if (pClient) {
								CFunctionBase* base = NULL;  //�洢��ǰ�̷߳���ĺ���
								Buffer data(sizeof(base));
								ret = pClient->Recv(data);
								if (ret <= 0) {//����
									m_epoll.Delete(*pClient);
									delete pClient;
									continue;
								}
								memcpy(&base, (char*)data, sizeof(base));
								if (base != NULL) {  //�����̷߳��亯��
									(*base)();
									delete base;
								}
							}
						}
					}
				}
			}
		}
		return 0;
	}
private:
	CEpoll m_epoll;
	std::vector<CThread*> m_threads;//��Ҫ��Ĭ�Ϲ��캯���Ϳ������캯�����ܷŽ�����  �߳��ǲ��ɸ��Ƶ�
	CSocketBase* m_server;
	Buffer m_path;
};