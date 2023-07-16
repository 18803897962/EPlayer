#pragma once
#include<string>
#include<string.h>
class Buffer :public std::string {
public:
	Buffer() :std::string() {//调用父类构造函数
	}
	Buffer(size_t size) :std::string() {
		resize(size);
	}
	Buffer(const std::string& str) :std::string(str) {
	}
	Buffer(const char* str) :std::string(str) {}
	Buffer(const char* str, size_t len) :std::string() {
		resize(len);
		memcpy((char*)c_str(), str, len);
	}
	Buffer(const char* begin, const char* end) :std::string() {//左闭右开区间
		long int len = end - begin;  //包含begin 不包含end
		if (len > 0) {
			resize(len);
			memcpy((char*)c_str(), begin, len);//从begin开始，copy长度为len
		}
	}
	operator char* () {
		return (char*)c_str();
	}
	operator char* () const {
		return (char*)c_str();
	}
	operator const void* () const {
		return c_str();
	}
	operator unsigned char* () const {
		return (unsigned char*)c_str();
	}
	operator const char* () const {
		return (char*)c_str();
	}
};


