#include"Logger.h"

LogInfo::LogInfo(const char* file, int line, const char* func, 
	pid_t pid, pthread_t tid, int loglevel, const char* fmt, ...) {  //可变参数 后面的...都按照const char*来输出
	const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATIAL" };
	char* buf = NULL;
	bAuto = false;
	int count = asprintf(&buf, "%s(%d):[%s] [%s]<%d-%d>(%s) ", 
		file, line, sLevel[loglevel], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);  //主体数据显示
	if (count > 0) {
		m_buf = buf;
		free(buf);
	}
	else return;
	va_list ap;
	va_start(ap, fmt);
	count = vasprintf(&buf, fmt, ap);//用户数据的显示
	if (count > 0) {
		m_buf += buf;//添加用户数据  即可变参数表示的数据
		free(buf);
	}
	m_buf += "\n";
	va_end(ap);
}

LogInfo::LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int loglevel) {  //  <<运算符 
	bAuto = true;
	const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATIAL" };
	char* buf = NULL;
	int count = asprintf(&buf, "%s(%d):[%s] [%s]<%d-%d>(%s)", 
		file, line, sLevel[loglevel], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);  //主体数据显示
	if (count > 0) {
		m_buf = buf;
		free(buf);
	}
}

LogInfo::LogInfo(const char* file, int line, const char* func, 
	pid_t pid, pthread_t tid, int loglevel, void* pdata, size_t nSize) {
	bAuto = false;
	const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATIAL" };
	char* buf = NULL;
	int count = asprintf(&buf, "%s(%d):[%s] [%s]<%d-%d>(%s) \n", 
		file, line, sLevel[loglevel], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);  //主体数据显示
	if (count > 0) {
		m_buf = buf;
		free(buf);
	}
	else return;

	Buffer out;
	size_t i = 0;
	char* Data = (char*)pdata;
	for (; i < nSize; i++) {
		char buff[16] = "";
		snprintf(buff, sizeof(buff), "%02X ", (Data[i] & 0xFF));  //Data[i]&0xFF 避免符号位拓展将前面的FF之前的为全部赋值为1 此做法将FF前的1全部赋0
		m_buf += buff;
		if (0 == (i + 1) % 16) {//每16个字符显示一次这段数据的字符形式
			m_buf += "\t; ";
			char b[17] = "";
			memcpy(b, Data + i - 15, 16);
			for (int j = 0; j < 16; j++) {
				if (b[j] < 32 && b[j] >= 0) {
					b[j] = '.';
				}
			}
			m_buf += b;

			/*
			for (size_t j = i - 15; j <= i; j++) {  //把前面的字符输出
				
				if ((Data[j] & 0xFF) > 31 && (Data[j] & 0xFF) < 0x7F) {  //小于32是控制字符 无法打印 大于0x7F无法显示的字符
					m_buf += Data[i];
				}
				else {
					m_buf += ".";
				}
			}*/
			m_buf += "\n";
			m_buf += "\n";

		}
	}
	//尾部不足16个的字符 处理
	size_t k = i % 16;  //i==17  0~15已经打印 k=1  j=16 需要打印 16 17
	if (k != 0) {
		//m_buf += "\t; ";
		for (size_t j = 0; j < 16 - k; j++) m_buf += " ";//倒序输出吗？？？
		m_buf += "\t; ";
		for (size_t j = i - k; j <= i; j++) {  //把前面的字符输出
			if ((Data[j] & 0xFF) > 31 && (Data[j] & 0xFF) < 0x7F) {  //小于32是控制字符 无法打印 大于0x7F无法显示的字符
				m_buf += Data[i];
			}
			else {
				m_buf += ".";
			}
		}
		m_buf += "\n";
	}
	//printf("\n m_buf=%s", m_buf);
}

LogInfo::~LogInfo() {
	if (bAuto) {  //流式日志 需要主动发
		m_buf += "\n";
		CLoggerServer::Trace(*this);
	}
}