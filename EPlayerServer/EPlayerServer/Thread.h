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
		//joinable �ɽ�ϵ� �ܱ������̻߳��պ�ɱ��  PTHREAD_CREATE_DETACHED ����� �����������߳�ɱ������ ����ֻ����ϵͳ����
		if (ret != 0) return -2;
		//ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);//���÷����ڽ����ڲ�
		//if (ret != 0) return -3;
		ret = pthread_create(&m_thread,&attr,&CThread::threadEntry,this);
		if (ret != 0) return -4;
		m_mapThread[m_thread] = this;  //��¼tid�Ͷ�Ӧ��CThread*ָ��
		ret = pthread_attr_destroy(&attr);
		if (ret != 0) return -5;
		return 0;
	}
	
	int Stop() {
		if (m_thread != 0) {//֤���̴߳���
			pthread_t thread = m_thread;
			m_thread = 0;
			timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 100 * 1000000;//100ms
			int ret = pthread_timedjoin_np(thread,NULL,&ts);//�ȴ�100ms��join
			if (ret == ETIMEDOUT) {//�Ѿ���ʱ ��ʱ��Ҫ���߳̽���
				pthread_detach(thread);//�̷߳���  ��ʱ���߳��������̶߳Ͽ���ϵ �߳̽��������˳�״̬���������̻߳�ȡ �����Լ��ͷ���Դ
				pthread_kill(thread,SIGUSR2);  //�ź���ͨ�� ������Ӧ���¼��������
			}
		}
		return 0;
	}

	int Pause() {
		if (m_thread != 0) return -1;    ///////????Ϊʲô������0����
		if (m_paused == true) {
			m_paused = false;//����pause״̬���� ���������
			return 0;
		}
		m_paused = true;
		int ret = pthread_kill(m_thread, SIGUSR1);//�ź���ͨ�� ������Ӧ���¼��������
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
	//std_call ���贫��thisָ�� ��Ϊ�̴߳���ʱҪ��Ĳ���ֻ����һ��void* 
	static void* threadEntry(void* arg) {
		CThread* thiz = (CThread*)arg; 
		struct sigaction act = { 0 };   //�����޸���ָ���ź�������Ĵ���������ͬʱ���ֲ�����
		sigemptyset(&act.sa_mask);  //���źż����
		act.sa_flags = SA_SIGINFO;  //��ʾҪʹ��������������Ӧ����
		act.sa_sigaction=&CThread::Sigaction;
		sigaction(SIGUSR1, &act, NULL); // ��ͣ  �յ��źž͵��ûص�����
		sigaction(SIGUSR2, &act, NULL); // �߳��˳�  �յ��źž͵��ûص�����   ���ź���ص�������

		thiz->EnterThread();//�����̴߳�����

		if (thiz->m_thread) thiz->m_thread = 0;
		pthread_t thread = pthread_self();
		std::map<pthread_t, CThread*>::iterator it = m_mapThread.find(thread);
		if (it != m_mapThread.end()) {//�Դ���
			m_mapThread[thread] = NULL;
		}
		pthread_detach(pthread_self());
		pthread_exit(NULL);
	}
	static void Sigaction(int signo, siginfo_t* info, void* arg) {
		if (signo == SIGUSR1) {//������ͣʹ��  pause
			pthread_t thread = pthread_self();
			auto it = m_mapThread.find(thread);
			if (it != m_mapThread.end()) {
				if (it->second) {  //it->second��ʾ���̶߳�Ӧ��Thread*ָ��
					while (it->second->m_paused) {
						if (it->second->m_thread == 0) {//�߳���Ч
							pthread_exit(NULL);
						}
						usleep(1000);//����1ms
					}
				}
			}

		}
		else if (signo == SIGUSR2) {
			pthread_exit(NULL);
		}
	}
	//this_call  this_call�Ὣthisָ�봫�� ���Ͳ�����
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
	pthread_t m_thread;//�߳�id
	bool m_paused;//�Ƿ���ͣ  true��ʾ��ͣ false��ʾ������
	static std::map<pthread_t, CThread*> m_mapThread;  //��̬��Ա����������CPP�г�ʼ�� ������ͷ�ļ��г�ʼ��  Ҳ���������ڹ��캯����ʼ�� ��Ϊ���ܻᱻ�ظ���ʼ��
};