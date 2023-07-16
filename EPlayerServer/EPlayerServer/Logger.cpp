#include"Logger.h"

LogInfo::LogInfo(const char* file, int line, const char* func, 
	pid_t pid, pthread_t tid, int loglevel, const char* fmt, ...) {  //�ɱ���� �����...������const char*�����
	const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATIAL" };
	char* buf = NULL;
	bAuto = false;
	int count = asprintf(&buf, "%s(%d):[%s] [%s]<%d-%d>(%s) ", 
		file, line, sLevel[loglevel], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);  //����������ʾ
	if (count > 0) {
		m_buf = buf;
		free(buf);
	}
	else return;
	va_list ap;
	va_start(ap, fmt);
	count = vasprintf(&buf, fmt, ap);//�û����ݵ���ʾ
	if (count > 0) {
		m_buf += buf;//����û�����  ���ɱ������ʾ������
		free(buf);
	}
	m_buf += "\n";
	va_end(ap);
}

LogInfo::LogInfo(const char* file, int line, const char* func, pid_t pid, pthread_t tid, int loglevel) {  //  <<����� 
	bAuto = true;
	const char sLevel[][8] = { "INFO","DEBUG","WARNING","ERROR","FATIAL" };
	char* buf = NULL;
	int count = asprintf(&buf, "%s(%d):[%s] [%s]<%d-%d>(%s)", 
		file, line, sLevel[loglevel], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);  //����������ʾ
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
		file, line, sLevel[loglevel], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);  //����������ʾ
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
		snprintf(buff, sizeof(buff), "%02X ", (Data[i] & 0xFF));  //Data[i]&0xFF �������λ��չ��ǰ���FF֮ǰ��Ϊȫ����ֵΪ1 ��������FFǰ��1ȫ����0
		m_buf += buff;
		if (0 == (i + 1) % 16) {//ÿ16���ַ���ʾһ��������ݵ��ַ���ʽ
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
			for (size_t j = i - 15; j <= i; j++) {  //��ǰ����ַ����
				
				if ((Data[j] & 0xFF) > 31 && (Data[j] & 0xFF) < 0x7F) {  //С��32�ǿ����ַ� �޷���ӡ ����0x7F�޷���ʾ���ַ�
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
	//β������16�����ַ� ����
	size_t k = i % 16;  //i==17  0~15�Ѿ���ӡ k=1  j=16 ��Ҫ��ӡ 16 17
	if (k != 0) {
		//m_buf += "\t; ";
		for (size_t j = 0; j < 16 - k; j++) m_buf += " ";//��������𣿣���
		m_buf += "\t; ";
		for (size_t j = i - k; j <= i; j++) {  //��ǰ����ַ����
			if ((Data[j] & 0xFF) > 31 && (Data[j] & 0xFF) < 0x7F) {  //С��32�ǿ����ַ� �޷���ӡ ����0x7F�޷���ʾ���ַ�
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
	if (bAuto) {  //��ʽ��־ ��Ҫ������
		m_buf += "\n";
		CLoggerServer::Trace(*this);
	}
}