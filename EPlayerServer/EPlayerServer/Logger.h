#pragma once
#include"Thread.h"
#include"Epoll.h"
#include"Socket.h"
#include<sys/timeb.h>
#include<stdarg.h>
#include<sstream>
#include<sys/stat.h>
#include"Public.h"
enum LogLevel {
	LOG_INFO,
	LOG_DEBUG,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATIAL  //致命错误  
};


class LogInfo {//不同种日志消息的封装
public:
	LogInfo(const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int loglevel, const char* fmt, ...);
	LogInfo(const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int loglevel);
	LogInfo(const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int loglevel, void* pdata, size_t nSize);
	~LogInfo();
public:
	operator Buffer() const {
		return m_buf;
	}
	template<typename T>
	LogInfo& operator <<(const T& data) {
		std::stringstream stream;
		stream << data;
		printf("[%s] (%d) [%s]  [%s]\n", __FILE__, __LINE__, __FUNCTION__, stream.str().c_str());
		m_buf += stream.str().c_str();
		printf("[%s] (%d) [%s]  [%s]\n", __FILE__, __LINE__, __FUNCTION__, (char*)m_buf);
		return *this;
	}
private:
	bool bAuto;//默认false 流式日志则为true
	Buffer m_buf;
};

class CLoggerServer {
public:
	CLoggerServer() :m_thread(&CLoggerServer::Threadfunc, this) {
		m_server = NULL;
		char curpath[256];
		getcwd(curpath, sizeof(curpath));//获取当前路径
		m_path = curpath;
		m_path += "/log/" + GetTimeStr() + ".log";
		printf(" %s %d %s path= %s\n", __FILE__, __LINE__, __FUNCTION__, (char*)m_path);
	}
	~CLoggerServer() {
		Close();
	}
public:
	CLoggerServer(const CLoggerServer&) = delete;
	CLoggerServer& operator=(const CLoggerServer&) = delete;
public:
	int Start() {//日志服务器启动
		if (m_server != NULL) {//说明已经start过了
			return -1;
		}
		if (access("log", W_OK | R_OK)) {//文件不可达 证明不存在 需要创建
			mkdir("log", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);//当前用户、组读写 其它读
		}
		m_file = fopen(m_path, "w+");//不存在就创建
		if (m_file == NULL) return -2;
		int ret = m_epoll.Create(1);//一个线程进行写入
		if (ret != 0) return -3;
		m_server = new CSocket();
		if (m_server == NULL) {//创建失败
			Close();
			return -4;
		}
		ret = m_server->Init(CSockParam("./log/server.sock", SOCK_ISSERVER|SOCK_ISREUSE));//创建本地套接字
		if (ret != 0) {
			printf("%s(%d)%s ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
			Close();
			return -5;
		}
		ret = m_epoll.Add(*m_server, EpollData((void*)m_server), EPOLLIN | EPOLLERR);
		if (ret != 0) {
			Close();
			return -6;
		}
		ret = m_thread.Start();
		if (ret != 0) {
			printf("%s(%d):<%s> pid=%d ret=%d error code=%d  msg=%s\n"
				, __FILE__, __LINE__, __FUNCTION__, getpid(), ret, errno, strerror(errno));
			Close();
			return -7;
		}
		return 0;
	}

	int Close() {
		if (m_server != NULL) {
			CSocketBase* ptr = m_server;
			m_server = NULL;
			delete ptr;
		}
		m_epoll.Close();
		m_thread.Stop();
		return 0;
	}
	static void Trace(const LogInfo& info) {//给其他非日志进程和线程使用
		static thread_local CSocket client;
		int ret = 0;
		if (client == -1) {
			ret = client.Init(CSockParam("./log/server.sock", 0));
			if (ret != 0) {
#ifdef _DEBUG
				printf(" %s %d %s  ret=%d \n", __FILE__, __LINE__, __FUNCTION__, ret);
#endif // _DEBUG
				return;
			}
			printf(" %s %d %s  ret=%d  %d\n", __FILE__, __LINE__, __FUNCTION__, ret, int(client));
			ret = client.Link();
			printf(" %s %d %s  ret=%d  %d\n", __FILE__, __LINE__, __FUNCTION__, ret, int(client));
		}
		ret = client.Send(info);
		printf(" %s %d %s  ret=%d  %d  error code:%d  str:%s\n", __FILE__, __LINE__, __FUNCTION__, ret, int(client),errno,strerror(errno));
	}
	static Buffer GetTimeStr() {
		Buffer result(1024);
		timeb tmb;
		ftime(&tmb);
		tm* ptm = (localtime)(&tmb.time);
		int ret = snprintf(result, sizeof(result), "%04d-%02d-%02d_%02d-%02d-%02d.%03d",
			ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, tmb.millitm);
		result.resize(ret);
		return result;
	}
private:
	int Threadfunc() {
		printf(" %s %d %s isvalid=%d\n", __FILE__, __LINE__, __FUNCTION__, m_thread.isValid());
		printf(" %s %d %s isvalid=%d\n", __FILE__, __LINE__, __FUNCTION__, (int)m_epoll);
		printf(" %s %d %s isvalid=%p\n", __FILE__, __LINE__, __FUNCTION__, m_server);
		EPEvents events;
		std::map<int, CSocketBase*> map_client;
		while (m_thread.isValid() && (m_epoll != -1) && (m_server != NULL)) {
			ssize_t ret = m_epoll.WaitEvents(events, 1000);   //ret表示等待到的事件数  若小于0则表示错误  获取到epoll等待的事件
			printf("%s(%d)%s\n", __FILE__, __LINE__, __FUNCTION__);
			if (ret < 0) break;
			if (ret > 0) {
				ssize_t i = 0;
				for (; i < ret; i++) {
					printf("%s(%d):%s  events:%d\n", events[i].events);
					if (events[i].events & EPOLLERR) {
						break;
					}
					else if (events[i].events & EPOLLIN) {
						if (events[i].data.ptr == m_server) {//服务器收到输入请求  有客户端要来连接
							CSocketBase* pClient = NULL;
							int r = m_server->Link(&pClient);
							printf(" %s %d %s  ret=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
							if (r < 0) {//link失败
								continue;
							}
							r = m_epoll.Add(*pClient, EpollData((void*)pClient), EPOLLIN | EPOLLERR);////TODO
							printf(" %s %d %s  ret=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
							if (r < 0) {
								delete pClient;
								continue;
							}
							printf(" %s %d %s \n", __FILE__, __LINE__, __FUNCTION__);
							std::map<int, CSocketBase*>::iterator it = map_client.find(*pClient);
							printf(" %s %d %s \n", __FILE__, __LINE__, __FUNCTION__);
							if (it != map_client.end()) {  //可能已经存在
								if (it->second) delete it->second;
							}
							map_client[*pClient] = pClient;
						}
						else {//客户端
							printf(" %s %d %s  ptr=%p \n", __FILE__, __LINE__, __FUNCTION__, events[i].data.ptr);
							CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
							if (pClient != NULL) {
								Buffer data(1024 * 1024);
								int r = pClient->Recv(data);
								printf(" %s %d %s  ret=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
								if (r < 0) {
									map_client[*pClient] = NULL;
									printf(" %s %d %s  ret=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
									delete pClient;
									printf(" %s %d %s  ret=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
								}
								else {//recv成功
									printf(" %s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
									//printf("%s", (char*)data);
									WriteLog(data);
								}
							}
						}
					}
				}
				if (i != ret) {//for循环中出现错误，提前break
					break;
				}
			}
		}
		for (std::map<int, CSocketBase*>::iterator it = map_client.begin(); it != map_client.end(); it++) {
			if (it->second != NULL) {
				delete it->second; //delete掉csocketbase*指针 防止内存泄漏
			}
		}
		map_client.clear();
		return 0;
	}
	void WriteLog(const Buffer& data) {
		if (m_file != NULL) {
			FILE* pfile = m_file;
			size_t ret = fwrite((const char*)data, 1, data.size(), pfile);
			printf("%s(%d):%s  ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
			fflush(pfile);
#ifdef _DEBUG
			printf("%s\n", (char*)data);
#endif
		}
	}
private:
	CThread m_thread;
	CEpoll m_epoll;
	CSocketBase* m_server;
	Buffer m_path;
	FILE* m_file;
};

#ifndef TRACE
#define TRACEI(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_INFO,__VA_ARGS__))  //表明trace中的...参数填入到_VA_ARGS__
#define TRACED(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_DEBUG,__VA_ARGS__))  //表明trace中的...参数填入到_VA_ARGS__
#define TRACEW(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_WARNING,__VA_ARGS__))  //表明trace中的...参数填入到_VA_ARGS__
#define TRACEE(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_ERROR,__VA_ARGS__))  //表明trace中的...参数填入到_VA_ARGS__
#define TRACEF(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_FATIAL,__VA_ARGS__))  //表明trace中的...参数填入到_VA_ARGS__

//LOGI<<"hello"<<"world";
#define LOGI LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_INFO)
#define LOGD LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_DEBUG)
#define LOGW LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_WARNING)
#define LOGE LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_ERROR)
#define LOGF LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_FATIAL)


//内存导出
#define DUMPI(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_INFO,data,size))
#define DUMPD(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_DEBUG,data,size))
#define DUMPW(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_WARNING,data,size))
#define DUMPE(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_ERROR,data,size))
#define DUMPF(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_FATIAL,data,size))

#endif // !TRACE
