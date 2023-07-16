#pragma once
#include<unistd.h>
#include<pthread.h>
#include<fcntl.h>
#include<signal.h>
#include<map>
#include"Function.h"

class CThread {
public:
	CThread() {
		m_function = NULL;
		m_thread = 0;
		m_paused = false;
	};
	template<typename _FUNCTION_,typename... _ARGS_>
	CThread(_FUNCTION_ func, _ARGS_... args):m_function(new CFunction<_FUNCTION_,_ARGS_...>(func,args...)) {
		m_thread = 0;
		m_paused = false;
	}
	~CThread() {};
public:
	CThread(const CThread&) = delete;
	CThread& operator=(const CThread&) = delete;
public:
	template<typename _FUNCTION_ ,typename... _ARGS_>
	int SetThreadFunc(_FUNCTION_ func, _ARGS_... args) {
		m_function = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_function == NULL) return -1;
		return 0;
	}

	int Start() {
		pthread_attr_t attr;
		int ret = 0;
		ret = pthread_attr_init(&attr);
		if (ret != 0) return -1;
		ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);  
		//joinable 可结合的 能被其他线程回收和杀死  PTHREAD_CREATE_DETACHED 分离的 不能由其它线程杀死回收 而是只能由系统回收
		if (ret != 0) return -2;
		//ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);//设置返回在进程内部
		//if (ret != 0) return -3;
		ret = pthread_create(&m_thread,&attr,&CThread::threadEntry,this);
		if (ret != 0) return -4;
		m_mapThread[m_thread] = this;  //记录tid和对应的CThread*指针
		ret = pthread_attr_destroy(&attr);
		if (ret != 0) return -5;
		return 0;
	}
	
	int Stop() {
		if (m_thread != 0) {//证明线程存在
			pthread_t thread = m_thread;
			m_thread = 0;
			timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 100 * 1000000;//100ms
			int ret = pthread_timedjoin_np(thread,NULL,&ts);//等待100ms后join
			if (ret == ETIMEDOUT) {//已经超时 此时需要将线程结束
				pthread_detach(thread);//线程分离  此时该线程与主控线程断开关系 线程结束后，其退出状态不由其它线程获取 而是自己释放资源
				pthread_kill(thread,SIGUSR2);  //信号量通信 唤起相应的事件处理程序
			}
		}
		return 0;
	}

	int Pause() {
		if (m_thread != 0) return -1;    ///////????为什么不等于0返回
		if (m_paused == true) {
			m_paused = false;//若是pause状态调用 则进行启动
			return 0;
		}
		m_paused = true;
		int ret = pthread_kill(m_thread, SIGUSR1);//信号量通信 唤起相应的事件处理程序
		if (ret != 0) {
			m_paused = false;
			return -2;
		}
		return 0;
	}
	
	bool isValid() const {
		return m_thread != 0;
	}

private:
	//std_call 无需传入this指针 因为线程创建时要求的参数只能有一个void* 
	static void* threadEntry(void* arg) {
		CThread* thiz = (CThread*)arg; 
		struct sigaction act = { 0 };   //检查或修改与指定信号相关联的处理动作（可同时两种操作）
		sigemptyset(&act.sa_mask);  //将信号集清空
		act.sa_flags = SA_SIGINFO;  //表示要使用三个参数的相应函数
		act.sa_sigaction=&CThread::Sigaction;
		sigaction(SIGUSR1, &act, NULL); // 暂停  收到信号就调用回调函数
		sigaction(SIGUSR2, &act, NULL); // 线程退出  收到信号就调用回调函数   将信号与回调函数绑定

		thiz->EnterThread();//进入线程处理函数

		if (thiz->m_thread) thiz->m_thread = 0;
		pthread_t thread = pthread_self();
		std::map<pthread_t, CThread*>::iterator it = m_mapThread.find(thread);
		if (it != m_mapThread.end()) {//仍存在
			m_mapThread[thread] = NULL;
		}
		pthread_detach(pthread_self());
		pthread_exit(NULL);
	}
	static void Sigaction(int signo, siginfo_t* info, void* arg) {
		if (signo == SIGUSR1) {//留给暂停使用  pause
			pthread_t thread = pthread_self();
			auto it = m_mapThread.find(thread);
			if (it != m_mapThread.end()) {
				if (it->second) {  //it->second表示此线程对应的Thread*指针
					while (it->second->m_paused) {
						if (it->second->m_thread == 0) {//线程无效
							pthread_exit(NULL);
						}
						usleep(1000);//休眠1ms
					}
				}
			}

		}
		else if (signo == SIGUSR2) {
			pthread_exit(NULL);
		}
	}
	//this_call  this_call会将this指针传入 类型不允许
	void EnterThread() {
		if (m_function != NULL) {
			int ret = (*m_function)();
			if (ret != 0) {
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
		}
	}
	
private:
	CFunctionBase* m_function;
	pthread_t m_thread;//线程id
	bool m_paused;//是否暂停  true表示暂停 false表示运行中
	static std::map<pthread_t, CThread*> m_mapThread;  //静态成员变量必须在CPP中初始化 不能在头文件中初始化  也不能在类内构造函数初始化 因为可能会被重复初始化
};