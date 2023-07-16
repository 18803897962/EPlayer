#pragma once
#include <unistd.h>
#include<sys/epoll.h>
#include<vector>
#include<errno.h>
#include<sys/signal.h>
#include<memory.h>
#define EVENT_SIZE 128
class EpollData {
public:
	EpollData() {
		m_data.u64 = 0;
	}
	EpollData(void* ptr) {
		m_data.ptr = ptr;
	}
	explicit EpollData(int fd) { //不允许隐式转换
		m_data.fd = fd;
	}
	explicit EpollData(uint32_t u32) { //不允许隐式转换
		m_data.u32 = u32;
	}
	explicit EpollData(uint64_t u64) { //不允许隐式转换
		m_data.u64 = u64;
	}
	EpollData(const EpollData& data) {
		m_data.u64 = data.m_data.u64;//联合体，只需要将最大的内存赋值即可
	}
public:
	operator epoll_data_t() {//类型转换运算符重载
		return m_data;
	}
	operator epoll_data_t() const {
		return m_data;
	}
	operator epoll_data_t* () {
		return &m_data;
	}
	operator const epoll_data_t* () const {
		return &m_data;
	}
	EpollData& operator=(const EpollData& data) {
		if (this != &data) {
			m_data.u64 = data.m_data.u64;//联合体，只需要将最大的内存赋值即可
		}
		return *this;
	}
	EpollData& operator=(void* data) {
		m_data.ptr = data;
		return *this;
	}
	EpollData& operator=(int data) {
		m_data.fd = data;
		return *this;
	}
	EpollData& operator=(uint32_t data) {
		m_data.u32 = data;
		return *this;
	}
	EpollData& operator=(uint64_t data) {
		m_data.u64 = data;
		return *this;
	}
private:
	epoll_data_t m_data;
};

using EPEvents = std::vector<epoll_event>;  //EPEvent指代std::vector<epoll_event>

class CEpoll
{
public:
	CEpoll() {
		m_epoll = -1;
	}
	~CEpoll() {
		Close();
	}
public:
	CEpoll(const CEpoll&) = delete;  //epoll不允许复制构造
	CEpoll& operator=(const CEpoll&) = delete;  //epoll不允许有=运算符重载
public:
	operator int() const {//类型转换运算符重载
		return m_epoll;
	}
public:
	int Create(unsigned count) {
		if (m_epoll != -1) return -1;//说明已经create过了
		m_epoll = epoll_create(count);
		if (m_epoll == -1) return -2;//生成错误
		return 0;
	}
	ssize_t WaitEvents(EPEvents& events, int timeout = 10) {
		if (m_epoll == -1) return -1;//未初始化
		EPEvents evs(EVENT_SIZE);   //using EPEvents = std::vector<epoll_event>;  此时EPEvents是vector类型
		int ret = epoll_wait(m_epoll,evs.data(),(int)evs.size(),timeout);
		if (ret == -1) {//发生错误
			if ((errno == EINTR)||(errno==EAGAIN)) {//系统中断  这些情况可以忽略
				return 0;
			}
			return -2;//真正的错误 返回-2
		}
		if (ret > (int)events.size()) {//有更多的event发生
			events.resize(ret);
		}
		memcpy(events.data(), evs.data(), sizeof(epoll_event) * ret);
		return ret;
	}
	int Add(int fd, const EpollData& data = EpollData((void*)0), uint32_t events = EPOLLIN) {   //添加的时候，将event.events设置为EPOLLIN  event.data设置为客户端CSockBase指针
		if (m_epoll == -1) return -1;//未初始化
		epoll_event ev = { events ,data };
		int ret = epoll_ctl(m_epoll,EPOLL_CTL_ADD,fd,&ev);  //epoll_ctl   epoll的控制函数
		if (ret == -1) return -2;
		return 0;
	}
	int Modify(int fd, uint32_t events, const EpollData& data = EpollData((void*)0)) {
		if (m_epoll == -1) return -1;//未初始化
		epoll_event ev = { events ,data };
		int ret = epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev);
		if (ret == -1) return -2;
		return 0;
	}
	int Delete(int fd) {
		if (m_epoll == -1) return -1;//未初始化
		int ret = epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, NULL);
		if (ret == -1) return -2;
		return 0;
	}
	void Close() {
		if (m_epoll != -1) {
			int fd = m_epoll;
			m_epoll = -1;
			close(fd);//防御性编程
		}
	}
private:
	int m_epoll;
};

