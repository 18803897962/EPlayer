#pragma once
#include"ThreadPool.h"
#include"Socket.h"
#include"Process.h"
//CServer������� ������fd
//Business ѭ���������Խ��̵�fd�ļ�������
//�����̳߳� ͨ��AddTask��ʽ ʹ���̳߳ش���  


//functionͷ�ļ��е��࣬���ڴ˴�����Ϊfunction�в�δ����Buffer��CSocketBase  ����֮����ܵ��½�������
template<typename _FUNCTION_, typename..._ARGS_>  //_FUNCTION_ָ����������  _ARGS_ָ���ɱ��������
class CConnectedFunction :public CFunctionBase {
public:
	CConnectedFunction(_FUNCTION_ func, _ARGS_... args)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) {//std::forward<typename>(type) ������typeԭ��ת�� ...��ʾԭ��չ�����Դ˺�����ͬ���͵Ĳ���
	}
	~CConnectedFunction() {}
	virtual int operator()(CSocketBase* pClient) {
		return m_binder(pClient);
	}
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//int��ʾ����ֵ _FUNCTION_��ʾ�������� _ARGS_��ʾ����
	//typename ��������һ������ ����������
};

template<typename _FUNCTION_, typename..._ARGS_>  //_FUNCTION_ָ����������  _ARGS_ָ���ɱ��������
class CReceivedFunction :public CFunctionBase {
public:
	CReceivedFunction(_FUNCTION_ func, _ARGS_... args)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) {//std::forward<typename>(type) ������typeԭ��ת�� ...��ʾԭ��չ�����Դ˺�����ͬ���͵Ĳ���
	}
	~CReceivedFunction() {}
	virtual int operator()(CSocketBase* pClient, const Buffer& data) {
		return m_binder(pClient, data);
	}
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//int��ʾ����ֵ _FUNCTION_��ʾ�������� _ARGS_��ʾ����
	//typename ��������һ������ ����������
};

class CBusiness {//ҵ������
public:
	CBusiness() :m_connectedcallback(NULL), m_SetRecvcallback(NULL){}
	virtual int CBusinessProcess(CProcess* proc) = 0;
	template<typename _FUNCTION_,typename... _ARGS_>
	int SetConnectCallback(_FUNCTION_ func, _ARGS_... args) {
		m_connectedcallback = new CConnectedFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_connectedcallback == NULL) {
			return -1;
		}
		return 0;  //�ص��������óɹ�
	}
	template<typename _FUNCTION_,typename... _ARGS_>
	int SetRecvCallback(_FUNCTION_ func,_ARGS_... args) {
		m_SetRecvcallback = new CReceivedFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_SetRecvcallback == NULL) {
			return -1;
		}
		return 0;
	}
protected:
	CFunctionBase* m_connectedcallback;//���ӳɹ��Ļص�����
	CFunctionBase* m_SetRecvcallback;
};
class CServer
{
public:
	CServer();
	~CServer();
public:
	CServer(const CServer&) = delete;
	CServer& operator=(const CServer&) = delete;
public:
	int Init(CBusiness* pBusiness,const Buffer& ip = "0.0.0.0", short port = 9999);
	int Run();
	int Close();
private:

	int ThreadFunc();
private:
	ThreadPool m_pool;
	CSocketBase* m_server;
	CEpoll m_epoll;
	CProcess m_process;//������ ��ɿͻ��˴���
	CBusiness* m_business;//��Ҫ�ֶ�delete
};


