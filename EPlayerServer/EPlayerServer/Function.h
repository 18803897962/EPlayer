#pragma once
#include <unistd.h>
#include<sys/types.h>
#include <functional>
//�麯����ģ�庯������ͬʱʹ��   ��ģ����������麯��

class CSocketBase;
class Buffer;//��ǰ���� ��CFunctionBase������ָ������� ָ�������ֻ��Ҫ֪�������༴�ɣ�����֪����ľ�������

class CFunctionBase {
public:
    virtual ~CFunctionBase() {}
    virtual int operator()() { return -1; }
    virtual int operator()(CSocketBase*) { return -1; }
    virtual int operator()(CSocketBase*, const Buffer&) { return -1; }
};
template<typename _FUNCTION_, typename..._ARGS_>  //_FUNCTION_ָ����������  _ARGS_ָ���ɱ��������
class CFunction :public CFunctionBase {
public:
    CFunction(_FUNCTION_ func, _ARGS_... args)
        :m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) {//std::forward<typename>(type) ������typeԭ��ת�� ...��ʾԭ��չ�����Դ˺�����ͬ���͵Ĳ���

    }
    ~CFunction() {}
    virtual int operator()() {
        return m_binder();
    }
    typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//int��ʾ����ֵ _FUNCTION_��ʾ�������� _ARGS_��ʾ����
    //typename ��������һ������ ����������
};

