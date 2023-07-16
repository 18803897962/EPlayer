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
		clock_gettime(CLOCK_REALTIME,&tp);//获取系统时钟 时间精确至ns
		char* buf = NULL;
		asprintf(&buf, "%d.%d.sock", tp.tv_sec % 100000, tp.tv_nsec % 1000000);//由系统自动根据大小分配内存
		if (buf != NULL) {
			m_path = buf;
			free(buf);//buf由系统分配，需要进行回收
		}//TODO:问题处理
		usleep(1);
	}
	~ThreadPool() {
		Close();
	}
public:
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
public:
	int Start(unsigned count) {//线程池启动  
		if (m_server != NULL) return -1;  //已经初始化
		if (m_path.size() == 0) return -2; //构造失败
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
			ret = m_threads[i]->Start();  //线程启动
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
		unlink(m_path); //本地套接字的链接文件，在close时需要将链接文件删除
	}
	template<typename _FUNCTION_,typename... _ARGS_>
	int AddTask(_FUNCTION_ func, _ARGS_... args) {
		static thread_local CSocket client;  //C++11中，为每个线程分配一个变量 同一个线程调用是相同的  不同线程调用，得到的是不同的
		int ret = 0;
		if (client == -1) {  //未初始化
			ret = client.Init(CSockParam(m_path,0));
			if (ret != 0) return -1;
			client.Link();
			if (ret != 0) return -2;
		}
		CFunctionBase* base = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (base == NULL) return -3;
		Buffer data(sizeof(base));
		memcpy(data, &base, sizeof(base));//需要发送指针base
		ret = client.Send(data);  //通过发送来进行dispatch  发送给TaskDispatch
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
		while (m_epoll != -1) {  //说明epoll正常进行
			EPEvents events;
			int ret = 0;
			size_t esize = m_epoll.WaitEvents(events);
			if (esize > 0) {
				for (size_t i = 0; i < esize;i++) {
					if (events[i].events & EPOLLIN) {//有输入
						CSocketBase* pClient = NULL;
						if (events[i].data.ptr == m_server) {//有客户端连接
							ret = m_server->Link(&pClient);
							if (ret != 0) continue;
							ret = m_epoll.Add(*pClient,EpollData((void*)pClient));
							if (ret != 0) {
								delete pClient;
								continue;
							}
						}
						else {//客户端数据
							pClient = (CSocketBase*)events[i].data.ptr;  //data在上方EpollData处添加
							if (pClient) {
								CFunctionBase* base = NULL;  //存储当前线程分配的函数
								Buffer data(sizeof(base));
								ret = pClient->Recv(data);
								if (ret <= 0) {//出错
									m_epoll.Delete(*pClient);
									delete pClient;
									continue;
								}
								memcpy(&base, (char*)data, sizeof(base));
								if (base != NULL) {  //调用线程分配函数
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
	std::vector<CThread*> m_threads;//需要有默认构造函数和拷贝构造函数才能放进容器  线程是不可复制的
	CSocketBase* m_server;
	Buffer m_path;
};