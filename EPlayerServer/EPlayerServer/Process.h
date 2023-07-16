#pragma once
#include"Function.h"
#include<memory.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include <netinet/in.h>
class CProcess {//���̿��ƺ���
public:
    CProcess() {
        m_func = NULL;
        memset(pipes, 0, sizeof(pipes));
    }
    ~CProcess() {
        if (m_func != NULL) {
            delete m_func;
            m_func = NULL;
        }
    }
    template<typename _FUNCTION_, typename..._ARGS_>  //_FUNCTION_ָ����������  _ARGS_ָ���ɱ��������
    int SetEctryFunc(_FUNCTION_ func, _ARGS_... args) {
        m_func = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);//�ɱ�����õ�ʱ��...���ں���  ����ʱ...����ǰ��
        return 0;
    }
    int CreateSubPro() {
        if (m_func == NULL) return -1;
        int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);//AF_LOCAL�����׽���  pipes�ܵ������˺�������
        if (ret == -1) return -2;
        pid_t pid = fork();
        if (pid == -1) return -3;
        if (pid == 0) {
            close(pipes[1]); //pipes[1]����д���ɸ�����д�룬�ӽ���ȡ��    
            //�����Ҫ˫��ͨ�ţ�����Ҫ�������Թܵ�  ����һ���ܵ�ͬʱ˫����д��������߶����׹ܵ�  ���߶�д�ܵ����ܵ�����
            pipes[1] = 0;
            //return (*m_func)();  //*������ ��ΪCFunctionBase�����  ()�������ã��������������
            ret = (*m_func)();
            exit(0);
        }
        close(pipes[0]);  //������ֻд�� ������
        pipes[0] = 0;
        m_pid = pid;
        return 0;
    }
    int SendFD(int fd) {//�����̷����ļ����������ӽ���
        struct msghdr msg;
        bzero(&msg, sizeof(msghdr));
        iovec iov[2];
        char buffer[2][10] = { "tisheng" ,"zuwo" };
        iov[0].iov_base = buffer[0];
        iov[0].iov_len = sizeof(buffer[0]);//base�ĳ���
        iov[1].iov_base = buffer[1];
        iov[1].iov_len = sizeof(buffer[1]);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        //���²���������Ҫ���ݵ�����
        //cmsghdr* cmsg=new cmsghdr;
        //bzero(cmsg,sizeof(cmsghdr));
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int))); //CMSG_LEN(len) ��ʾsizeof(cmsghdr)+len
        memset(cmsg, 0, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        *(int*)CMSG_DATA(cmsg) = fd;   //CMSG_DATA����cmsg������ ����void*���� ��Ҫǿת����ֵ !!!!!!
        //printf("%d  fd has been send fd=%d\n", __LINE__, fd);
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

        ssize_t size = sendmsg(pipes[1], &msg, 0);  //pipes[1]д��
        free(cmsg);
        if (size == -1) {//����ʧ��
            //printf(" %d  %d  %s   this fd=%d\n", __LINE__, errno, strerror(errno),fd);
            return -2;
        }
        //delete cmsg;
        return 0;
    }
    int RecvFD(int& fd) {//�������� �����޸�
        msghdr msg;
        bzero(&msg, sizeof(msghdr));
        iovec iov[2];
        char buffer[][10] = { "","" };
        iov[0].iov_base = buffer[0];
        iov[0].iov_len = sizeof(buffer[0]);
        iov[1].iov_base = buffer[1];
        iov[1].iov_len = sizeof(buffer[1]);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));  //�������յ�����
        memset(cmsg, 0, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;//�׽��ֶ�
        cmsg->cmsg_type = SCM_RIGHTS;//������������Ҫ�õ�Ȩ��

        msg.msg_control = cmsg;//������Ϣ
        msg.msg_controllen = CMSG_LEN(sizeof(int));
        ssize_t size = recvmsg(pipes[0], &msg, 0);//pipes[0]��������
        if (size == -1) {//����ʧ��
            printf(" %d  %d  %s\n", __LINE__, errno, strerror(errno));
            return -2;
        }
        //delete cmsg;
        free(cmsg);
        fd = *(int*)CMSG_DATA(cmsg);  //�õ��ļ�������
        return 0;
    }
    int SendSocket(int fd,const struct sockaddr_in* addr) {//�����̷����ļ����������ӽ���
        struct msghdr msg;
        bzero(&msg, sizeof(msghdr));
        iovec iov;
        char buffer[20] = "";
        bzero(buffer, sizeof(buffer));
        memcpy(buffer, addr, sizeof(sockaddr_in));
        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        //���²���������Ҫ���ݵ�����
        //cmsghdr* cmsg=new cmsghdr;
        //bzero(cmsg,sizeof(cmsghdr));
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int))); //CMSG_LEN(len) ��ʾsizeof(cmsghdr)+len
        memset(cmsg, 0, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        *(int*)CMSG_DATA(cmsg) = fd;   //CMSG_DATA����cmsg������ ����void*���� ��Ҫǿת����ֵ !!!!!!
        //printf("%d  fd has been send fd=%d\n", __LINE__, fd);
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

        ssize_t size = sendmsg(pipes[1], &msg, 0);  //pipes[1]д��
        free(cmsg);
        if (size == -1) {//����ʧ��
            printf(" %d  %d  %s   this fd=%d\n", __LINE__, errno, strerror(errno),fd);
            return -2;
        }
        //delete cmsg;
        return 0;
    }

    int RecvSocket(int& fd, const struct sockaddr_in* addr) {//�������� �����޸�
        msghdr msg;
        bzero(&msg, sizeof(msghdr));
        iovec iov;
        char buffer[20] = "";
        bzero(buffer, sizeof(buffer));
        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));  //�������յ�����
        memset(cmsg, 0, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;//�׽��ֶ�
        cmsg->cmsg_type = SCM_RIGHTS;//������������Ҫ�õ�Ȩ��

        msg.msg_control = cmsg;//������Ϣ
        msg.msg_controllen = CMSG_LEN(sizeof(int));
        ssize_t size = recvmsg(pipes[0], &msg, 0);//pipes[0]��������
        if (size == -1) {//����ʧ��
            printf(" %d  %d  %s\n", __LINE__, errno, strerror(errno));
            return -2;
        }
        //delete cmsg;
        memcpy((void*)addr, buffer, sizeof(sockaddr_in));
        free(cmsg);
        fd = *(int*)CMSG_DATA(cmsg);  //�õ��ļ�������
        return 0;
    }

    static int SwitchDeamon() {  //�ػ���������
        pid_t ret = fork();
        if (ret == -1) return -1;
        if (ret > 0) exit(0);//�������˳�
        //�ӽ���
        ret = setsid();
        if (ret == -1) return -2;//setsid()ʧ��  �ɹ�����ִ��
        ret = fork();
        if (ret == -1) return -3;
        if (ret > 0) exit(0);//�ӽ����˳� 
        //����� �����ػ�״̬
        for (int i = 0; i < 3; i++) {
            close(i);  //�رձ�׼�������
        }
        umask(0);//����ļ��ڱ�λ
        signal(SIGCHLD, SIG_IGN);  //����signal
        return 0;
    }
private:
    CFunctionBase* m_func;
    pid_t m_pid;
    int pipes[2];//�ܵ�
};