#pragma once
#include <unistd.h>
#include<sys/types.h>
#include <functional>
//虚函数和模板函数不能同时使用   但模板类可以有虚函数

class CSocketBase;
class Buffer;//提前声明 在CFunctionBase中生成指针和引用 指针和引用只需要知道这是类即可，无需知道类的具体内容

class CFunctionBase {
public:
    virtual ~CFunctionBase() {}
    virtual int operator()() { return -1; }
    virtual int operator()(CSocketBase*) { return -1; }
    virtual int operator()(CSocketBase*, const Buffer&) { return -1; }
};
template<typename _FUNCTION_, typename..._ARGS_>  //_FUNCTION_指代函数类型  _ARGS_指代可变参数类型
class CFunction :public CFunctionBase {
public:
    CFunction(_FUNCTION_ func, _ARGS_... args)
        :m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) {//std::forward<typename>(type) 表明将type原样转发 ...表示原地展开，对此后都做相同类型的操作

    }
    ~CFunction() {}
    virtual int operator()() {
        return m_binder();
    }
    typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//int表示返回值 _FUNCTION_表示函数类型 _ARGS_表示参数
    //typename 声明其是一个类型 类型声明符
};

