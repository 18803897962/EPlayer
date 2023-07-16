#pragma once
#include"ThreadPool.h"
#include"Socket.h"
#include"Process.h"
//CServer负责接收 并发送fd
//Business 循环接收来自进程的fd文件描述符
//启动线程池 通过AddTask方式 使用线程池处理  


//function头文件中的类，放在此处是因为function中并未引用Buffer和CSocketBase  引用之后可能导致交叉引用
template<typename _FUNCTION_, typename..._ARGS_>  //_FUNCTION_指代函数类型  _ARGS_指代可变参数类型
class CConnectedFunction :public CFunctionBase {
public:
	CConnectedFunction(_FUNCTION_ func, _ARGS_... args)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) {//std::forward<typename>(type) 表明将type原样转发 ...表示原地展开，对此后都做相同类型的操作
	}
	~CConnectedFunction() {}
	virtual int operator()(CSocketBase* pClient) {
		return m_binder(pClient);
	}
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//int表示返回值 _FUNCTION_表示函数类型 _ARGS_表示参数
	//typename 声明其是一个类型 类型声明符
};

template<typename _FUNCTION_, typename..._ARGS_>  //_FUNCTION_指代函数类型  _ARGS_指代可变参数类型
class CReceivedFunction :public CFunctionBase {
public:
	CReceivedFunction(_FUNCTION_ func, _ARGS_... args)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) {//std::forward<typename>(type) 表明将type原样转发 ...表示原地展开，对此后都做相同类型的操作
	}
	~CReceivedFunction() {}
	virtual int operator()(CSocketBase* pClient, const Buffer& data) {
		return m_binder(pClient, data);
	}
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//int表示返回值 _FUNCTION_表示函数类型 _ARGS_表示参数
	//typename 声明其是一个类型 类型声明符
};

class CBusiness {//业务事务
public:
	CBusiness() :m_connectedcallback(NULL), m_SetRecvcallback(NULL){}
	virtual int CBusinessProcess(CProcess* proc) = 0;
	template<typename _FUNCTION_,typename... _ARGS_>
	int SetConnectCallback(_FUNCTION_ func, _ARGS_... args) {
		m_connectedcallback = new CConnectedFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_connectedcallback == NULL) {
			return -1;
		}
		return 0;  //回调函数设置成功
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
	CFunctionBase* m_connectedcallback;//连接成功的回调函数
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
	CProcess m_process;//开进程 完成客户端处理
	CBusiness* m_business;//需要手动delete
};


